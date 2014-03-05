(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.resolveAttribute = function(program, attribute, quiet) {
        var prev = null;
        var def = null;
        var partialQualifiers = [];
        var env = program.environment;
        
        for(var i = 0; i < attribute.pieces.length; i++) {
            var piece = attribute.pieces[i];
            partialQualifiers.push(piece);
            
            if(!prev) {
                def = env.get(piece);
            } else {
                var pkg = wiz.sym.check('PackageDef', prev);
                if(!pkg) {
                    var previousName = partialQualifiers.slice(0, partialQualifiers.length - 1).join(".");
                    wiz.compile.error("attempt to get symbol '" + attribute.fullName + "', but '" + previousName + "' is not a package", attribute.location);
                } else {
                    env = pkg.environment;
                    def = env.get(piece, true); // qualified lookup is shallow.
                }
            }
            
            if(!def) {
                var partiallyQualifiedName = partialQualifiers.join(".");
                var fullyQualifiedName = attribute.fullName;
                
                if(!quiet) {
                    if(partiallyQualifiedName == fullyQualifiedName) {
                        wiz.compile.error("reference to undeclared symbol '" + partiallyQualifiedName + "'", attribute.location);
                    } else {
                        wiz.compile.error("reference to undeclared symbol '" + partiallyQualifiedName + "' (in '" + fullyQualifiedName + "')", attribute.location);
                    }
                }
                return null;
            }
            
            prev = def;
        }
        return def;
    }

    wiz.compile.tryFoldConstant = function(program, root, runtimeForbidden, finalized, result) {
        var values = {}; // Expression -> uint
        var completeness = {} // Expression -> bool
        var runtimeRootForbidden = runtimeForbidden;
        var badAttr = false;
        var depth = 0;

        function updateValue(node, value, complete) {
            complete = typeof(complete) !== 'undefined' ? complete : true;
            values[node.nodeId] = value;
            completeness[node.nodeId] = complete;
            if(depth == 0 && complete) {
                result.constTail = node;
            }
        }

        result.constTail = null;
        wiz.compile.traverse(root, {
            Infix: function(e) {
                var first = e.operands[0];

                if(typeof(values[first.nodeId]) === 'undefined') {
                    if(depth == 0) {
                        result.constTail = null;
                    }
                    return;
                }

                var a = values[first.nodeId];
                updateValue(e, a, false);

                for(var i = 0; i < e.types.length; i++) {
                    var type = e.types[i];
                    var operand = e.operands[i + 1];
                    if(typeof(values[operand.nodeId]) === 'undefined') {
                        if(depth == 0) {
                            result.constTail = e.operands[i];
                        }
                        return;
                    }
                    var b = values[operand.nodeId];

                    switch(type) {
                        case wiz.parse.Infix.Add: 
                            if((a + b) % wiz.ast.Expression.Max < a) {
                                wiz.compile.error("addition yields result which will overflow outside of 0..65535.", operand.location);
                                return;
                            } else {
                                a += b;
                            }
                            break;
                        case wiz.parse.Infix.Sub:
                            if(a < b) {
                                wiz.compile.error("subtraction yields result which will overflow outside of 0..65535.", operand.location);
                                return;
                            } else {
                                a -= b;
                            }
                            break;
                        case wiz.parse.Infix.Mul:
                            if(b != 0 && a > Math.floor(wiz.ast.Expression.Max / b))
                            {
                                wiz.compile.error("multiplication yields result which will overflow outside of 0..65535.", operand.location);
                                return;
                            } else {
                                a *= b;
                            }
                            break;
                        case wiz.parse.Infix.Div:
                            if(a == 0) {
                                wiz.compile.error("division by zero is undefined.", operand.location);
                                return;
                            } else {
                                a = Math.floor(a / b);
                            }
                            break;
                        case wiz.parse.Infix.Mod:
                            if(a == 0) {
                                wiz.compile.error("modulo by zero is undefined.", operand.location);
                                return;
                            } else {
                                a %= b;
                            }
                            break;
                        case wiz.parse.Infix.ShiftL:
                            // If shifting more than N bits, or ls << rs > 2^N-1, then wiz.compile.error.
                            if(b > 15 || (a << b >> 16) != 0) {
                                wiz.compile.error("logical shift left yields result which will overflow outside of 0..65535.", operand.location);
                                return;
                            } else {
                                a <<= b;
                            }
                            break;
                        case wiz.parse.Infix.ShiftR:
                            a >>= b;
                            break;
                        case wiz.parse.Infix.And:
                            a &= b;
                            break;
                        case wiz.parse.Infix.Or:
                            a |= b;
                            break;
                        case wiz.parse.Infix.Xor:
                            a ^= b;
                            break;
                        case wiz.parse.Infix.At:
                            a &= (1 << b);
                            break;
                        case wiz.parse.Infix.AddC:
                        case wiz.parse.Infix.SubC:
                        case wiz.parse.Infix.ArithShiftL:
                        case wiz.parse.Infix.ArithShiftR:
                        case wiz.parse.Infix.RotateL:
                        case wiz.parse.Infix.RotateR:
                        case wiz.parse.Infix.RotateLC:
                        case wiz.parse.Infix.RotateRC:
                        case wiz.parse.Infix.Colon:
                        default:
                            if(runtimeForbidden) {
                                wiz.compile.error("infix operator " + wiz.parse.getInfixName(type) + " cannot be used in constant expression", operand.location);
                            }
                            if(depth == 0) {
                                result.constTail = e.operands[i];
                            }
                            return;
                    }
                    updateValue(e, a, i == e.types.length - 1);
                }
            },

            Prefix: {
                pre: function(e) {
                    if(e.type == wiz.parse.Prefix.Grouping) {
                        depth++;
                        runtimeForbidden = false;
                    } else if(e.type == wiz.parse.Prefix.Indirection) {
                        depth++;
                    }
                },
                post: function(e) {
                    switch(e.type) {
                        case wiz.parse.Prefix.Low:
                        case wiz.parse.Prefix.High:
                        case wiz.parse.Prefix.Swap:
                            break;
                        case wiz.parse.Prefix.Grouping:
                            depth--;
                            if(depth == 0) {
                                runtimeForbidden = runtimeRootForbidden;
                            }
                            break;
                        case wiz.parse.Prefix.Not:
                        case wiz.parse.Prefix.Sub:
                            if(runtimeForbidden) {
                                wiz.compile.error("prefix operator " + wiz.parse.getPrefixName(e.type) + " cannot be used in constant expression", e.location);
                            }
                            return;
                        case wiz.parse.Prefix.Indirection:
                            depth--;
                            if(runtimeForbidden) {
                                wiz.compile.error("indirection operator cannot be used in constant expression", e.location);
                            }
                            return;
                    }
                    
                    if(typeof(values[e.operand.nodeId]) === 'undefined') {
                        return;
                    }
                    var r = values[e.operand.nodeId];
                    switch(e.type) {
                        case wiz.parse.Prefix.Low:
                            updateValue(e, r & 0xFF);
                            break;
                        case wiz.parse.Prefix.High:
                            updateValue(e, (r >> 8) & 0xFF);
                            break;
                        case wiz.parse.Prefix.Swap:
                            updateValue(e, ((r & 0x0F0F) << 4) | ((r & 0xF0F0) >> 4));
                            break;
                        case wiz.parse.Prefix.Grouping:
                            updateValue(e, r);
                            break;
                        case wiz.parse.Prefix.Not:
                        case wiz.parse.Prefix.Indirection:
                        case wiz.parse.Prefix.Sub:
                            break;
                    }
                }
            },

            Postfix: function(e) {
                if(typeof(values[e.operand.nodeId]) === 'undefined') {
                    return;
                } else {
                    if(runtimeForbidden) {
                        wiz.compile.error("postfix operator " + wiz.parse.getPostfixName(e.type) + " cannot be used in constant expression", e.location);
                    }
                }
            },

            Attribute: function(a) {
                var def = wiz.compile.resolveAttribute(program, a);
                if(!def) {
                    badAttr = true;
                    return;
                }
                if(wiz.sym.check('ConstDef', def)) {
                    program.enterInline("constant '" + a.fullName + "'", a);
                    program.enterEnvironment(def.environment);
                    var v = {};
                    var folded = wiz.compile.foldConstant(program, def.decl.value, program.finalized, v);
                    program.leaveEnvironment();
                    program.leaveInline();
                    if(folded) {
                        updateValue(a, v.value);
                        return;
                    }
                } else if(wiz.sym.check('VarDef', def)) {
                    if(def.hasAddress) {
                        updateValue(a, def.address);
                        return;
                    }
                } else if(wiz.sym.check('LabelDef', def)) {
                    if(def.hasAddress) {
                        updateValue(a, def.address);
                        return;
                    }
                }

                if(def && runtimeForbidden && finalized) {
                    wiz.compile.error("'" + a.fullName + "' was declared, but could not be evaluated.", a.location);
                }
            },

            String: function(s) {
                wiz.compile.error("string literal is not allowed here", s.location);
            },

            Number: function(n) {
                updateValue(n, n.value);
            },
        });

        result.value = typeof(values[root.nodeId]) !== 'undefined' ? values[root.nodeId] : 0xCACA;
        if(!completeness[root.nodeId] && runtimeForbidden && finalized) {
            wiz.compile.error("expression could not be resolved as a constant", root.location);
            result.constTail = null;
            return false;
        }
        if(badAttr) {
            result.constTail = null;
            return false;
        }
        return true;
    }

    wiz.compile.foldConstant = function(program, root, finalized, result) {
        return wiz.compile.tryFoldConstant(program, root, true, finalized, result) && result.constTail == root;
    }

    wiz.compile.foldBoundedNumber = function(program, root, type, limit, finalized, result) {
        if(wiz.compile.foldConstant(program, root, finalized, result)) {
            if(result.value > limit) {
                wiz.compile.error(
                    "value " + result.value + " is outside of representable " + type + " range 0.." + limit,
                    root.location
                );
                return false;
            }
            return true;
        }
        return false;
    }

    wiz.compile.foldBit = function(program, root, finalized, result) {
        return wiz.compile.foldBoundedNumber(program, root, "bit", 1, finalized, result);
    }

    wiz.compile.foldBitIndex = function(program, root, finalized, result) {
        return wiz.compile.foldBoundedNumber(program, root, "bitwise index", 7, finalized, result);
    }

    wiz.compile.foldWordBitIndex = function(program, root, finalized, result) {
        return wiz.compile.foldBoundedNumber(program, root, "bitwise index", 15, finalized, result);
    }

    wiz.compile.foldByte = function(program, root, finalized, result) {
        return wiz.compile.foldBoundedNumber(program, root, "8-bit", 255, finalized, result);
    }

    wiz.compile.foldWord = function(program, root, finalized, result) {
        return wiz.compile.foldBoundedNumber(program, root, "16-bit", 65535, finalized, result);
    }

    wiz.compile.foldSignedByte = function(program, root, finalized, result) {
        if(wiz.compile.foldWord(program, root, finalized, result)) {
            if(!negative && result.value < 127) {
                return true;
            } else if(negative && result.value < 128) {
                result.value = ~result.value + 1;
                return true;
            } else {
                wiz.compile.error(
                    "value " + (negative ? "-" : "") + result.value + " is outside of representable signed 8-bit range -128..127.",
                    root.location
                );
            }
        }
        return false;
    }

    wiz.compile.foldRelativeByte = function(program, root, description, help, origin, finalized, result) {
        if(wiz.compile.foldWord(program, root, finalized, result)) {
            var offset = result.value - origin;
            if(offset >= -128 && offset <= 127) {
                result.value = offset & 255;
                return true;
            } else {
                wiz.compile.error(
                    description + " is outside of representable signed 8-bit range -128..127. "
                    + help + " (from = " + origin + ", to = " + result.value + ", (from - to) = " + offset + ")",
                    root.location
                );
            }
        }
        return false;
    }

    wiz.compile.foldStorage = function(program, s, result) {
        result.sizeless = false;
        if(!s.size) {
            result.sizeless = true;
            result.value = 1;
        } else if(!wiz.compile.foldConstant(program, s.size, true, result)) {
            return false;
        }
        switch(s.type) {
            case wiz.parse.Keyword.Byte: result.unit = 1; break;
            case wiz.parse.Keyword.Word: result.unit = 2; break;
            default:
                wiz.compile.error("Unsupported storage type " + wiz.parse.getKeywordName(s.type), s.location);
        }
        result.value *= result.unit;
        return true;
    }

    wiz.compile.foldDataExpression = function(program, root, unit, finalized) {
        switch(unit) {
            case 1:
                if(wiz.ast.check('String', root)) {
                    return root.value;
                } else {
                    var result = {};
                    wiz.compile.foldByte(program, root, finalized, result);
                    return [result.value & 0xFF];
                }
            case 2:
                var result = {};
                wiz.compile.foldWord(program, root, finalized, result);
                return [result.value & 0xFF, (result.value >> 8) & 0xFF];
            default:
                throw new Error("unhandled unit size");
        }
    }

    wiz.compile.createBlockHandler = function(program) {
        return {
            pre: function(block) {
                var env = program.nextNodeEnvironment(block);
                if(!env) {
                    if(block.name) {
                        var match = program.environment.get(block.name, true);
                        if(wiz.sym.check('PackageDef', match)) {
                            env = match.environment;
                        } else {
                            env = wiz.compile.Environment(program.environment);
                        }
                    } else {
                        env = wiz.compile.Environment(program.environment);
                    }
                    program.createNodeEnvironment(block, env);
                }
                program.enterEnvironment(env);
            },
            post: function(block) {
                var pkg = program.environment;
                program.leaveEnvironment();
                if(block.name) {
                    var match = program.environment.get(block.name, true);
                    if(wiz.sym.check('PackageDef', match) === null) {
                        program.environment.put(block.name, wiz.sym.PackageDef(block, pkg));
                    }
                }
            }
        };
    }

    wiz.compile.createRelocationHandler = function(program) {
        return function(stmt) {
            var description = "'in' statement";
            var address = {};
            if(!stmt.dest || wiz.compile.foldConstant(program, stmt.dest, program.finalized, address)) {
                var def = wiz.sym.check('BankDef', program.environment.get(stmt.mangledName));
                if(def) {
                    program.switchBank(def.bank);
                    if(stmt.dest) {
                        def.bank.setAddress(description, address.value, stmt.location);
                    }
                } else {
                    wiz.compile.error("unknown bank '" + stmt.name + "' referenced by " + description, stmt.location);
                }
            }
        };
    }

    wiz.compile.createPushHandler = function(program) {
        return function(stmt) {
            var description = "'push' statement";
            var code = program.platform.generatePush(program, stmt);
            var bank = program.checkBank(description, stmt.location);
            if(program.finalized) {
                bank.writePhysical(code, stmt.location);
            } else {
                bank.reservePhysical(description, code.length, stmt.location);
            }
        };
    }

    wiz.compile.createInlineCallValidator = function(program) {
        return {
            pre: function(stmt) {
                if(stmt.type == wiz.parse.Keyword.Inline) {
                    program.enterInline("'inline call'", stmt);
                }
            },
            post: function(stmt)  {
                if(stmt.type == wiz.parse.Keyword.Inline) {
                    program.leaveInline();
                }
            }
        };
    }

    wiz.compile.createJumpHandler = function(program) {
        return {
            pre: function(stmt) {
                if(stmt.type == wiz.parse.Keyword.Inline) {
                    program.enterInline("'inline call'", stmt);
                }
            },
            post: function(stmt) {
                if(stmt.type == wiz.parse.Keyword.Inline) {
                    program.leaveInline();
                    return;
                }
                var description = wiz.parse.getKeywordName(stmt.type) + " statement";
                var code = program.platform.generateJump(program, stmt);
                var bank = program.checkBank(description, stmt.location);
                if(program.finalized) {
                    bank.writePhysical(code, stmt.location);
                } else {
                    bank.reservePhysical(description, code.length, stmt.location);
                }
            }
        };
    }

    wiz.compile.createAssignmentHandler = function(program) {
        return function(stmt) {
            var description = "assignment";
            var code = program.platform.generateAssignment(program, stmt);
            var bank = program.checkBank(description, stmt.location);
            if(program.finalized) {
                bank.writePhysical(code, stmt.location);
            } else {
                bank.reservePhysical(description, code.length, stmt.location);
            }
        };
    }

    wiz.compile.createComparisonHandler = function(program) {
        return function(stmt) {
            var description = "comparison";
            var code = program.platform.generateComparison(program, stmt);
            var bank = program.checkBank(description, stmt.location);
            if(program.finalized) {
                bank.writePhysical(code, stmt.location);
            } else {
                bank.reservePhysical(description, code.length, stmt.location);
            }
        };
    }

    wiz.compile.build = function(program, root) {
        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),

            LetDecl: function(decl) {
                program.environment.put(decl.name, wiz.sym.ConstDef(decl, program.environment));
            },

            Conditional: function(cond) {
                cond.expand();
            },

            Loop: function(loop) {
                loop.expand();
            },

            FuncDecl: function(decl) {
                decl.expand();
                program.environment.put(decl.name, wiz.sym.FuncDef(decl));
            },

            Unroll: function(unroll) {
                var times = {};
                if(wiz.compile.foldConstant(program, unroll.repetitions, true, times)) {
                    unroll.expand(times.value);
                }
            }
        });

        var depth = 0;
        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),

            Loop: {
                pre: function(loop) {
                    depth++;
                },
                post: function(loop) {
                    depth--;
                }
            },

            Jump: function(jump) {
                switch(jump.type) {
                    case wiz.parse.Keyword.While:
                    case wiz.parse.Keyword.Until:
                    case wiz.parse.Keyword.Break:
                    case wiz.parse.Keyword.Continue:
                        if(depth == 0) {
                            wiz.compile.error("'" + wiz.parse.getKeywordName(jump.type) + "' used outside of a 'loop'.", jump.location);
                        } else {
                            jump.expand();
                        }
                        break;
                    case wiz.parse.Keyword.Call:
                        var a = wiz.ast.check('Attribute', jump.destination);
                        if(a) {
                            var def = wiz.compile.resolveAttribute(program, a, true);
                            if(wiz.sym.check('FuncDef', def)) {
                                if(def.decl.inlined) {
                                    wiz.compile.error("call to inline function '" + a.fullName + "' must be 'inline call'.", jump.location);
                                }
                            }
                        }
                        break;
                    case wiz.parse.Keyword.Inline:
                        var a = wiz.ast.check('Attribute', jump.destination);
                        if(a) {
                            var def = wiz.compile.resolveAttribute(program, a);
                            if(def) {
                                if(wiz.sym.check('FuncDef', def)) {
                                    jump.expand(def.decl.inner);
                                } else {
                                    wiz.compile.error("an inline call to a non-function really makes no sense.", jump.location);
                                }
                            }
                        } else {
                            wiz.compile.error("an inline call to a non-function really makes no sense.", jump.location);
                        }
                        break;
                    default:
                }
            }
        });

        wiz.compile.verify();

        program.clearEnvironment();
        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),
            Jump: wiz.compile.createInlineCallValidator(program),

            LetDecl: function(decl) {
                var aliasing = false;
                var attr = wiz.ast.check('Attribute', decl.value);
                if(attr) {
                    var def = wiz.compile.resolveAttribute(program, attr, true);
                    if(def) {
                        aliasing = true;
                        program.environment.put(decl.name, wiz.sym.AliasDef(decl, def));
                    }
                }
                if(!aliasing) {
                    program.environment.put(decl.name, wiz.sym.ConstDef(decl, program.environment));
                }
            },

            BankDecl: function(decl) {
                for(var i = 0; i < decl.names.length; i++) {
                    program.environment.put(decl.names[i], wiz.sym.BankDef(decl));
                }
            },

            VarDecl: function(decl) {
                for(var i = 0; i < decl.names.length; i++) {
                    program.environment.put(decl.names[i], wiz.sym.VarDef(decl));
                }
            },

            LabelDecl: function(decl) {
                program.environment.put(decl.name, wiz.sym.LabelDef(decl));
            },
        });

        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),

            BankDecl: function(decl) {
                var size = {};
                if(wiz.compile.foldConstant(program, decl.size, true, size)) {
                    for(var i = 0; i < decl.names.length; i++) {
                        var name = decl.names[i];
                        var def = wiz.sym.check('BankDef', program.environment.get(name));
                        if(def) {
                            def.bank = wiz.compile.Bank(name, decl.type == "rom", size.value);
                            program.addBank(def.bank);
                        }
                    }
                }
            }
        });
        wiz.compile.verify();

        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),
            Relocation: wiz.compile.createRelocationHandler(program),

            VarDecl: function(decl) {
                var description = "variable declaration";
                var result = {};
                if(wiz.compile.foldStorage(program, decl.storage, result)) {
                    var bank = program.checkBank(description, decl.location);
                    for(var i = 0; i < decl.names.length; i++) {
                        var def = wiz.sym.check('VarDef', program.environment.get(decl.names[i]));
                        if(def) {
                            def.hasAddress = true;
                            def.address = bank.checkAddress(description, decl.location);
                            bank.reserveVirtual(description, result.value, decl.location);
                        }
                    }
                }
            },
        });
        wiz.compile.verify();

        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),
            Relocation: wiz.compile.createRelocationHandler(program),
            Push: wiz.compile.createPushHandler(program),
            Jump: wiz.compile.createJumpHandler(program),
            Assignment: wiz.compile.createAssignmentHandler(program),
            Comparison: wiz.compile.createComparisonHandler(program),

            LabelDecl: function(decl) {
                var description = "label declaration";
                var bank = program.checkBank(description, decl.location);
                var def = wiz.sym.check('LabelDef', program.environment.get(decl.name));
                if(def) {
                    def.hasAddress = true;
                    def.address = bank.checkAddress(description, decl.location);
                }
            },

            Embed: function(stmt) {
                var description = "'embed' statement";

                var file = wiz.fs.open(filename);
                if(!file) {
                    wiz.compile.error("could not embed file '" + stmt.filename + "'", stmt.location, true);
                    return;
                }
                stmt.data = file.readBinary();
                
                var bank = program.checkBank(description, stmt.location);
                bank.reservePhysical(description, stmt.data.length, stmt.location);
            },

            Data: function(stmt) {
                var description = "inline data";
                var result = {};
                if(wiz.compile.foldStorage(program, stmt.storage, result)) {
                    var data = [];
                    if(!stmt.items.length) {
                        throw new Error('no items in data statement');
                    }
                    for(var i = 0; i < stmt.items.length; i++) {
                        var item = stmt.items[i];
                        var bytes = wiz.compile.foldDataExpression(program, item, result.unit, program.finalized);
                        for(var j = 0; j < bytes.length; j++) {
                            data.push(bytes[j]);
                        }
                    }
                    if(!result.sizeless) {
                        if(data.length < result.value) {
                            // Fill unused section with final byte of data.
                            var pad = data[data.length - 1];
                            var k = result.value - data.length;
                            while(k > 0) {
                                data.push(pad);
                                k--;
                            }
                        } else if(data.length > result.value) {
                            wiz.compile.error(
                                description + " is an " + data.length + "-byte sequence, which is "
                                + (data.length - result.value) + " byte(s) over the declared "
                                + result.value + "-byte limit",
                                stmt.location
                            );
                        }
                    }
                    var bank = program.checkBank(description, stmt.location);
                    bank.reservePhysical(description, data.length, stmt.location);
                }
            }
        });
        wiz.compile.verify();

        program.finalized = true;
        program.rewind();
        wiz.compile.traverse(root, {
            Block: wiz.compile.createBlockHandler(program),
            Relocation: wiz.compile.createRelocationHandler(program),
            Push: wiz.compile.createPushHandler(program),
            Jump: wiz.compile.createJumpHandler(program),
            Assignment: wiz.compile.createAssignmentHandler(program),
            Comparison: wiz.compile.createComparisonHandler(program),

            LabelDecl: function(decl) {
                var description = "label declaration";
                var bank = program.checkBank(description, decl.location);
                var def = wiz.sym.check('LabelDef', program.environment.get(name));
                if(def) {
                    var addr = bank.checkAddress(description, decl.location);
                    if(!def.hasAddress) {
                        wiz.compile.error("what the hell. label was never given address!", decl.location, true);
                    }
                    if(addr != def.address) {
                        wiz.compile.error(
                            "what the hell. inconsistency in label positions detected"
                            + " (was " + addr + " on instruction selection pass, " + def.address + " on code-gen pass)",
                            decl.location, true
                        );
                    }
                }
            },

            Embed: function(stmt) {
                var description = "'embed' statement";
                var bank = program.checkBank(description, stmt.location);
                bank.writePhysical(stmt.data, stmt.location);
            },

            Data: function(stmt) {
                var description = "inline data";
                var result = {};
                if(wiz.compile.foldStorage(program, stmt.storage, result)) {
                    var data = [];
                    if(!stmt.items.length) {
                        throw new Error('no items in data statement');
                    }
                    for(var i = 0; i < stmt.items.length; i++) {
                        var item = stmt.items[i];
                        var bytes = wiz.compile.foldDataExpression(program, item, result.unit, program.finalized);
                        for(var j = 0; j < bytes.length; j++) {
                            data.push(bytes[j]);
                        }
                    }
                    if(!result.sizeless) {
                        if(data.length < result.value) {
                            // Fill unused section with final byte of data.
                            var pad = data[data.length - 1];
                            var k = result.value - data.length;
                            while(k > 0) {
                                data.push(pad);
                                k--;
                            }
                        } else if(data.length > result.value) {
                            wiz.compile.error(
                                description + " is an " + data.length + "-byte sequence, which is "
                                + (data.length - result.value) + " byte(s) over the declared "
                                + result.value + "-byte limit",
                                stmt.location
                            );
                        }
                    }
                    var bank = program.checkBank(description, stmt.location);
                    bank.writePhysical(data, stmt.location);
                }
            }
        });
        wiz.compile.verify();
    }

})(this);