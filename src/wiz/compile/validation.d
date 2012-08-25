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

bool foldConstExpr(Program program, ast.Expression root, ref uint result, bool forbidUndefined = true)
{
    ast.Expression  _;
    return partiallyFoldConstExpr(program, root, result, _, true, forbidUndefined);
}

bool partiallyFoldConstExpr(Program program, ast.Expression root, ref uint result, ref ast.Expression lastConst, bool mustFold, bool forbidUndefined)
{
    uint[ast.Expression] values;
    bool constant = true;
    bool mustFoldRoot = mustFold;
    uint depth = 0;

    traverse(root,
        (ast.Infix e)
        {
            auto first = e.operands[0];
            if((first in values) is null)
            {
                return;
            }
            uint a = values[first];

            foreach(i, type; e.types)
            {                
                auto operand = e.operands[i + 1];
                if((operand in values) is null)
                {
                    return;
                }
                uint b = values[operand];

                final switch(type)
                {
                    case parse.Infix.Add: 
                        if(a + ast.Expression.Max < b)
                        {
                            error("addition yields result which will overflow outside of 0..65535.", operand.location);
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
                        constant = false;
                        if(mustFold)
                        {
                            error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in constant expression", operand.location);
                        }
                        return;
                }
                if(constant)
                {
                    values[e] = a;
                    lastConst = e;
                }
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
                    case parse.Prefix.Not, parse.Prefix.Indirection:
                        constant = false;
                        if(mustFold)
                        {
                            error("prefix operator " ~ parse.getPrefixName(e.type) ~ " cannot be used in constant expression", e.location);
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
                        values[e] = r & 0xFF;
                        break;
                    case parse.Prefix.High:
                        values[e] = (r >> 8) & 0xFF;
                        break;
                    case parse.Prefix.Swap:
                        values[e] = ((r & 0x0F0F) << 4) | ((r & 0xF0F0) >> 4);
                        break;
                    case parse.Prefix.Grouping:
                        values[e] = r;
                        break;
                    case parse.Prefix.Not, parse.Prefix.Indirection:
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
                constant = false;
                if(mustFold)
                {
                    error("postfix operator " ~ parse.getPostfixName(e.type) ~ " cannot be used in constant expression", e.location);
                }
            }
        },

        (ast.Attribute a)
        {
            auto def = resolveAttribute(program, a);
            if(auto constdef = cast(sym.ConstDef) def)
            {
                uint v;
                if(foldConstExpr(program, (cast(ast.ConstDecl) constdef.decl).value, v))
                {
                    if(constant)
                    {
                        lastConst = a;
                    }
                    values[a] = v;
                    return;
                }
            }
            else if(auto vardef = cast(sym.LabelDef) def)
            {
                uint v;
                if(vardef.hasAddress)
                {
                    if(constant)
                    {
                        lastConst = a;
                    }
                    values[a] = vardef.address;
                    return;
                }
            }
            else if(auto labeldef = cast(sym.LabelDef) def)
            {
                uint v;
                if(labeldef.hasAddress)
                {
                    if(constant)
                    {
                        lastConst = a;
                    }
                    values[a] = labeldef.address;
                    return;
                }
            }

            constant = false;
            if(def && mustFold && forbidUndefined)
            {
                error("'" ~ a.fullName() ~ "' was declared, but could not be evaluated.", root.location);
            }
        },

        (ast.Number n)
        {
            if(constant)
            {
                lastConst = n;
            }
            values[n] = n.value;
        },
    );

    auto resolution = (root in values);
    if(resolution is null)
    {
        if(mustFold)
        {
            error("expression could not be resolved as a constant", root.location);
        }
        return false;
    }
    else
    {
        result = *resolution;
        return true;
    }
}

bool foldStorage(Program program, ast.Storage s, ref uint result)
{
    if(foldConstExpr(program, s.size, result))
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
        if(stmt.dest is null || foldConstExpr(program, stmt.dest, address))
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
            if(foldConstExpr(program, decl.size, size))
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

