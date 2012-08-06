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
            def = env.get!(sym.Definition)(piece, true);
        }
        else
        {
            sym.PackageDef pkg = cast(sym.PackageDef) prev;
            if(pkg is null)
            {
                string fullyQualifiedName = std.string.join(attribute.pieces, ".");
                string previousName = std.string.join(partialQualifiers[0 .. partialQualifiers.length - 1], ".");
                error("attempt to get symbol '" ~ fullyQualifiedName ~ "', but '" ~ previousName ~ "' is not a package", attribute.location);
            }
            else
            {
                env = pkg.environment;
                def = env.get!(sym.Definition)(piece, false);
            }
        }
        
        if(def is null)
        {
            string partiallyQualifiedName = std.string.join(partialQualifiers, ".");
            string fullyQualifiedName = std.string.join(attribute.pieces, ".");
            
            if(partiallyQualifiedName == fullyQualifiedName)
            {
                error("reference to undefined symbol '" ~ partiallyQualifiedName ~ "'", attribute.location);
            }
            else
            {
                error("reference to undefined symbol '" ~ partiallyQualifiedName ~ "' (in '" ~ fullyQualifiedName ~ "')", attribute.location);
            }
            return null;
        }
        
        prev = def;
    }
    return def;
}

bool foldConstExpr(Program program, ast.Expression root, ref uint result)
{
    uint[ast.Expression] values;
    traverse(root,
        (ast.Infix e)
        {
            if((e.left in values) is null || (e.right in values) is null)
            {
                return;
            }
            uint a = values[e.left];
            uint b = values[e.right];
            switch(e.infixType)
            {
                case parse.Token.Add: 
                    if(a + ast.Expression.Max < b)
                    {
                        error("addition yields result which will overflow outside of 0..65535.", e.right.location);
                    }
                    else
                    {
                        values[e] = a + b;
                    }
                    break;
                case parse.Token.Sub:
                    if(a < b)
                    {
                        error("subtraction yields result which will overflow outside of 0..65535.", e.right.location);
                    }
                    else
                    {
                        values[e] = a - b;
                    }
                    break;
                case parse.Token.Mul:
                    if(a > ast.Expression.Max / b)
                    {
                        error("multiplication yields result which will overflow outside of 0..65535.", e.right.location);
                    }
                    else
                    {
                        values[e] = a * b;
                    }
                    break;
                case parse.Token.Div:
                    if(a == 0)
                    {
                        error("division by zero is undefined.", e.right.location);
                    }
                    else
                    {
                        values[e] = a / b;
                    }
                    break;
                case parse.Token.Mod:
                    if(a == 0)
                    {
                        error("modulo by zero is undefined.", e.right.location);
                    }
                    else
                    {
                        values[e] = a % b;
                    }
                    break;
                case parse.Token.ShiftL:
                    // If shifting more than N bits, or ls << rs > 2^N-1, then error.
                    if(b > 16 || (b > 0 && (a & ~(1 << (16 - b))) != 0))
                    {
                        error("logical shift left yields result which will overflow outside of 0..65535.", e.right.location);
                    }
                    else
                    {
                        values[e] = a << b;
                    }
                    break;
                case parse.Token.ShiftR:
                    values[e] = a >> b;
                    break;
                case parse.Token.And:
                    values[e] = a & b;
                    break;
                case parse.Token.Or:
                    values[e] = a | b;
                    break;
                case parse.Token.Xor:
                    values[e] = a ^ b;
                    break;
                case parse.Token.At:
                    values[e] = a & (1 << b);
                    break;
                default:
                    error("infix operator " ~ parse.getSimpleTokenName(e.infixType) ~ " cannot be used in constant expression", e.location);
            }
        },

        (ast.Prefix e)
        {
            if((e.operand in values) is null)
            {
                return;
            }
            uint r = values[e.operand];
            switch(e.prefixType)
            {
                case parse.Token.Less:
                    values[e] = r & 0xFF;
                    break;
                case parse.Token.Greater:
                    values[e] = (r >> 8) & 0xFF;
                    break;
                case parse.Token.Swap:
                    values[e] = ((r & 0x0F0F) << 4) | ((r & 0xF0F0) >> 4);
                    break;
                default:
                    error("prefix operator " ~ parse.getSimpleTokenName(e.prefixType) ~ " cannot be used in constant expression", e.location);
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
                error("postfix operator " ~ parse.getSimpleTokenName(e.postfixType) ~ " cannot be used in constant expression", e.location);
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
                    values[a] = v;
                }
            }
        },

        (ast.Number n)
        {
            values[n] = n.value;
        },
    );

    auto resolution = (root in values);
    if(resolution is null)
    {
        error("expression could not be resolved as a constant", root.location);
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
    if(foldConstExpr(program, s.arraySize, result))
    {
        switch(s.storageType)
        {
            case parse.Keyword.BYTE: break;
            case parse.Keyword.WORD: result *= 2; break;
            default:
                error("Unsupported storage type " ~ parse.getKeywordName(s.storageType), s.location);
        }
        return true;
    }
    return false;
}

auto createBlockHandler(Program program)
{
    return(Visitor.Pass pass, ast.Block block)
    {
        if(pass == Visitor.Pass.BEFORE)
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
        auto code = program.platform.generateCommand(program, stmt);
    };
}

auto createJumpHandler(Program program)
{
    return(ast.Jump stmt)
    {
        auto code = program.platform.generateJump(program, stmt);
    };
}

auto createAssignmentHandler(Program program)
{
    return(ast.Assignment stmt)
    {
        auto code = program.platform.generateAssignment(program, stmt);
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
                    def.bank = new Bank(name, decl.bankType == "rom", size);
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

        (ast.LabelDecl decl)
        {
            enum description = "label declaration";
            auto bank = program.checkBank(description, decl.location);
            auto def = program.environment.get!(sym.LabelDef)(decl.name);
            def.address = bank.checkAddress(description, decl.location);
        },
        
        (ast.Command stmt)
        {
            auto description = parse.getKeywordName(stmt.type) ~ " statement";
            auto code = program.platform.generateCommand(program, stmt);
            auto bank = program.checkBank(description, stmt.location);
            bank.reservePhysical(description, code.length, stmt.location);
        },

        (ast.Jump stmt)
        {
            auto description = parse.getKeywordName(stmt.type) ~ " statement";
            auto code = program.platform.generateJump(program, stmt);
            auto bank = program.checkBank(description, stmt.location);
            bank.reservePhysical(description, code.length, stmt.location);
        },

        (ast.Assignment stmt)
        {
            enum description = "assignment";
            auto code = program.platform.generateAssignment(program, stmt);
            auto bank = program.checkBank(description, stmt.location);
            bank.reservePhysical(description, code.length, stmt.location);
        },
    );
    verify();

}

