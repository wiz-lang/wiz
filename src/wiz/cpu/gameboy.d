module wiz.cpu.gameboy;

import wiz.lib;
import wiz.cpu.lib;

private
{
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

    Argument buildIndirection(compile.Program program, ast.Expression root)
    {
        if(auto attr = cast(ast.Attribute) root)
        {
            auto def = compile.resolveAttribute(program, attr);
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
                ~ " on indirected operand is not supported (only 'hl' is valid)",
                postfix.location
            );
            return null;
        }
        else if(auto infix = cast(ast.Infix) root)
        {
            Builtin registerLeft;
            Builtin registerRight;

            if(infix.type == parse.Infix.Colon)
            {
                if(auto attr = cast(ast.Attribute) infix.right)
                {
                    auto def = compile.resolveAttribute(program, attr);
                    if(auto builtin = cast(Builtin) def)
                    {
                        if(auto prefix = cast(ast.Prefix) infix.left)
                        {
                            if(prefix.type == parse.Infix.Sub)
                            {
                                return new Argument(ArgumentType.NegativeIndex, prefix.operand, new Argument(builtin.type));
                            }
                        }
                        return new Argument(ArgumentType.PositiveIndex, infix.left, new Argument(builtin.type));
                    }
                }
                error(
                    "index operator ':' must have register as a right-hand term",
                    infix.location
                );
                return null;
            }
        }
        return new Argument(ArgumentType.Indirection, new Argument(ArgumentType.Immediate, root));
    }

    Argument buildArgument(compile.Program program, ast.Expression root)
    {
        uint v;
        if(auto attr = cast(ast.Attribute) root)
        {
            auto def = compile.resolveAttribute(program, attr);
            if(auto builtin = cast(Builtin) def)
            {
                return new Argument(builtin.type);
            }
        }
        else if(auto prefix = cast(ast.Prefix) root)
        {
            if(prefix.type == parse.Token.LBracket)
            {
                return buildIndirection(program, prefix);
            }
        }
        else if(auto infix = cast(ast.Infix) root)
        {
            if(infix.type == parse.Token.At)
            {
                if(auto attr = cast(ast.Attribute) infix.left)
                {
                    auto def = compile.resolveAttribute(program, attr);
                    if(auto builtin = cast(Builtin) def)
                    {
                        return new Argument(ArgumentType.BitIndex, infix.right, new Argument(builtin.type));
                    }
                }
            }
        }
        return new Argument(ArgumentType.Immediate, root);
    }
}

class GameboyPlatform : Platform
{
    BuiltinTable builtins()
    {
        return [
            "a": new Builtin(ArgumentType.A),
            "b": new Builtin(ArgumentType.B),
            "c": new Builtin(ArgumentType.C),
            "d": new Builtin(ArgumentType.D),
            "e": new Builtin(ArgumentType.E),
            "f": new Builtin(ArgumentType.F),
            "h": new Builtin(ArgumentType.H),
            "l": new Builtin(ArgumentType.L),
            "af": new Builtin(ArgumentType.AF),
            "bc": new Builtin(ArgumentType.BC),
            "de": new Builtin(ArgumentType.DE),
            "hl": new Builtin(ArgumentType.HL),
            "sp": new Builtin(ArgumentType.SP),
            "carry": new Builtin(ArgumentType.Carry),
            "zero": new Builtin(ArgumentType.Zero),
            "interrupt": new Builtin(ArgumentType.Interrupt),
        ];
    }

    ubyte[] generateCommand(compile.Program program, ast.Command stmt)
    {
        switch(stmt.type)
        {
            case parse.Keyword.Push:
                auto argument = buildArgument(program, stmt.argument);
                switch(argument.type)
                {
                    case ArgumentType.AF: return [0xF5];
                    case ArgumentType.BC: return [0xC5];
                    case ArgumentType.DE: return [0xD5];
                    case ArgumentType.HL: return [0xE5];
                    default: return [];
                }
            default:
                return [];
        }
    }
    
    ubyte[] generateJump(compile.Program program, ast.Jump stmt)
    {
        ubyte getJumpOpcode(ArgumentType type = ArgumentType.None, bool negated = false)
        {
            switch(type)
            {
                case ArgumentType.Zero: return negated ? 0xC2 : 0xCA;
                case ArgumentType.Carry: return negated ? 0xD2 : 0xDA;
                case ArgumentType.None: return 0xC3;
                default:
                    assert(0);
            }
        }

        ubyte getCallOpcode(ArgumentType type = ArgumentType.None, bool negated = false)
        {
            switch(type)
            {
                case ArgumentType.Zero: return negated ? 0xC4 : 0xCC;
                case ArgumentType.Carry: return negated ? 0xD4 : 0xDC;
                case ArgumentType.None: return 0xCD;
                default:
                    assert(0);
            }
        }

        ubyte getReturnOpcode(ArgumentType type = ArgumentType.None, bool negated = false)
        {
            switch(type)
            {
                case ArgumentType.Zero: return negated ? 0xC0 : 0xC8;
                case ArgumentType.Carry: return negated ? 0xD0 : 0xD8;
                case ArgumentType.None: return 0xC9;
                default:
                    assert(0);
            }
        }

        switch(stmt.type)
        {
            case parse.Keyword.Goto:
                auto argument = buildArgument(program, stmt.destination);
                switch(argument.type)
                {
                    case ArgumentType.Immediate:
                        uint address;
                        if(!compile.foldConstExpr(program, argument.immediate, address))
                        {
                            // For now, sub a placeholder expression.
                            // At a later pass, we will ensure that the address is resolvable.
                            address = 0xFACE;
                        }
                        if(stmt.condition is null)
                        {
                            return [getJumpOpcode(), address & 0xFF, (address >> 8) & 0xFF];
                        }
                        else
                        {
                            auto cond = stmt.condition;
                            if(cond.attr is null)
                            {
                                ubyte opcode;
                                final switch(cond.branch)
                                {
                                    case parse.Branch.Equal:
                                        opcode = getJumpOpcode(ArgumentType.Zero, cond.negated);
                                    case parse.Branch.NotEqual:
                                        opcode = getJumpOpcode(ArgumentType.Zero, !cond.negated);
                                    case parse.Branch.Less:
                                        opcode = getJumpOpcode(ArgumentType.Carry, !cond.negated);
                                    case parse.Branch.GreaterEqual:
                                        opcode = getJumpOpcode(ArgumentType.Carry, cond.negated);
                                    case parse.Branch.Greater:
                                    case parse.Branch.LessEqual:
                                        error(
                                            "comparision '" ~ parse.getBranchName(cond.branch)
                                            ~ "' unsupported in 'when' clause of 'goto'", cond.location
                                        );
                                        return [];
                                }
                                return [opcode, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            else
                            {
                                auto attr = cond.attr;
                                auto def = compile.resolveAttribute(program, attr);
                                if(auto builtin = cast(Builtin) def)
                                {
                                    switch(builtin.type)
                                    {
                                        case ArgumentType.Carry:
                                            return [
                                                getJumpOpcode(ArgumentType.Carry, cond.negated),
                                                address & 0xFF, (address >> 8) & 0xFF
                                            ];
                                        case ArgumentType.Zero:
                                            return [
                                                getJumpOpcode(ArgumentType.Zero, cond.negated),
                                                address & 0xFF, (address >> 8) & 0xFF
                                            ];
                                        default:
                                    }
                                }
                                error(
                                    "unrecognized condition '" ~ attr.fullName()
                                    ~ "' used in 'when' clause of 'goto'", cond.location
                                );
                                return [];
                            }
                        }
                    case ArgumentType.HL:
                        if(stmt.condition is null)
                        {
                            return [0xE9];
                        }
                        else
                        {
                            error("'goto hl' does not support 'when' clause", stmt.condition.location);
                            return [];
                        }
                    default: return [];
                }
            case parse.Keyword.Call:
                auto argument = buildArgument(program, stmt.destination);
                switch(argument.type)
                {
                    case ArgumentType.Immediate:
                        uint address;
                        if(!compile.foldConstExpr(program, argument.immediate, address))
                        {
                            // For now, sub a placeholder expression.
                            // At a later pass, we will ensure that the address is resolvable.
                            address = 0xFACE;
                        }
                        if(stmt.condition is null)
                        {
                            return [getCallOpcode(), address & 0xFF, (address >> 8) & 0xFF];
                        }
                        else
                        {
                            auto cond = stmt.condition;
                            if(cond.attr is null)
                            {
                                ubyte opcode;
                                final switch(cond.branch)
                                {
                                    case parse.Branch.Equal:
                                        opcode = getCallOpcode(ArgumentType.Zero, cond.negated);
                                    case parse.Branch.NotEqual:
                                        opcode = getCallOpcode(ArgumentType.Zero, !cond.negated);
                                    case parse.Branch.Less:
                                        opcode = getCallOpcode(ArgumentType.Carry, !cond.negated);
                                    case parse.Branch.GreaterEqual:
                                        opcode = getCallOpcode(ArgumentType.Carry, cond.negated);
                                    case parse.Branch.Greater:
                                    case parse.Branch.LessEqual:
                                        error(
                                            "comparision '" ~ parse.getBranchName(cond.branch)
                                            ~ "' unsupported in 'when' clause of 'call'", cond.location
                                        );
                                        return [];
                                }
                                return [opcode, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            else
                            {
                                auto attr = cond.attr;
                                auto def = compile.resolveAttribute(program, attr);
                                if(auto builtin = cast(Builtin) def)
                                {
                                    switch(builtin.type)
                                    {
                                        case ArgumentType.Carry:
                                            return [
                                                getCallOpcode(ArgumentType.Carry, cond.negated),
                                                address & 0xFF, (address >> 8) & 0xFF
                                            ];
                                        case ArgumentType.Zero:
                                            return [
                                                getCallOpcode(ArgumentType.Zero, cond.negated),
                                                address & 0xFF, (address >> 8) & 0xFF
                                            ];
                                        default:
                                    }
                                }
                                error(
                                    "unrecognized condition '" ~ attr.fullName()
                                    ~ "' used in 'when' clause of 'call'", cond.location
                                );
                                return [];
                            }
                        }
                    default: return [];
                }
            case parse.Keyword.Return:
                if(stmt.condition is null)
                {
                    return [getReturnOpcode()];
                }
                else
                {
                    auto cond = stmt.condition;
                    if(cond.attr is null)
                    {
                        ubyte opcode;
                        final switch(cond.branch)
                        {
                            case parse.Branch.Equal:
                                return [getReturnOpcode(ArgumentType.Zero, cond.negated)];
                            case parse.Branch.NotEqual:
                                return [getReturnOpcode(ArgumentType.Zero, !cond.negated)];
                            case parse.Branch.Less:
                                return [getReturnOpcode(ArgumentType.Carry, !cond.negated)];
                            case parse.Branch.GreaterEqual:
                                return [getReturnOpcode(ArgumentType.Carry, cond.negated)];
                            case parse.Branch.Greater:
                            case parse.Branch.LessEqual:
                                error(
                                    "comparision '" ~ parse.getBranchName(cond.branch)
                                    ~ "' unsupported in 'when' clause of 'return'", cond.location
                                );
                                return [];
                        }
                    }
                    else
                    {
                        auto attr = cond.attr;
                        auto def = compile.resolveAttribute(program, attr);
                        if(auto builtin = cast(Builtin) def)
                        {
                            switch(builtin.type)
                            {
                                case ArgumentType.Carry:
                                    return [getReturnOpcode(ArgumentType.Zero, cond.negated)];
                                case ArgumentType.Zero:
                                    return [getReturnOpcode(ArgumentType.Carry, cond.negated)];
                                default:
                            }
                        }
                        error(
                            "unrecognized condition '" ~ attr.fullName()
                            ~ "' used in 'when' clause of 'return'", cond.location
                        );
                        return [];
                    }
                }
            case parse.Keyword.Resume:
                return [0xD9];
            case parse.Keyword.Break:
                return [];
            case parse.Keyword.Continue:
                return [];
            case parse.Keyword.While:
                return [];
            case parse.Keyword.Until:
                return [];
            case parse.Keyword.Abort:
                return [0x40];
            case parse.Keyword.Sleep:
                return [0x76];
            case parse.Keyword.Suspend:
                return [0x10, 0x00];
            case parse.Keyword.Nop:
                return [0x00];
            default:
                return [];
        }
    }

    ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt)
    {
        return [];
    }
}