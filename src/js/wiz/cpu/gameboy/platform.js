/*jslint bitwise: true */
(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.cpu = typeof(wiz.cpu) !== 'undefined' ? wiz.cpu : {};
    wiz.cpu.gameboy = typeof(wiz.cpu.gameboy) !== 'undefined' ? wiz.cpu.gameboy : {};

wiz.cpu.gameboy.GameboyPlatform = function() {
    var that = wiz.cpu.AbstractPlatform();
    that.builtins = builtins;
    that.generatePush = generatePush;
    that.generateJump = generateJump;
    that.generateComparison = generateComparison;
    that.generateAssignment = generateAssignment;
    that.patch = patch;

    var Argument = wiz.cpu.gameboy.Argument;
    var ArgumentType = wiz.cpu.gameboy.ArgumentType;
    var buildIndirection = wiz.cpu.gameboy.buildIndirection;
    var buildArgument = wiz.cpu.gameboy.buildArgument;
    var error = wiz.compile.error;

    function builtins() {
        return {
            "a": wiz.cpu.gameboy.Builtin(ArgumentType.A),
            "b": wiz.cpu.gameboy.Builtin(ArgumentType.B),
            "c": wiz.cpu.gameboy.Builtin(ArgumentType.C),
            "d": wiz.cpu.gameboy.Builtin(ArgumentType.D),
            "e": wiz.cpu.gameboy.Builtin(ArgumentType.E),
            "f": wiz.cpu.gameboy.Builtin(ArgumentType.F),
            "h": wiz.cpu.gameboy.Builtin(ArgumentType.H),
            "l": wiz.cpu.gameboy.Builtin(ArgumentType.L),
            "af": wiz.cpu.gameboy.Builtin(ArgumentType.AF),
            "bc": wiz.cpu.gameboy.Builtin(ArgumentType.BC),
            "de": wiz.cpu.gameboy.Builtin(ArgumentType.DE),
            "hl": wiz.cpu.gameboy.Builtin(ArgumentType.HL),
            "sp": wiz.cpu.gameboy.Builtin(ArgumentType.SP),
            "carry": wiz.cpu.gameboy.Builtin(ArgumentType.Carry),
            "zero": wiz.cpu.gameboy.Builtin(ArgumentType.Zero),
            "interrupt": wiz.cpu.gameboy.Builtin(ArgumentType.Interrupt),
        };
    }

    function patch(buffer) {
        var headersum = 0;
        var globalsum = 0;
        for(var i = 0x134; i <= 0x14D; i++) {
            headersum = (headersum - buffer[i] - 1) & 0xFF;
        }
        buffer[0x14D] = headersum;
        for(var i = 0; i < buffer.length; i++) {
            if(i != 0x14E && i != 0x14F) {
                globalsum += buffer[i];
            }
        }
        buffer[0x14E] = (globalsum >> 8) & 0xFF;
        buffer[0x14F] = globalsum & 0xFF;
    }

    function generatePush(program, stmt) {
        var argument;
        var code = [];
        if(stmt.intermediary) {
            argument = buildArgument(program, stmt.intermediary);
            // 'push x via y' -> 'y = x; push y'
            code.push.apply(code, generateCalculatedAssignment(program, stmt, argument, stmt.src));
        } else {
            argument = buildArgument(program, stmt.src);
        }
        switch(argument.type) {
            case ArgumentType.AF: return code.push(0xF5), code;
            case ArgumentType.BC: return code.push(0xC5), code;
            case ArgumentType.DE: return code.push(0xD5), code;
            case ArgumentType.HL: return code.push(0xE5), code;
            default:
                error("cannot push operand " + argument.toString() + " in 'push' statement", stmt.src.location);
                return [];
        }
    }

    function resolveJumpCondition(program, stmt, result)
    {
        var flag;
        var cond = stmt.condition;
        var attr = cond.attr;
        var negated = cond.negated;
        if(!cond) {
            throw new Error('condition is null');
        }

        if(!attr) {
            switch(cond.branch) {
                case wiz.parse.Branch.Equal:
                    flag = Argument(ArgumentType.Zero);
                    break;
                case wiz.parse.Branch.NotEqual:
                    flag = Argument(ArgumentType.Zero);
                    negated = !negated;
                    break;
                case wiz.parse.Branch.Less:
                    flag = Argument(ArgumentType.Carry);
                    break;
                case wiz.parse.Branch.GreaterEqual:
                    flag = Argument(ArgumentType.Carry);
                    negated = !negated;
                    break;
                case wiz.parse.Branch.Greater:
                case wiz.parse.Branch.LessEqual:
                    error(
                        "comparision " + wiz.parse.getSimpleTokenName(cond.branch)
                        + " unsupported in 'when' clause", cond.location
                    );
                    return false;
            }
        } else {
            var def = wiz.compile.resolveAttribute(program, attr);
            if(wiz.sym.check('Builtin', def)) {
                switch(def.type) {
                    case ArgumentType.Carry:
                    case ArgumentType.Zero:
                        flag = Argument(def.type);
                        break;
                    default:
                }
            }
        }
        if(flag) {
            result.value = flag.getFlagIndex(negated);
            return true;
        } else {
            error(
                "unrecognized condition '" + attr.fullName
                + "' used in 'when' clause", cond.location
            );
            return false;
        }
    }

    function ensureUnconditional(stmt, context, code) {
        if(stmt.condition) {
            error("'when' clause is not allowed for " + context, stmt.condition.location);
            return [];
        } else {
            return code;
        }
    }

    function generateJump(program, stmt) {
        switch(stmt.type) {
            case wiz.parse.Keyword.Goto:
                var argument = buildArgument(program, stmt.destination);
                if(!argument) {
                    return [];
                }
                switch(argument.type) {
                    case ArgumentType.Immediate:
                        if(stmt.far) {
                            var address = {};
                            wiz.compile.foldWord(program, argument.immediate, program.finalized, address);
                            if(stmt.condition) {
                                var index = {};
                                if(resolveJumpCondition(program, stmt, index)) {
                                    return [(0xC2 + index.value * 0x08) & 0xFF, address.value & 0xFF, (address.value >> 8) & 0xFF];
                                }
                            } else {
                                return [0xC3, address.value & 0xFF, (address.value >> 8) & 0xFF];
                            }
                        } else {
                            var description = "relative jump";
                            var bank = program.checkBank(description, stmt.location);
                            var pc = bank.checkAddress(description, stmt.location);
                            var offset = {};
                            wiz.compile.foldRelativeByte(program, stmt.destination,
                                "relative jump distance",
                                "rewrite the branch, shorten the gaps in your code, or add a '!' far indicator.",
                                pc + 2, program.finalized, offset
                            );
                            if(stmt.condition) {
                                var index = {};
                                if(resolveJumpCondition(program, stmt, index)) {
                                    return [(0x20 + index.value * 0x08) & 0xFF, offset.value & 0xFF];
                                }
                            } else {
                                return [0x18, offset.value & 0xFF];
                            }
                        }
                        return [];
                    case ArgumentType.HL:
                        if(!stmt.condition) {
                            return [0xE9];
                        } else {
                            error("'goto hl' does not support 'when' clause", stmt.destination.location);
                        }
                        return [];
                    default:
                        error("unsupported argument to 'goto'", stmt.destination.location);
                        return [];
                }
            case wiz.parse.Keyword.Call:
                var argument = buildArgument(program, stmt.destination);
                if(!argument) {
                    return [];
                }
                switch(argument.type) {
                    case ArgumentType.Immediate:
                        var address = {};
                        wiz.compile.foldWord(program, argument.immediate, program.finalized, address);
                        if(stmt.condition) {
                            var index = {};
                            if(resolveJumpCondition(program, stmt, index)) {
                                return [(0xC4 + index.value * 0x08) & 0xFF, address.value & 0xFF, (address.value >> 8) & 0xFF];
                            }
                        } else {
                            return [0xCD, address.value & 0xFF, (address.value >> 8) & 0xFF];
                        }
                        return [];
                    default:
                        error("unsupported argument to 'call'", stmt.destination.location);
                        return [];
                }
            case wiz.parse.Keyword.Return:
                if(stmt.condition) {
                    var index = {};
                    if(resolveJumpCondition(program, stmt, index))
                    {
                        return [(0xC0 + index.value * 0x08) & 0xFF];
                    }
                } else {
                    return [0xC9];
                }
            case wiz.parse.Keyword.Resume: return ensureUnconditional(stmt, "'resume'", [0xD9]);
            case wiz.parse.Keyword.Abort: return ensureUnconditional(stmt,  "'abort'", [0x40]);
            case wiz.parse.Keyword.Sleep: return ensureUnconditional(stmt,  "'sleep'", [0x76]);
            case wiz.parse.Keyword.Suspend: return ensureUnconditional(stmt,  "'suspend'", [0x10, 0x00]);
            case wiz.parse.Keyword.Nop: return ensureUnconditional(stmt, "'nop'", [0x00]);
            default:
                error("instruction not supported", stmt.destination.location);
                return [];
        }
    }

    function generateComparison(program, stmt) {
        var left = buildArgument(program, stmt.left);
        var right = stmt.right ? buildArgument(program, stmt.right) : null;
        if(!left) {
            return [];
        }
        switch(left.type) {
            case ArgumentType.A:
                if(!right) {
                    // 'compare a' -> 'or a'
                    return [0xB7];
                } else {
                    // 'compare a to expr' -> 'cp a, expr'
                    switch(right.type) {
                        case ArgumentType.Immediate:
                            var value = {};
                            wiz.compile.foldWord(program, right.immediate, program.finalized, value);
                            return [0xFE, value.value & 0xFF];
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            return [(0xB8 + right.getRegisterIndex()) & 0xFF];
                        case ArgumentType.Indirection:
                            if(right.base.type == ArgumentType.HL) {
                                return [0xBE];
                            } else {
                                error("indirected operand in 'to' is not supported (only 'compare a to [hl]' is valid)", stmt.right.location);
                                return [];
                            }
                        case ArgumentType.A: return [0xBF];
                        default:
                            error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                            return [];
                    }
                }
            case ArgumentType.BitIndex:
                // 'compare r@i' -> 'bit r, i'
                if(!right) {
                    var index = {};
                    if(!wiz.compile.foldBitIndex(program, left.immediate, program.finalized, index)) {
                        return [];
                    }
                    left = left.base;
                    switch(left.type) {
                        case ArgumentType.A: break;
                        case ArgumentType.B: break;
                        case ArgumentType.C: break;
                        case ArgumentType.D: break;
                        case ArgumentType.E: break;
                        case ArgumentType.H: break;
                        case ArgumentType.L: break;
                        case ArgumentType.Indirection:
                            if(left.base.type != ArgumentType.HL) {
                                error("indirected operand on left-hand side of '@' is not supported (only '[hl] @ ...' is valid)", stmt.right.location);
                                return [];
                            }
                            break;
                        default:
                            error("unsupported operand on left-hand side of '@'", stmt.right.location);
                            return [];
                    }
                    return [0xCB, (0x40 + index.value * 0x08 + left.getRegisterIndex()) & 0xFF];
                } else {
                    error("'to' clause is unsupported for 'compare ... @ ...'", stmt.right.location);
                    return [];
                }
            default:
                return [];
        }
    }

    function generateAssignment(program, stmt) {
        if(!stmt.src) {
            return generatePostfixAssignment(program, stmt);
        } else {
            var dest = buildArgument(program, stmt.dest);
            if(stmt.intermediary) {
                var intermediary = buildArgument(program, stmt.intermediary);
                // 'x = y via z' -> 'z = y; x = z'
                var result = generateCalculatedAssignment(program, stmt, intermediary, stmt.src);
                return result.push.apply(result, generateCalculatedAssignment(program, stmt, dest, stmt.intermediary)), result;
            } else {
                return generateCalculatedAssignment(program, stmt, dest, stmt.src);
            }
        }
    }

    function generatePostfixAssignment(program, stmt) {
        var dest = buildArgument(program, stmt.dest);
        var operatorIndex, operatorName;
        switch(stmt.postfix) {
            case wiz.parse.Postfix.Inc: operatorIndex = 0; operatorName = "'++'"; break;
            case wiz.parse.Postfix.Dec: operatorIndex = 1; operatorName = "'--'"; break;
        }

        if(!dest) {
            return [];
        }
        switch(dest.type) {
            case ArgumentType.A:
            case ArgumentType.B:
            case ArgumentType.C:
            case ArgumentType.D:
            case ArgumentType.E:
            case ArgumentType.H:
            case ArgumentType.L:
                return [(0x04 + operatorIndex + dest.getRegisterIndex() * 0x08) & 0xFF];
            case ArgumentType.Indirection:
                if(dest.base.type == ArgumentType.HL) {
                    return [(0x04 + operatorIndex + dest.getRegisterIndex() * 0x08) & 0xFF];
                } else {
                    error(stmt.dest.toString() + " cannot be operand of " + operatorName, stmt.dest.location);
                    return [];
                }
            case ArgumentType.BC:
            case ArgumentType.DE:
            case ArgumentType.HL:
            case ArgumentType.SP:
                return [(0x03 + (operatorIndex * 0x08) + dest.getPairIndex() * 0x10) & 0xFF];
            default:
                error(stmt.dest.toString() + " cannot be operand of " + operatorName, stmt.dest.location);
                return [];
        }
    }

    function generateCalculatedAssignment(program, stmt, dest, src)
    {
        if(!dest) {
            return [];
        }

        var infix = wiz.ast.check('Infix', src);
        if(infix) {
            var result = {};
            if(!wiz.compile.tryFoldConstant(program, infix, false, program.finalized, result)) {
                return [];
            }
            var constTail = result.constTail;
            var loadsrc = null;
            if(!constTail) {
                loadsrc = infix.operands[0];
            } else {
                loadsrc = wiz.ast.Number(wiz.parse.Token.Integer, result.value, constTail.location);
            }
            var code = getLoad(program, stmt, dest, loadsrc);
            var found = !constTail || constTail == infix.operands[0];
            for(var i = 0; i < infix.types.length; i++) {
                var type = infix.types[i];
                var node = infix.operands[i + 1];
                if(node == constTail) {
                    found = true;
                } else if(found) {
                    var operand = buildArgument(program, node);
                    if(!operand) {
                        return [];
                    }
                    if(i == 0 && patchStackPointerLoadOffset(program, type, node, dest, operand, loadsrc, code)) {
                        continue;
                    }
                    code.push.apply(code, getModify(program, type, node, dest, operand));
                }
            }
            return code;
        } else {
            return getLoad(program, stmt, dest, src);
        }
    }

    function patchStackPointerLoadOffset(program, type, node, dest, operand, loadsrc, code) {
        if(dest.type != ArgumentType.HL) {
            return false;
        }
        switch(type) {
            case wiz.parse.Infix.Add: break;
            case wiz.parse.Infix.Sub: break;
            default: return false;
        }
        switch(operand.type) {
            case ArgumentType.Immediate:
                var load = buildArgument(program, loadsrc);
                if(!load || load.type != ArgumentType.SP) {
                    return false;
                }

                var value = {};
                wiz.compile.foldSignedByte(program, operand.immediate, type == wiz.parse.Infix.Sub, program.finalized, value);
                // Monkey patch 'hl = sp + 00'
                code[code.length - 1] = value.value & 0xFF;
                return true;
            default:
                return false;
        }
    }

    function operatorError(type, dest, location) {
        error("infix operator " + wiz.parse.getSimpleTokenName(type) + " cannot be used in assignment '=' to " + dest.toString() + ".", location);
        return [];
    }

    function operandError(type, dest, operand, location) {
        error(operand.toString() + " cannot be operand of " + wiz.parse.getSimpleTokenName(type) + " in assignment '=' to " + dest.toString() + ".", location);
        return [];
    }

    function getModify(program, type, node, dest, operand) {
        switch(dest.type) {
            case ArgumentType.A:
                switch(type) {
                    case wiz.parse.Infix.Add:
                    case wiz.parse.Infix.AddC:
                    case wiz.parse.Infix.Sub:
                    case wiz.parse.Infix.SubC:
                    case wiz.parse.Infix.And:
                    case wiz.parse.Infix.Xor:
                    case wiz.parse.Infix.Or:
                        return getAccumulatorArithmetic(program, type, node, dest, operand);
                    case wiz.parse.Infix.ShiftL:
                    case wiz.parse.Infix.ShiftR:
                    case wiz.parse.Infix.ArithShiftL:
                    case wiz.parse.Infix.ArithShiftR:
                        return getRegisterShift(program, type, node, dest, operand);
                    case wiz.parse.Infix.RotateL:
                    case wiz.parse.Infix.RotateR:
                    case wiz.parse.Infix.RotateLC:
                    case wiz.parse.Infix.RotateRC:
                        return getAccumulatorShift(program, type, node, dest, operand);
                    default:
                        return operatorError(type, dest, node.location);
                }
            case ArgumentType.B:
            case ArgumentType.C:
            case ArgumentType.D:
            case ArgumentType.E:
            case ArgumentType.H:
            case ArgumentType.L:
                switch(type) {
                    case wiz.parse.Infix.RotateLC:
                    case wiz.parse.Infix.RotateRC:
                    case wiz.parse.Infix.RotateL:
                    case wiz.parse.Infix.RotateR:
                    case wiz.parse.Infix.ShiftL:
                    case wiz.parse.Infix.ShiftR:
                    case wiz.parse.Infix.ArithShiftL:
                    case wiz.parse.Infix.ArithShiftR:
                        return getRegisterShift(program, type, node, dest, operand);
                    default:
                       return operatorError(type, dest, node.location);
                }
            case ArgumentType.Indirection:
                switch(dest.base.type) {
                    // 'r = [hl]' -> 'ld r, [hl]'
                    case ArgumentType.HL:
                        switch(type) {
                            case wiz.parse.Infix.RotateLC:
                            case wiz.parse.Infix.RotateRC:
                            case wiz.parse.Infix.RotateL:
                            case wiz.parse.Infix.RotateR:
                            case wiz.parse.Infix.ShiftL:
                            case wiz.parse.Infix.ShiftR:
                            case wiz.parse.Infix.ArithShiftL:
                            case wiz.parse.Infix.ArithShiftR:
                                return getRegisterShift(program, type, node, dest, operand);
                            default:
                                return operatorError(type, dest, node.location);
                        }
                    default:
                        return operatorError(type, dest, node.location);
                }
            case ArgumentType.BC:
            case ArgumentType.DE:
                switch(type) {
                    case wiz.parse.Infix.ShiftL:
                    case wiz.parse.Infix.ShiftR:
                        return getPairShift(program, type, node, dest, operand);
                    default:
                        return operatorError(type, dest, node.location);
                }
            case ArgumentType.HL:
                switch(type) {
                    case wiz.parse.Infix.Add:
                        if(operand.type == ArgumentType.Immediate) {
                            return operandError(type, dest, operand, node.location);
                        } else {
                            return getHighLowArithmetic(program, type, node, dest, operand);
                        }
                    case wiz.parse.Infix.Sub:
                        return operatorError(type, dest, node.location);
                    case wiz.parse.Infix.ShiftL:
                    case wiz.parse.Infix.ShiftR:
                        return getPairShift(program, type, node, dest, operand);
                    default:
                        return operatorError(type, dest, node.location);
                }
            case ArgumentType.SP:
                switch(type) {
                    case wiz.parse.Infix.Add:
                    case wiz.parse.Infix.Sub:
                        return getStackPointerArithmetic(program, type, node, dest, operand);
                    default:
                        return operatorError(type, dest, node.location);
                }
            case ArgumentType.Carry:
                switch(type) {
                    case wiz.parse.Infix.Xor:
                        throw new Error('todo'); // TODO
                    default:
                        return operatorError(type, dest, node.location);
                }
                return [];
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function getAccumulatorArithmetic(program, type, node, dest, operand) {
        var operatorIndex;
        switch(type) {
            case wiz.parse.Infix.Add: operatorIndex = 0; break;
            case wiz.parse.Infix.AddC: operatorIndex = 1; break;
            case wiz.parse.Infix.Sub: operatorIndex = 2; break;
            case wiz.parse.Infix.SubC: operatorIndex = 3; break;
            case wiz.parse.Infix.And: operatorIndex = 4; break;
            case wiz.parse.Infix.Xor: operatorIndex = 5; break;
            case wiz.parse.Infix.Or: operatorIndex = 6; break;
        }
        switch(operand.type) {
            case ArgumentType.Immediate:
                var value = {};
                wiz.compile.foldByte(program, operand.immediate, program.finalized, value);
                return [(0xC6 + operatorIndex * 0x08) & 0xFF, value.value & 0xFF];
            case ArgumentType.A:
            case ArgumentType.B:
            case ArgumentType.C:
            case ArgumentType.D:
            case ArgumentType.E:
            case ArgumentType.H:
            case ArgumentType.L:
                return [(0x80 + operatorIndex * 0x08 + operand.getRegisterIndex()) & 0xFF];
            case ArgumentType.Indirection:
                switch(operand.base.type) {
                    // 'r = [hl]' -> 'ld r, [hl]'
                    case ArgumentType.HL:
                        return [(0x80 + operatorIndex * 0x08 + operand.getRegisterIndex()) & 0xFF];
                        break;
                    default:
                        return operandError(type, dest, operand, node.location);
                }
                break;
            default:
                return operandError(type, dest, operand, node.location);
        }
    }


    function getHighLowArithmetic(program, type, node, dest, operand) {
        switch(operand.type) {
            case ArgumentType.BC:
            case ArgumentType.DE:
            case ArgumentType.HL:
            case ArgumentType.SP:
                return [(0x09 + operand.getPairIndex() * 0x10) & 0xFF];
                break;
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function getStackPointerArithmetic(program, type, node, dest, operand) {
        switch(operand.type) {
            case ArgumentType.Immediate:
                var value = {};
                wiz.compile.foldSignedByte(program, operand.immediate, type == wiz.parse.Infix.Sub, program.finalized, value);
                return [0xE8, value.value & 0xFF];
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function getAccumulatorShift(program, type, node, dest, operand) {
        var operatorIndex;
        switch(type) {
            case wiz.parse.Infix.RotateLC: operatorIndex = 0; break;
            case wiz.parse.Infix.RotateRC: operatorIndex = 1; break;
            case wiz.parse.Infix.RotateL: operatorIndex = 2; break;
            case wiz.parse.Infix.RotateR: operatorIndex = 3; break;
        }
        switch(operand.type) {
            case ArgumentType.Immediate:
                var value = {};
                if(!wiz.compile.foldBitIndex(program, operand.immediate, program.finalized, value)) {
                    return [];
                }
                var code = [];
                while(value.value--) {
                    code.push((0x07 + operatorIndex * 0x08) & 0xFF);
                }
                return code;
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function getRegisterShiftIndex(type) {
        switch(type) {
            case wiz.parse.Infix.RotateLC: return 0;
            case wiz.parse.Infix.RotateRC: return 1;
            case wiz.parse.Infix.RotateL: return 2;
            case wiz.parse.Infix.RotateR: return 3;
            case wiz.parse.Infix.ArithShiftL: return 4;
            case wiz.parse.Infix.ArithShiftR: return 5;
            case wiz.parse.Infix.ShiftL: return 4; // Logical shl == arith shl
            case wiz.parse.Infix.ShiftR: return 7;
        };
    }

    function getRegisterShift(program, type, node, dest, operand) {
        var operatorIndex = getRegisterShiftIndex(type);
        switch(operand.type) {
            case ArgumentType.Immediate:
                var value = {};
                if(!wiz.compile.foldBitIndex(program, operand.immediate, program.finalized, value)) {
                    return [];
                }
                var code = [];
                while(value.value--) {
                    code.push(0xCB, (operatorIndex * 0x08 + dest.getRegisterIndex()) & 0xFF);
                }
                return code;
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function getPairShift(program, type, node, dest, operand) {
        var shift;

        switch(type) {
            case wiz.parse.Infix.ShiftL:
                shift = [
                    // sla l
                    0xCB, (getRegisterShiftIndex(wiz.parse.Infix.ShiftL) * 0x08 + dest.getPairLowIndex()) & 0xFF,
                    // rl h
                    0xCB, (getRegisterShiftIndex(wiz.parse.Infix.RotateL) * 0x08 + dest.getPairHighIndex()) & 0xFF,
                ];
                break;
            case wiz.parse.Infix.ShiftR:
                shift = [
                    // sra h
                    0xCB, (getRegisterShiftIndex(wiz.parse.Infix.ShiftR) * 0x08 + dest.getPairHighIndex()) & 0xFF,
                    // rr l
                    0xCB, (getRegisterShiftIndex(wiz.parse.Infix.RotateR) * 0x08 + dest.getPairLowIndex()) & 0xFF,
                ];
                break;
        }
        switch(operand.type) {
            case ArgumentType.Immediate:
                var value = {};
                if(!wiz.compile.foldWordBitIndex(program, operand.immediate, program.finalized, value)) {
                    return [];
                }
                var code = [];
                while(value.value--) {
                    code.push.apply(code, shift);
                }
                return code;
            default:
                return operandError(type, dest, operand, node.location);
        }
    }

    function invalidAssignmentDestError(dest, location) {
        error("assignment '=' to " + dest.toString() + " is invalid.", location);
        return [];
    }

    function invalidAssignmentError(dest, load, location) {
        error("invalid assignment '=' of " + dest.toString() + " to " + load.toString() + ".", location);
        return [];
    }

    function getLoad(program, stmt, dest, loadsrc) {
        var load = buildArgument(program, loadsrc);
        if(load) {
            return getPrefixLoad(program, stmt, dest, load);
        } else {
            return [];
        }
    }

    function getPrefixLoad(program, stmt, dest, load) {
        switch(load.type) {
            // 'r = <>r' -> 'swap r'
            case ArgumentType.Swap:
                switch(dest.type) {
                    case ArgumentType.A:
                    case ArgumentType.B:
                    case ArgumentType.C:
                    case ArgumentType.D:
                    case ArgumentType.E:
                    case ArgumentType.H:
                    case ArgumentType.L:
                            var result = getPrefixLoad(program, stmt, dest, load.base);
                            return result.push(0xCB, (0x30 + dest.getRegisterIndex()) & 0xFF), result;
                    case ArgumentType.Indirection:
                        if(dest.base.type == ArgumentType.HL) {
                            var result = getPrefixLoad(program, stmt, dest, load.base);
                            return result.push(0xCB, (0x30 + dest.getRegisterIndex()) & 0xFF), result;
                        }
                    default:
                        return [];
                }
            // 'a = ~a' -> 'cpl a'
            case ArgumentType.Not:
                if(dest.type == ArgumentType.A) {
                    var result = getPrefixLoad(program, stmt, dest, load.base);
                    return result.push(0x2F), result;
                } else if(dest.type == ArgumentType.Carry) {
                    var result = getPrefixLoad(program, stmt, dest, load.base)
                    return result.push(0x3F), result;
                } else {
                    return invalidAssignmentError(dest, load, stmt.location);
                }
            // 'a = -a' -> 'cpl a, inc a'
            // 'carry = ~carry' -> 'ccf'
            case ArgumentType.Negated:
                if(dest.type == ArgumentType.A) {
                    var result = getPrefixLoad(program, stmt, dest, load.base);
                    return result.push(0x2F, 0x3C), result;
                } else {
                    return invalidAssignmentError(dest, load, stmt.location);
                }
            default:
                return getBaseLoad(program, stmt, dest, load);
        }
    }

    function getBaseLoad(program, stmt, dest, load) {
        switch(dest.type) {
            case ArgumentType.A:
                switch(load.type) {
                    // 'a = n' -> 'ld a, n'
                    case ArgumentType.Immediate:
                        return getRegisterLoadImmediate(program, stmt, dest, load);
                    // 'a = a' -> (nothing)
                    // 'a = r' -> 'ld a, r'
                    case ArgumentType.A:
                    case ArgumentType.B:
                    case ArgumentType.C:
                    case ArgumentType.D:
                    case ArgumentType.E:
                    case ArgumentType.H:
                    case ArgumentType.L:
                        return getRegisterLoadRegister(program, stmt, dest, load);
                    case ArgumentType.Indirection:
                        switch(load.base.type) {
                            case ArgumentType.Immediate:
                                return getAccumulatorLoadIndirectImmediate(program, stmt, load);
                            // 'a = [bc]' -> 'ld a, [bc]'
                            // 'a = [de]' -> 'ld a, [de]'
                            case ArgumentType.BC:
                            case ArgumentType.DE:
                                return getAccumulatorLoadIndirectPair(program, stmt, load);
                            // 'a = [hl]' -> 'ld a, [hl]'
                            case ArgumentType.HL: 
                                return getRegisterLoadRegister(program, stmt, dest, load);
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    case ArgumentType.IndirectionInc:
                    case ArgumentType.IndirectionDec:
                        switch(load.base.type) {
                            // 'a = [hl++]' -> 'ldi a, [hl]',
                            // 'a = [hl--]' -> 'ldd a, [hl]',
                            case ArgumentType.HL:
                                return getAccumulatorLoadIndirectIncrement(program, stmt, load);
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    case ArgumentType.PositiveIndex:
                        var value = {};
                        wiz.compile.foldWord(program, load.immediate, program.finalized, value);

                        if(value.value != 0xFF00 || load.base.type != ArgumentType.C) {
                            return invalidAssignmentError(dest, load, stmt.location);
                        } else {
                            return [0xF2];
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.B:
            case ArgumentType.C:
            case ArgumentType.D:
            case ArgumentType.E:
            case ArgumentType.H:
            case ArgumentType.L:
                switch(load.type) {
                    // 'r = n' -> 'ld r, n'
                    case ArgumentType.Immediate:
                        return getRegisterLoadImmediate(program, stmt, dest, load);
                    // 'r = r' -> (nothing)
                    // 'r = r2' -> 'ld r, r2'
                    case ArgumentType.A: 
                    case ArgumentType.B:
                    case ArgumentType.C:
                    case ArgumentType.D:
                    case ArgumentType.E:
                    case ArgumentType.H:
                    case ArgumentType.L:
                        return getRegisterLoadRegister(program, stmt, dest, load);
                    case ArgumentType.Indirection:
                        switch(load.base.type) {
                            // 'r = [hl]' -> 'ld r, [hl]'
                            case ArgumentType.HL:
                                return getRegisterLoadRegister(program, stmt, dest, load);
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.Indirection:
                switch(dest.base.type) {
                    case ArgumentType.Immediate:
                        if(load.type == ArgumentType.A) {
                            return getIndirectImmediateLoadAccumulator(program, stmt, dest);
                        } else {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    // '[bc] = a' -> 'ld [bc], a'
                    // '[de] = a' -> 'ld [de], a'
                    case ArgumentType.BC:
                    case ArgumentType.DE:
                        if(load.type == ArgumentType.A) {
                            return getIndirectPairLoadAccumulator(program, stmt, dest);
                        } else {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    case ArgumentType.HL:
                        switch(load.type)
                        {
                            // '[hl] = n' -> 'ld [hl], n'
                            case ArgumentType.Immediate:
                                return getRegisterLoadImmediate(program, stmt, dest, load);
                            // '[hl] = r' -> 'ld [hl], r'
                            case ArgumentType.A: 
                            case ArgumentType.B:
                            case ArgumentType.C:
                            case ArgumentType.D:
                            case ArgumentType.E:
                            case ArgumentType.H:
                            case ArgumentType.L:
                                return getRegisterLoadRegister(program, stmt, dest, load);
                            case ArgumentType.Indirection:
                                switch(load.base.type) {
                                    // '[hl] = [hl]' -> (nothing)
                                    case ArgumentType.HL:
                                        return [];
                                    default:
                                        return invalidAssignmentError(dest, load, stmt.location);
                                }
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.AF:
                switch(load.type) {
                    // 'af = pop' -> 'pop af'
                    case ArgumentType.Pop:
                        return getPairLoadPop(program, stmt, dest);
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.BC:
            case ArgumentType.DE:
            case ArgumentType.HL:
                switch(load.type) {
                    // 'rr = n' -> 'ld rr, n'
                    case ArgumentType.Immediate:
                        return getPairLoadImmediate(program, stmt, dest, load);
                    // 'rr = pop' -> 'pop rr'
                    case ArgumentType.Pop:
                        return getPairLoadPop(program, stmt, dest);
                    // 'hl = sp' -> 'ld hl, sp+0x0000'
                    case ArgumentType.SP:
                        if(dest.type == ArgumentType.HL) {
                            // 'hl = sp' -> 'ldhl sp,0'
                            return [0xF8, 0x00];
                        } else {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    // 'rr = rr' -> (none)
                    // 'hl = rr' -> 'ld hl, 0x0000; add hl, rr'
                    case ArgumentType.BC:
                    case ArgumentType.DE:
                    case ArgumentType.HL:
                        if(dest.type != load.type) {
                            if(dest.type == ArgumentType.HL) {
                                return getHighLowLoadPair(program, stmt, load);
                            } else {
                                return invalidAssignmentError(dest, load, stmt.location);
                            }
                        } else {
                            return [];
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.SP:
                switch(load.type) {
                    // 'sp = n' -> 'ld sp, n'
                    case ArgumentType.Immediate:
                        return getPairLoadImmediate(program, stmt, dest, load);
                    // sp = sp -> (none)
                    case ArgumentType.SP:
                        return [];
                    // 'sp = hl' -> 'ld sp, hl'
                    case ArgumentType.HL:
                        return [0xF9];
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.IndirectionInc:
            case ArgumentType.IndirectionDec:
                switch(load.type) {
                    case ArgumentType.A:
                        // '[hl++] = a' -> 'ld [hl+], a'
                        return getIndirectIncrementLoadAccumulator(program, stmt, dest);
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.PositiveIndex:
                var value = {};
                wiz.compile.foldWord(program, dest.immediate, program.finalized, value);

                if(value.value != 0xFF00 || dest.base.type != ArgumentType.C) {
                    error("assignment '=' to indexed memory location other than '[0xFF00:c]' is invalid.", stmt.location);
                    return [];
                }
                switch(load.type) {
                    // '[FF00:c]' -> 'ldh [c], a'
                    case ArgumentType.A:
                        return [0xE2];
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.BitIndex:
                var index = {};
                if(!wiz.compile.foldBitIndex(program, dest.immediate, program.finalized, index)) {
                    return [];
                }
                switch(dest.base.type) {
                    case ArgumentType.A: break;
                    case ArgumentType.B: break;
                    case ArgumentType.C: break;
                    case ArgumentType.D: break;
                    case ArgumentType.E: break;
                    case ArgumentType.H: break;
                    case ArgumentType.L: break;
                    case ArgumentType.Indirection:
                        if(dest.base.type != ArgumentType.HL) {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                        break;
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
                switch(load.type) {
                    // 'r@i = 0' -> 'res r, i'
                    // 'r@i = 1' -> 'set r, i'
                    case ArgumentType.Immediate:
                        var value = {};
                        if(!wiz.compile.foldBit(program, load.immediate, program.finalized, value)) {
                            return [];
                        }
                        if(value.value == 0) {
                            return [0xCB, (0x80 + index.value * 0x08 + dest.base.getRegisterIndex()) & 0xFF];
                        } else {
                            return [0xCB, (0xC0 + index.value * 0x08 + dest.base.getRegisterIndex()) & 0xFF];
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.Interrupt:
                switch(load.type) {
                    // 'interrupt = 0' -> 'di'
                    // 'interrupt = 1' -> 'ei'
                    case ArgumentType.Immediate:
                        var value = {};
                        if(!wiz.compile.foldBit(program, load.immediate, program.finalized, value)) {
                            return [];
                        }
                        if(value.value == 0) {
                            return [0xF3];
                        } else {
                            return [0xFB];
                        }
                    default:
                        return invalidAssignmentError(dest, load, stmt.location);
                }
            case ArgumentType.Pop:
            case ArgumentType.Immediate:
            case ArgumentType.Not:
            case ArgumentType.Negated:
            case ArgumentType.Swap:
            case ArgumentType.F:
            case ArgumentType.Zero:
            case ArgumentType.NegativeIndex:
                return invalidAssignmentDestError(dest, stmt.location);
            case ArgumentType.Carry:
                switch(load.type) {
                    case ArgumentType.Immediate:
                        var value = {};
                        if(!wiz.compile.foldBit(program, load.immediate, program.finalized, value)) {
                            return [];
                        }
                        // 'carry = 0' -> 'scf; ccf'    
                        if(value.value == 0) {
                            return [0x37, 0x3F];
                        }
                        // 'carry = 1' -> 'scf'
                        else {
                            return [0x37];
                        }
                    // 'carry = carry' -> (none)
                    case ArgumentType.Carry:
                        return [];
                    default:
                        return invalidAssignmentDestError(dest, stmt.location);
                }
                break;
            case ArgumentType.None:
                throw new Error("load must have argument");
        }
    }

    function getRegisterLoadImmediate(program, stmt, dest, load) {
        var value = {};
        wiz.compile.foldByte(program, load.immediate, program.finalized, value);
        return [(0x06 + dest.getRegisterIndex() * 0x08) & 0xFF, value.value & 0xFF];
    }

    function getRegisterLoadRegister(program, stmt, dest, load) {
        // ld r, r2, where r != r2 (we treat load self as empty code)
        if(dest.getRegisterIndex() != load.getRegisterIndex()) {
            return [(0x40 + dest.getRegisterIndex() * 0x08 + load.getRegisterIndex()) & 0xFF];
        }
        return [];
    }

    function getPairLoadImmediate(program, stmt, dest, load) {
        var value = {};
        wiz.compile.foldWord(program, load.immediate, program.finalized, value);
        return [(0x01 + dest.getPairIndex() * 0x10) & 0xFF, value.value & 0xFF, (value.value >> 8) & 0xFF];
    }

    function getPairLoadPop(program, stmt, dest) {
        return [(0xC1 + dest.getPairIndex() * 0x10) & 0xFF];
    }

    function getHighLowLoadPair(program, stmt, load) {
        return [0x21, 0x00, 0x00, (0x09 + load.getPairIndex() * 0x10) & 0xFF];
    }

    function getAccumulatorLoadIndirectImmediate(program, stmt, load) {
        var value = {};
        wiz.compile.foldWord(program, load.base.immediate, program.finalized, value);
        // 'a = [0xFFnn]' -> 'ldh a, [nn]'
        if((value.value & 0xFF00) == 0xFF00) {
            return [0xF0, value.value & 0xFF];
        }
        // 'a = [nnnn]' -> 'ld a, [nnnn]'
        else {
            return [0xFA, value.value & 0xFF, (value.value >> 8) & 0xFF];
        }
    }

    function getIndirectImmediateLoadAccumulator(program, stmt, dest) {
        var value = {};
        wiz.compile.foldWord(program, dest.base.immediate, program.finalized, value);
        // '[0xFFnn] = a' -> 'ldh [nn], a'
        if((value.value & 0xFF00) == 0xFF00) {
            return [0xE0, value.value & 0xFF];
        }
        // '[nnnn] = a' -> 'ld [nnnn], a'
        else {
            return [0xEA, value.value & 0xFF, (value.value >> 8) & 0xFF];
        }
    }

    function getAccumulatorLoadIndirectPair(program, stmt, load) {
        return [(0x0A + load.base.getPairIndex() * 0x10) & 0xFF];
    }

    function getIndirectPairLoadAccumulator(program, stmt, dest) {
        return [(0x02 + dest.base.getPairIndex() * 0x10) & 0xFF];
    }

    function getAccumulatorLoadIndirectIncrement(program, stmt, load) {
        switch(load.type) {
            case ArgumentType.IndirectionInc: return [0x2A];
            case ArgumentType.IndirectionDec: return [0x3A];
        }
    }

    function getIndirectIncrementLoadAccumulator(program, stmt, dest) {
        switch(dest.type) {
            case ArgumentType.IndirectionInc: return [0x22];
            case ArgumentType.IndirectionDec: return [0x32];
        }
    }

    return that;
}

})(this);