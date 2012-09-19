module wiz.cpu.mos6502.argument;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.mos6502.lib;

enum ArgumentType
{
    None,
    Immediate,
    Indirection,
    Index,
    BitAnd,
    Not,
    Negated,
    Pop,
    A,
    X,
    Y,
    S,
    P,
    Zero,
    Negative,
    Decimal,
    Overflow,
    Carry,
    Interrupt,
}

class Argument
{
    ArgumentType type;
    ast.Expression immediate;
    Argument base;
    Argument secondary;

    this(ArgumentType type)
    {
        this.type = type;
    }

    this(ArgumentType type, ast.Expression immediate)
    {
        this(type);
        this.immediate = immediate;
    }

    this(ArgumentType type, Argument base, Argument secondary = null)
    {
        this(type);
        this.base = base;
        this.secondary = secondary;
    }

    string toString()
    {
        return toString(true);
    }

    string toString(bool quoted)
    {
        static string wrap(string s, bool quoted)
        {
            return quoted ? "'" ~ s ~ "'" : s;
        }
        final switch(type)
        {
            case ArgumentType.None: return "???";
            case ArgumentType.Immediate: return "immediate";
            case ArgumentType.Indirection: return wrap("[" ~ base.toString(false) ~ "]", quoted);
            case ArgumentType.Index: return wrap("[" ~ base.toString(false) ~ ":" ~ secondary.toString(false) ~ "]", quoted);
            case ArgumentType.BitAnd: return wrap("a & " ~ base.toString(false), quoted);
            case ArgumentType.Not: return wrap("~" ~ base.toString(false), quoted);
            case ArgumentType.Negated: return wrap("-" ~ base.toString(false), quoted);
            case ArgumentType.Pop: return wrap("pop", quoted);
            case ArgumentType.A: return wrap("a", quoted);
            case ArgumentType.X: return wrap("x", quoted);
            case ArgumentType.Y: return wrap("y", quoted);
            case ArgumentType.S: return wrap("s", quoted);
            case ArgumentType.P: return wrap("p", quoted);
            case ArgumentType.Zero: return wrap("zero", quoted);
            case ArgumentType.Negative: return wrap("negative", quoted);
            case ArgumentType.Decimal: return wrap("decimal", quoted);
            case ArgumentType.Overflow: return wrap("overflow", quoted);
            case ArgumentType.Carry: return wrap("carry", quoted);
            case ArgumentType.Interrupt: return wrap("interrupt", quoted);
        }
    }

    ubyte getFlagIndex(bool negated)
    {
        switch(type)
        {
            case ArgumentType.Zero: return negated ? 0x6 : 0x7;
            case ArgumentType.Negative: return negated ? 0x0 : 0x1;
            case ArgumentType.Overflow: return negated ? 0x2 : 0x3;
            case ArgumentType.Carry: return negated ? 0x4 : 0x5;
            default: assert(0);
        }
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
            return new Argument(ArgumentType.Indirection, buildIndirection(program, prefix.operand));
        }
    }
    else if(auto postfix = cast(ast.Postfix) root)
    {
        error(
            "operator " ~ parse.getPostfixName(postfix.type)
            ~ " on indirected operand is not supported.",
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
                    return new Argument(ArgumentType.Index, buildArgument(program, infix.operands[0]), new Argument(builtin.type));
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
        if(prefix.type == parse.Token.Not)
        {
            return new Argument(ArgumentType.Not, buildArgument(program, prefix.operand));
        }
        if(prefix.type == parse.Token.Sub)
        {
            return new Argument(ArgumentType.Negated, buildArgument(program, prefix.operand));
        }
    }
    else if(auto pop = cast(ast.Pop) root)
    {
        return new Argument(ArgumentType.Pop);
    }
    return new Argument(ArgumentType.Immediate, root);
}

Argument buildComparisonArgument(compile.Program program, ast.Expression root)
{
    if(auto infix = cast(ast.Infix) root)
    {
        if(infix.types[0] == parse.Token.And)
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
                    if(builtin.type == ArgumentType.A)
                    {
                        return new Argument(ArgumentType.BitAnd, buildArgument(program, infix.operands[1]));
                    }
                }
            }
        }
    }
    return buildArgument(program, root);
}