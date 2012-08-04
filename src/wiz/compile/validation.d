module wiz.compile.validation;

import wiz.lib;
import wiz.compile.lib;

static import std.stdio;

void build(Program program, ast.Node root)
{
    aggregate(program, root);
}

bool foldNumber(Program program, ast.Expression root, ref uint result)
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
            uint r;
            switch(e.infixType)
            {
                case parse.Token.ADD: r = a + b; break;
                case parse.Token.SUB: r = a - b; break;
                case parse.Token.MUL: r = a * b; break;
                case parse.Token.DIV: r = a / b; break;
                case parse.Token.MOD: r = a % b; break;
                case parse.Token.SHL: r = a << b; break;
                case parse.Token.SHR: r = a >> b; break;
                case parse.Token.AND: r = a & b; break;
                case parse.Token.OR: r = a | b; break;
                case parse.Token.XOR: r = a ^ b; break;
                case parse.Token.AT: r = a & (1 << b); break;
                default:
                    error("Unsupported operator " ~ parse.getSimpleTokenName(e.infixType), e.location);
            }
            values[e] = r;
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
                case parse.Token.LT: r = r & 0xFF; break;
                case parse.Token.GT: r = (r >> 8) & 0xFF; break;
                case parse.Token.SWAP:
                    r = ((r & 0x0F0F) << 4) | ((r & 0xF0F0) >> 4);
                    break;
                //case parse.Token.LBRACKET: break;
                default:
                    error("Unsupported operator " ~ parse.getSimpleTokenName(e.prefixType), e.location);
            }
            values[e] = r;
        },

        (ast.Postfix e)
        {
            if((e.operand in values) is null)
            {
                return;
            }
            // TODO
            values[e] = 0;
        },

        (ast.Attribute e)
        {
            // TODO
            values[e] = 0;
        },

        (ast.Number e)
        {
            values[e] = e.value;
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
    if(foldNumber(program, s.arraySize, result))
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

void aggregate(Program program, ast.Node root)
{
    root.traverse(
        (Visitor.Pass pass, ast.Block block)
        {
            if(pass == Visitor.Pass.BEFORE)
            {
                program.enterEnvironment(block);
            }
            else
            {
                program.leaveEnvironment();
            }
        },

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
        (Visitor.Pass pass, ast.Block block)
        {
            if(pass == Visitor.Pass.BEFORE)
            {
                program.enterEnvironment(block);
            }
            else
            {
                program.leaveEnvironment();
            }
        },

        (ast.BankDecl decl)
        {
            uint size;
            if(foldNumber(program, decl.size, size))
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

    root.traverse(
        (Visitor.Pass pass, ast.Block block)
        {
            if(pass == Visitor.Pass.BEFORE)
            {
                program.enterEnvironment(block);
            }
            else
            {
                program.leaveEnvironment();
            }
        },

        (ast.Relocation stmt)
        {
            uint address;
            if(stmt.dest is null || foldNumber(program, stmt.dest, address))
            {
                auto def = program.environment.get!(sym.BankDef)(stmt.mangledName);
                if(def is null)
                {
                    error("unknown bank '" ~ stmt.name ~ "' in relocation statement", stmt.location);
                    return;
                }

                program.switchBank(def.bank);
                if(stmt.dest !is null)
                {
                    def.bank.setAddress("'in' statement", address, stmt.location);
                    //error(std.string.format("in %s, 0x%X:", stmt.name, address), stmt.location);
                }
            }
        },

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
}