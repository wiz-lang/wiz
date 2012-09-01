module wiz.compile.compilation;

import wiz.lib;
import wiz.compile.lib;

static import std.stdio;

sym.Definition resolveAttribute(Program program, ast.Attribute attribute)
{
    sym.Definition prev, def;
    string[] partialQualifiers;
    auto env = program.environment;
    
    foreach(i, piece; attribute.pieces)
    {
        partialQualifiers ~= piece;
        
        if(prev is null)
        {
            def = env.get!(sym.Definition)(piece);
        }
        else
        {
            sym.PackageDef pkg = cast(sym.PackageDef) prev;
            if(pkg is null)
            {
                string previousName = std.string.join(partialQualifiers[0 .. partialQualifiers.length - 1], ".");
                error("attempt to get symbol '" ~ attribute.fullName() ~ "', but '" ~ previousName ~ "' is not a package", attribute.location);
            }
            else
            {
                env = pkg.environment;
                def = env.get!(sym.Definition)(piece, true); // qualified lookup is shallow.
            }
        }
        
        if(def is null)
        {
            string partiallyQualifiedName = std.string.join(partialQualifiers, ".");
            string fullyQualifiedName = attribute.fullName();
            
            if(partiallyQualifiedName == fullyQualifiedName)
            {
                error("reference to undeclared symbol '" ~ partiallyQualifiedName ~ "'", attribute.location);
            }
            else
            {
                error("reference to undeclared symbol '" ~ partiallyQualifiedName ~ "' (in '" ~ fullyQualifiedName ~ "')", attribute.location);
            }
            return null;
        }
        
        prev = def;
    }
    return def;
}

bool tryFoldConstant(Program program, ast.Expression root, bool runtimeForbidden, bool finalized, ref uint result, ref ast.Expression constTail)
{
    uint[ast.Expression] values;
    bool[ast.Expression] completeness;
    bool runtimeRootForbidden = runtimeForbidden;
    bool badAttr = false;
    uint depth = 0;

    void updateValue(ast.Expression node, uint value, bool complete=true)
    {
        values[node] = value;
        completeness[node] = complete;
        if(depth == 0 && complete)
        {
            constTail = node;
        }
    }

    constTail = null;
    traverse(root,
        (ast.Infix e)
        {
            auto first = e.operands[0];
            if((first in values) is null)
            {
                if(depth == 0)
                {
                    constTail = null;
                }
                return;
            }

            uint a = values[first];
            updateValue(e, a, false);

            foreach(i, type; e.types)
            {
                auto operand = e.operands[i + 1];
                if((operand in values) is null)
                {
                    if(depth == 0)
                    {
                        constTail = e.operands[i];
                    }
                    return;
                }
                uint b = values[operand];

                final switch(type)
                {
                    case parse.Infix.Add: 
                        if(a + ast.Expression.Max < b)
                        {
                            error("addition yields result which will overflow outside of 0..65535.", operand.location);
                            return;
                        }
                        else
                        {
                            a += b;
                        }
                        break;
                    case parse.Infix.Sub:
                        if(a < b)
                        {
                            error("subtraction yields result which will overflow outside of 0..65535.", operand.location);
                            return;
                        }
                        else
                        {
                            a -= b;
                        }
                        break;
                    case parse.Infix.Mul:
                        if(a > ast.Expression.Max / b)
                        {
                            error("multiplication yields result which will overflow outside of 0..65535.", operand.location);
                            return;
                        }
                        else
                        {
                            a *= b;
                        }
                        break;
                    case parse.Infix.Div:
                        if(a == 0)
                        {
                            error("division by zero is undefined.", operand.location);
                            return;
                        }
                        else
                        {
                            a /= b;
                        }
                        break;
                    case parse.Infix.Mod:
                        if(a == 0)
                        {
                            error("modulo by zero is undefined.", operand.location);
                            return;
                        }
                        else
                        {
                            a %= b;
                        }
                        break;
                    case parse.Infix.ShiftL:
                        // If shifting more than N bits, or ls << rs > 2^N-1, then error.
                        if(b > 16 || (b > 0 && (a & ~(1 << (16 - b))) != 0))
                        {
                            error("logical shift left yields result which will overflow outside of 0..65535.", operand.location);
                            return;
                        }
                        else
                        {
                            a <<= b;
                        }
                        break;
                    case parse.Infix.ShiftR:
                        a >>= b;
                        break;
                    case parse.Infix.And:
                        a &= b;
                        break;
                    case parse.Infix.Or:
                        a |= b;
                        break;
                    case parse.Infix.Xor:
                        a ^= b;
                        break;
                    case parse.Infix.At:
                        a &= (1 << b);
                        break;
                    case parse.Infix.AddC, parse.Infix.SubC,
                        parse.Infix.ArithShiftL, parse.Infix.ArithShiftR,
                        parse.Infix.RotateL, parse.Infix.RotateR,
                        parse.Infix.RotateLC, parse.Infix.RotateRC,
                        parse.Infix.Colon:
                        if(runtimeForbidden)
                        {
                            error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in constant expression", operand.location);
                        }
                        return;
                }

                updateValue(e, a, i == e.types.length - 1);
            }
        },

        (Visitor.Pass pass, ast.Prefix e)
        {
            if(pass == Visitor.Pass.Before)
            {
                if(e.type == parse.Prefix.Grouping)
                {
                    depth++;
                    runtimeForbidden = false;
                }
                else if(e.type == parse.Prefix.Indirection)
                {
                    depth++;
                }
            }
            else
            {
                final switch(e.type)
                {
                    case parse.Prefix.Low, parse.Prefix.High, parse.Prefix.Swap:
                        break;
                    case parse.Prefix.Grouping:
                        depth--;
                        if(depth == 0)
                        {
                            runtimeForbidden = runtimeRootForbidden;
                        }
                        break;
                    case parse.Prefix.Not, parse.Prefix.Sub:
                        if(runtimeForbidden)
                        {
                            error("prefix operator " ~ parse.getPrefixName(e.type) ~ " cannot be used in constant expression", e.location);
                        }
                        return;
                    case parse.Prefix.Indirection:
                        depth--;
                        if(runtimeForbidden)
                        {
                            error("indirection operator cannot be used in constant expression", e.location);
                        }
                        return;
                }
                
                if((e.operand in values) is null)
                {
                    return;
                }
                uint r = values[e.operand];
                final switch(e.type)
                {
                    case parse.Prefix.Low:
                        updateValue(e, r & 0xFF);
                        break;
                    case parse.Prefix.High:
                        updateValue(e, (r >> 8) & 0xFF);
                        break;
                    case parse.Prefix.Swap:
                        updateValue(e, ((r & 0x0F0F) << 4) | ((r & 0xF0F0) >> 4));
                        break;
                    case parse.Prefix.Grouping:
                        updateValue(e, r);
                        break;
                    case parse.Prefix.Not, parse.Prefix.Indirection, parse.Prefix.Sub:
                        break;
                }
            }
        },

        (ast.Postfix e)
        {
            if((e.operand in values) is null)
            {
                return;
            }
            else
            {
                if(runtimeForbidden)
                {
                    error("postfix operator " ~ parse.getPostfixName(e.type) ~ " cannot be used in constant expression", e.location);
                }
            }
        },

        (ast.Attribute a)
        {
            auto def = resolveAttribute(program, a);
            if(!def)
            {
                badAttr = true;
                return;
            }
            if(auto constdef = cast(sym.ConstDef) def)
            {
                program.enterEnvironment(constdef.decl, constdef.environment);
                uint v;
                bool folded = foldConstant(program, (cast(ast.ConstDecl) constdef.decl).value, program.finalized, v);
                program.leaveEnvironment();
                if(folded)
                {
                    updateValue(a, v);
                    return;
                }
            }
            else if(auto vardef = cast(sym.VarDef) def)
            {
                uint v;
                if(vardef.hasAddress)
                {
                    updateValue(a, vardef.address);
                    return;
                }
            }
            else if(auto labeldef = cast(sym.LabelDef) def)
            {
                uint v;
                if(labeldef.hasAddress)
                {
                    updateValue(a, labeldef.address);
                    return;
                }
            }

            if(def && finalized)
            {
                error("'" ~ a.fullName() ~ "' was declared, but could not be evaluated.", a.location);
            }
        },

        (ast.String s)
        {
            error("string literal is not allowed here", s.location);
        },

        (ast.Number n)
        {
            updateValue(n, n.value);
        },
    );

    result = values.get(root, 0);
    if(!completeness.get(root, false) && runtimeForbidden && finalized)
    {
        error("expression could not be resolved as a constant", root.location);
        constTail = null;
        return false;
    }
    if(badAttr)
    {
        constTail = null;
        return false;
    }
    return true;
}

bool foldConstant(Program program, ast.Expression root, bool finalized, ref uint result)
{
    ast.Expression constTail;
    return tryFoldConstant(program, root, true, finalized, result, constTail) && constTail == root;
}

bool foldBoundedNumber(compile.Program program, ast.Expression root, string type, uint limit, bool finalized, ref uint result)
{
    if(compile.foldConstant(program, root, finalized, result))
    {
        if(result > limit)
        {
            error(
                std.string.format(
                    "value %s is outside of representable %s range 0..%s", result, type, limit
                ), root.location
            );
            return false;
        }
        return true;
    }
    return false;
}

bool foldBit(compile.Program program, ast.Expression root, bool finalized, ref uint result)
{
    return foldBoundedNumber(program, root, "bit", 1, finalized, result);
}

bool foldBitIndex(compile.Program program, ast.Expression root, bool finalized, ref uint result)
{
    return foldBoundedNumber(program, root, "bitwise index", 1, finalized, result);
}

bool foldByte(compile.Program program, ast.Expression root, bool finalized, ref uint result)
{
    return foldBoundedNumber(program, root, "8-bit", 255, finalized, result);
}

bool foldWord(compile.Program program, ast.Expression root, bool finalized, ref uint result)
{
    return foldBoundedNumber(program, root, "16-bit", 65535, finalized, result);
}

bool foldSignedByte(compile.Program program, ast.Expression root, bool negative, bool finalized, ref uint result)
{
    if(foldWord(program, root, finalized, result))
    {
        if(!negative && result < 127)
        {
            return true;
        }
        else if(negative && result < 128)
        {
            result = ~result + 1;
            return true;
        }
        else
        {
            error(
                std.string.format(
                    "value %s%s is outside of representable signed 8-bit range -128..127.", negative ? "-" : "", result
                ), root.location
            );
        }
    }
    return false;
}

bool foldRelativeByte(compile.Program program, ast.Expression root, string description, string help, uint origin, bool finalized, ref uint result)
{
    if(foldWord(program, root, finalized, result))
    {
        std.stdio.writefln("%s: result: %s", root.location, result);
        int offset = cast(int) result - cast(int) origin;
        if(program.finalized)
        {
            std.stdio.writefln("%s: result:0x%04X - origin:0x%04X = 0x%04X (%d)", root.location, result, origin, cast(ubyte) offset, offset);
        }
        if(offset >= -128 && offset <= 127)
        {
            result = cast(ubyte) offset;
            return true;
        }
        else
        {
            error(
                std.string.format(
                    description ~ " is outside of representable signed 8-bit range -128..127. "
                    ~ help ~ " (from = %s, to = %s, (from - to) = %s)",
                    origin, result, offset
                ), root.location
            );
        }
    }
    return false;
}

bool foldStorage(Program program, ast.Storage s, ref bool sizeless, ref uint unit, ref uint total)
{
    sizeless = false;
    if(s.size is null)
    {
        sizeless = true;
        total = 1;
    }
    else if(!foldConstant(program, s.size, true, total))
    {
        return false;
    }
    switch(s.type)
    {
        case parse.Keyword.Byte: unit = 1; break;
        case parse.Keyword.Word: unit = 2; break;
        default:
            error("Unsupported storage type " ~ parse.getKeywordName(s.type), s.location);
    }
    total *= unit;
    return true;
}

ubyte[] foldDataExpression(Program program, ast.Expression root, uint unit, bool finalized)
{
    switch(unit)
    {
        case 1:
            if(auto str = cast(ast.String) root)
            {
                return cast(ubyte[]) str.value;
            }
            else
            {
                uint result;
                foldByte(program, root, finalized, result);
                return [result & 0xFF];
            }
        case 2:
            uint result;
            foldWord(program, root, finalized, result);
            return [result & 0xFF, (result >> 8) & 0xFF];
        default:
            assert(0);
    }
}

auto createBlockHandler(Program program)
{
    return(Visitor.Pass pass, ast.Block block)
    {
        if(pass == Visitor.Pass.Before)
        {
            if(block.name)
            {
                auto match = program.environment.get!(sym.PackageDef)(block.name);
                if(match !is null)
                {
                    program.enterEnvironment(block, match.environment);
                }
                else
                {
                    program.enterEnvironment(block);
                }
            }
            else
            {
                program.enterEnvironment(block);
            }
        }
        else
        {
            auto pkg = program.environment;
            program.leaveEnvironment();
            if(block.name)
            {
                auto match = program.environment.get!(sym.PackageDef)(block.name);
                if(match is null)
                {
                    program.environment.put(block.name, new sym.PackageDef(block, pkg));
                }
            }
        }
    };
}

auto createRelocationHandler(Program program)
{
    return(ast.Relocation stmt)
    {
        enum description = "'in' statement";
        uint address;
        if(stmt.dest is null || foldConstant(program, stmt.dest, program.finalized, address))
        {
            auto def = program.environment.get!(sym.BankDef)(stmt.mangledName);
            if(def is null)
            {
                error("unknown bank '" ~ stmt.name ~ "' referenced by " ~ description, stmt.location);
                return;
            }
            program.switchBank(def.bank);
            if(stmt.dest !is null)
            {
                def.bank.setAddress(description, address, stmt.location);
            }
        }
    };
}

auto createCommandHandler(Program program)
{
    return(ast.Command stmt)
    {
        auto description = parse.getKeywordName(stmt.type) ~ " statement";
        auto code = program.platform.generateCommand(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        if(program.finalized)
        {
            bank.writePhysical(code, stmt.location);
        }
        else
        {
            bank.reservePhysical(description, code.length, stmt.location);
        }
    };
}

auto createJumpHandler(Program program)
{
    return(ast.Jump stmt)
    {
        auto description = parse.getKeywordName(stmt.type) ~ " statement";
        auto code = program.platform.generateJump(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        if(program.finalized)
        {
            bank.writePhysical(code, stmt.location);
        }
        else
        {
            bank.reservePhysical(description, code.length, stmt.location);
        }
    };
}

auto createAssignmentHandler(Program program)
{
    return(ast.Assignment stmt)
    {
        enum description = "assignment";
        auto code = program.platform.generateAssignment(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        if(program.finalized)
        {
            bank.writePhysical(code, stmt.location);
        }
        else
        {
            bank.reservePhysical(description, code.length, stmt.location);
        }
    };
}

auto createComparisonHandler(Program program)
{
    return(ast.Comparison stmt)
    {
        enum description = "comparison";
        auto code = program.platform.generateComparison(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        if(program.finalized)
        {
            bank.writePhysical(code, stmt.location);
        }
        else
        {
            bank.reservePhysical(description, code.length, stmt.location);
        }
    };
}

void build(Program program, ast.Node root)
{
    uint depth = 0;
    root.traverse(
        (ast.Conditional cond)
        {
            cond.expand();
        },

        (ast.Loop loop)
        {
            loop.expand();
        },
    );

    root.traverse(
        (Visitor.Pass pass, ast.Loop loop)
        {
            if(pass == Visitor.Pass.Before)
            {
                depth++;
            }
            else
            {
                depth--;
            }
        },

        (ast.Jump jump)
        {
            switch(jump.type)
            {
                case parse.Keyword.While, parse.Keyword.Until,
                    parse.Keyword.Break, parse.Keyword.Continue:
                    if(depth == 0)
                    {
                        error("'" ~ parse.getKeywordName(jump.type) ~ "' used outside of a 'loop'.", jump.location);
                    }
                    else
                    {
                        jump.expand();
                    }
                    break;
                default:
            }
        }
    );

    root.traverse(
        createBlockHandler(program),

        (ast.ConstDecl decl)
        {
            program.environment.put(decl.name, new sym.ConstDef(decl, program.environment));
        },

        (ast.BankDecl decl)
        {
            foreach(i, name; decl.names)
            {
                program.environment.put(name, new sym.BankDef(decl));
            }
        },

        (ast.VarDecl decl)
        {
            foreach(i, name; decl.names)
            {
                program.environment.put(name, new sym.VarDef(decl));
            }
        },

        (ast.LabelDecl decl)
        {
            program.environment.put(decl.name, new sym.LabelDef(decl));
        },
    );

    root.traverse(
        createBlockHandler(program),

        (ast.BankDecl decl)
        {
            uint size;
            if(foldConstant(program, decl.size, true, size))
            {
                foreach(i, name; decl.names)
                {
                    auto def = program.environment.get!(sym.BankDef)(name);
                    def.bank = new Bank(name, decl.type == "rom", size);
                    program.addBank(def.bank);
                }
            }
        }
    );
    verify();

    program.rewind();
    root.traverse(
        createBlockHandler(program),
        createRelocationHandler(program),

        (ast.VarDecl decl)
        {
            enum description = "variable declaration";
            bool sizeless;
            uint unit, total;
            if(foldStorage(program, decl.storage, sizeless, unit, total))
            {
                auto bank = program.checkBank(description, decl.location);
                foreach(i, name; decl.names)
                {
                    auto def = program.environment.get!(sym.VarDef)(name);
                    def.hasAddress = true;
                    def.address = bank.checkAddress(description, decl.location);
                    bank.reserveVirtual(description, total, decl.location);
                }
            }
        },
    );
    verify();

    program.rewind();
    root.traverse(
        createBlockHandler(program),
        createRelocationHandler(program),
        createCommandHandler(program),
        createJumpHandler(program),
        createAssignmentHandler(program),
        createComparisonHandler(program),

        (ast.LabelDecl decl)
        {
            enum description = "label declaration";
            auto bank = program.checkBank(description, decl.location);
            auto def = program.environment.get!(sym.LabelDef)(decl.name);
            def.hasAddress = true;
            def.address = bank.checkAddress(description, decl.location);
        },

        (ast.Embed stmt)
        {
            enum description = "'embed' statement";

            if(std.file.exists(stmt.filename))
            {
                if(std.file.isDir(stmt.filename))
                {
                    error("attempt to embed directory '" ~ stmt.filename ~ "'", stmt.location, true);
                }
            }
            else
            {
                error("could not embed file '" ~ stmt.filename ~ "'", stmt.location, true);
            }
        
            try
            {
                std.stdio.File file = std.stdio.File(stmt.filename, "rb");
                file.seek(0);
                auto start = cast(uint) file.tell();
                file.seek(0, std.stdio.SEEK_END);
                auto end = cast(uint) file.tell();
                
                stmt.data = new ubyte[end - start];
                file.seek(0);
                file.rawRead(stmt.data);
                file.close();
            }
            catch(std.stdio.Exception e)
            {
                error("could not embed file '" ~ stmt.filename ~ "' (" ~ e.toString ~ ")", stmt.location, true);
            }
            
            auto bank = program.checkBank(description, stmt.location);
            bank.reservePhysical(description, stmt.data.length, stmt.location);
        },

        (ast.Data stmt)
        {
            enum description = "inline data";
            bool sizeless;
            uint unit, total;
            if(foldStorage(program, stmt.storage, sizeless, unit, total))
            {
                ubyte[] data;
                assert(stmt.items.length > 0);
                foreach(item; stmt.items)
                {
                    data ~= foldDataExpression(program, item, unit, program.finalized);
                }
                if(!sizeless)
                {
                    if(data.length < total)
                    {
                        // Fill unused section with final byte of data.
                        data ~= std.array.array(std.range.repeat(data[data.length - 1], total - data.length));
                    }
                    else if(data.length > total)
                    {
                        error(
                            std.string.format(
                                "%s is an %s-byte sequence, which is %s byte(s) over the declared %s-byte limit",
                                description, data.length, data.length - total, total
                            ), stmt.location
                        );
                    }
                }
                auto bank = program.checkBank(description, stmt.location);
                bank.reservePhysical(description, data.length, stmt.location);
                stmt.data = data;
            }
        }
    );
    verify();

    program.finalized = true;
    program.rewind();
    root.traverse(
        createBlockHandler(program),
        createRelocationHandler(program),
        createCommandHandler(program),
        createJumpHandler(program),
        createAssignmentHandler(program),
        createComparisonHandler(program),

        (ast.LabelDecl decl)
        {
            enum description = "label declaration";
            auto bank = program.checkBank(description, decl.location);
            auto def = program.environment.get!(sym.LabelDef)(decl.name);
            auto addr = bank.checkAddress(description, decl.location);
            std.stdio.writefln("%s: def %s: 0x%04X", decl.location, decl.name, def.address);
            if(!def.hasAddress)
            {
                error("what the hell. label was never given address!", decl.location, true);
            }
            if(addr != def.address)
            {
                error(
                    std.string.format(
                        "what the hell. inconsistency in label positions detected"
                        ~ " (was 0x%04X instruction selection pass, 0x%04X on code-gen pass)",
                        addr, def.address
                    ), decl.location, true
                );
            }
        },

        (ast.Embed stmt)
        {
            enum description = "'embed' statement";
            auto bank = program.checkBank(description, stmt.location);
            bank.writePhysical(stmt.data, stmt.location);
        },

        (ast.Data stmt)
        {
            enum description = "inline data";
            auto bank = program.checkBank(description, stmt.location);
            bank.writePhysical(stmt.data, stmt.location);
        }
    );
}

