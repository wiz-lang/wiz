module wiz.compile.validation;

import wiz.lib;
import wiz.compile.lib;

static import std.stdio;

void build(Program program, ast.Node root)
{
    aggregate(program, root);
}

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

bool tryFoldConstant(Program program, ast.Expression root, ref uint result, ref ast.Expression constTail, bool mustFold, bool forbidUndefined)
{
    uint[ast.Expression] values;
    bool[ast.Expression] completeness;
    bool mustFoldRoot = mustFold;
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
                        if(mustFold)
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
                    mustFold = true;
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
                            mustFold = mustFoldRoot;
                        }
                        break;
                    case parse.Prefix.Not, parse.Prefix.Sub:
                        if(mustFold)
                        {
                            error("prefix operator " ~ parse.getPrefixName(e.type) ~ " cannot be used in constant expression", e.location);
                        }
                        return;
                    case parse.Prefix.Indirection:
                        depth--;
                        if(mustFold)
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
                if(mustFold)
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
                uint v;
                if(foldConstant(program, (cast(ast.ConstDecl) constdef.decl).value, v, true))
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

            if(def && mustFold && forbidUndefined)
            {
                error("'" ~ a.fullName() ~ "' was declared, but could not be evaluated.", root.location);
            }
        },

        (ast.Number n)
        {
            updateValue(n, n.value);
        },
    );

    result = values.get(root, 0);
    if(!completeness.get(root, false) && mustFold)
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

bool foldConstant(Program program, ast.Expression root, ref uint result, bool forbidUndefined)
{
    ast.Expression constTail;
    return tryFoldConstant(program, root, result, constTail, true, forbidUndefined) && constTail == root;
}

bool foldBoundedNumber(compile.Program program, ast.Expression root, string type, uint limit, ref uint result, bool forbidUndefined)
{
    if(compile.foldConstant(program, root, result, forbidUndefined))
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

bool foldBit(compile.Program program, ast.Expression root, ref uint result, bool forbidUndefined)
{
    return foldBoundedNumber(program, root, "bit", 1, result, forbidUndefined);
}

bool foldBitIndex(compile.Program program, ast.Expression root, ref uint result, bool forbidUndefined)
{
    return foldBoundedNumber(program, root, "bitwise index", 1, result, forbidUndefined);
}

bool foldByte(compile.Program program, ast.Expression root, ref uint result, bool forbidUndefined)
{
    return foldBoundedNumber(program, root, "8-bit", 255, result, forbidUndefined);
}

bool foldWord(compile.Program program, ast.Expression root, ref uint result, bool forbidUndefined)
{
    return foldBoundedNumber(program, root, "8-bit", 65535, result, forbidUndefined);
}

bool foldSignedByte(compile.Program program, ast.Expression root, bool negative, ref uint result, bool forbidUndefined)
{
    if(foldWord(program, root, result, forbidUndefined))
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

bool foldRelativeByte(compile.Program program, ast.Expression root, string description, string help, uint origin, ref uint result, bool forbidUndefined)
{
    if(foldWord(program, root, result, forbidUndefined))
    {
        int offset = cast(int) result - cast(int) origin;
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

bool foldStorage(Program program, ast.Storage s, ref uint result)
{
    if(foldConstant(program, s.size, result, true))
    {
        switch(s.type)
        {
            case parse.Keyword.Byte: break;
            case parse.Keyword.Word: result *= 2; break;
            default:
                error("Unsupported storage type " ~ parse.getKeywordName(s.type), s.location);
        }
        return true;
    }
    return false;
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
        if(stmt.dest is null || foldConstant(program, stmt.dest, address, true))
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
        bank.reservePhysical(description, code.length, stmt.location);
    };
}

auto createJumpHandler(Program program)
{
    return(ast.Jump stmt)
    {
        auto description = parse.getKeywordName(stmt.type) ~ " statement";
        auto code = program.platform.generateJump(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        bank.reservePhysical(description, code.length, stmt.location);
    };
}

auto createAssignmentHandler(Program program)
{
    return(ast.Assignment stmt)
    {
        enum description = "assignment";
        auto code = program.platform.generateAssignment(program, stmt);
        auto bank = program.checkBank(description, stmt.location);
        bank.reservePhysical(description, code.length, stmt.location);
    };
}

auto createComparisonHandler(Program program)
{
    return(ast.Comparison stmt)
    {
        auto code = program.platform.generateComparison(program, stmt);
    };
}

void aggregate(Program program, ast.Node root)
{
    root.traverse(
        createBlockHandler(program),

        (ast.ConstDecl decl)
        {
            program.environment.put(decl.name, new sym.ConstDef(decl));
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
            if(foldConstant(program, decl.size, size, true))
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
            uint size;
            if(foldStorage(program, decl.storage, size))
            {
                auto bank = program.checkBank(description, decl.location);
                foreach(i, name; decl.names)
                {
                    auto def = program.environment.get!(sym.VarDef)(name);
                    def.hasAddress = true;
                    def.address = bank.checkAddress(description, decl.location);
                    bank.reserveVirtual(description, size, decl.location);
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
    );
    verify();

}

