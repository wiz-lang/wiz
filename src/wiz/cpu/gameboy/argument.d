module wiz.cpu.gameboy.argument;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.gameboy.lib;

static import std.conv;
static import std.array;
static import std.string;

enum ArgumentType
{
    None,
    Immediate,
    Indirection,
    IndirectionInc,
    IndirectionDec,
    PositiveIndex,
    NegativeIndex,
    BitIndex,
    Not,
    Swap,
    Pop,
    A,
    B,
    C,
    D,
    E,
    F,
    H,
    L,
    AF,
    BC,
    DE,
    HL,
    SP,
    Carry,
    Zero,
    Interrupt,
}

class Argument
{
    ArgumentType type;
    ast.Expression immediate;
    Argument base;

    this(ArgumentType type)
    {
        this.type = type;
    }

    this(ArgumentType type, ast.Expression immediate)
    {
        this(type);
        this.immediate = immediate;
    }

    this(ArgumentType type, Argument base)
    {
        this(type);
        this.base = base;
    }

    this(ArgumentType type, ast.Expression immediate, Argument base)
    {
        this(type);
        this.immediate = immediate;
        this.base = base;
    }
}

class Builtin : sym.Definition
{
    ArgumentType type;

    this(ArgumentType type)
    {
        super(new ast.BuiltinDecl());
        this.type = type;
    }
}

ubyte getRegisterIndex(Argument reg)
{
    switch(reg.type)
    {
        case ArgumentType.B: return 0x0; break;
        case ArgumentType.C: return 0x1; break;
        case ArgumentType.D: return 0x2; break;
        case ArgumentType.E: return 0x3; break;
        case ArgumentType.H: return 0x4; break;
        case ArgumentType.L: return 0x5; break;
        case ArgumentType.Indirection:
            if(reg.base.type == ArgumentType.HL)
            {
                return 0x6;
            }
            break;
        case ArgumentType.A: return 0x7; break;
        default:
    }
    assert(0);
}

ubyte getPairIndex(Argument pair)
{
    switch(pair.type)
    {
        case ArgumentType.BC: return 0x0; break;
        case ArgumentType.DE: return 0x1; break;
        case ArgumentType.HL: return 0x2; break;
        case ArgumentType.SP: return 0x3; break;
        default:
    }
    assert(0);
}

ubyte getFlagIndex(ArgumentType flagType, bool negated)
{
    switch(flagType)
    {
        case ArgumentType.Zero: return negated ? 0x0 : 0x1; break;
        case ArgumentType.Carry: return negated ? 0x2 : 0x3; break;
        default: assert(0);
    }
}

Argument buildIndirection(compile.Program program, ast.Expression root)
{
    if(auto attr = cast(ast.Attribute) root)
    {
        auto def = compile.resolveAttribute(program, attr);
        if(!def)
        {
            return null;
        }
        if(auto builtin = cast(Builtin) def)
        {
            return new Argument(ArgumentType.Indirection, new Argument(builtin.type));
        }
    }
    else if(auto prefix = cast(ast.Prefix) root)
    {
        if(prefix.type == parse.Prefix.Indirection)
        {
            error("double-indirection is not supported on the gameboy", prefix.location);
            return null;
        }
    }
    else if(auto postfix = cast(ast.Postfix) root)
    {
        ArgumentType argumentType;
        final switch(postfix.type)
        {
            case parse.Postfix.Inc: argumentType = ArgumentType.IndirectionInc; break;
            case parse.Postfix.Dec: argumentType = ArgumentType.IndirectionDec; break;
        }
        if(auto attr = cast(ast.Attribute) postfix.operand)
        {
            auto def = compile.resolveAttribute(program, attr);
            if(!def)
            {
                return null;
            }
            if(auto builtin = cast(Builtin) def)
            {
                if(builtin.type == ArgumentType.HL)
                {
                    return new Argument(argumentType, new Argument(builtin.type));
                }
            }
        }
        error(
            "operator " ~ parse.getPostfixName(postfix.type)
            ~ " on indirected operand is not supported (only '[hl"
            ~ parse.getPostfixName(postfix.type) ~ "]' is valid)",
            postfix.location
        );
        return null;
    }
    else if(auto infix = cast(ast.Infix) root)
    {
        Builtin registerLeft;
        Builtin registerRight;

        if(infix.types[0] == parse.Infix.Colon)
        {
            if(auto attr = cast(ast.Attribute) infix.operands[1])
            {
                auto def = compile.resolveAttribute(program, attr);
                if(!def)
                {
                    return null;
                }
                if(auto builtin = cast(Builtin) def)
                {
                    if(auto prefix = cast(ast.Prefix) infix.operands[0])
                    {
                        if(prefix.type == parse.Infix.Sub)
                        {
                            return new Argument(ArgumentType.NegativeIndex, prefix.operand, new Argument(builtin.type));
                        }
                    }
                    return new Argument(ArgumentType.PositiveIndex, infix.operands[0], new Argument(builtin.type));
                }
            }
            error(
                "index operator ':' must have register as a right-hand term",
                infix.location
            );
            return null;
        }
    }
    else if(auto pop = cast(ast.Pop) root)
    {
        error("'pop' is not allowed inside of indirection", pop.location);
        return null;
    }
    return new Argument(ArgumentType.Indirection, new Argument(ArgumentType.Immediate, root));
}

Argument buildArgument(compile.Program program, ast.Expression root)
{
    uint v;
    if(auto attr = cast(ast.Attribute) root)
    {
        auto def = compile.resolveAttribute(program, attr);
        if(!def)
        {
            return null;
        }
        if(auto builtin = cast(Builtin) def)
        {
            return new Argument(builtin.type);
        }
    }
    else if(auto prefix = cast(ast.Prefix) root)
    {
        if(prefix.type == parse.Token.LBracket)
        {
            return buildIndirection(program, prefix.operand);
        }
        if(prefix.type == parse.Token.Swap)
        {
            return new Argument(ArgumentType.Swap, buildArgument(program, prefix.operand));
        }
        if(prefix.type == parse.Token.Not)
        {
            return new Argument(ArgumentType.Not, buildArgument(program, prefix.operand));
        }
    }
    else if(auto infix = cast(ast.Infix) root)
    {
        if(infix.types[0] == parse.Token.At)
        {
            if(auto attr = cast(ast.Attribute) infix.operands[0])
            {
                auto def = compile.resolveAttribute(program, attr);
                if(!def)
                {
                    return null;
                }
                if(auto builtin = cast(Builtin) def)
                {
                    return new Argument(ArgumentType.BitIndex, infix.operands[1], new Argument(builtin.type));
                }
            }
        }
    }
    else if(auto pop = cast(ast.Pop) root)
    {
        return new Argument(ArgumentType.Pop);
    }
    return new Argument(ArgumentType.Immediate, root);
}