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
        ubyte[] getJumpCode(uint address, ArgumentType type = ArgumentType.None, bool negated = false)
        {
            ubyte opcode;
            switch(type)
            {
                case ArgumentType.Zero: opcode = negated ? 0xC2 : 0xCA; break;
                case ArgumentType.Carry: opcode = negated ? 0xD2 : 0xDA; break;
                case ArgumentType.None: opcode = 0xC3; break;
                default:
                    assert(0);
            }
            return [opcode, address & 0xFF, (address >> 8) & 0xFF];
        }

        ubyte[] getCallCode(uint address, ArgumentType type = ArgumentType.None, bool negated = false)
        {
            ubyte opcode;
            switch(type)
            {
                case ArgumentType.Zero: opcode = negated ? 0xC4 : 0xCC; break;
                case ArgumentType.Carry: opcode = negated ? 0xD4 : 0xDC; break;
                case ArgumentType.None: opcode = 0xCD; break;
                default:
                    assert(0);
            }
            return [opcode, address & 0xFF, (address >> 8) & 0xFF];
        }

        ubyte[] getReturnCode(ArgumentType type = ArgumentType.None, bool negated = false)
        {
            switch(type)
            {
                case ArgumentType.Zero: return [negated ? 0xC0 : 0xC8];
                case ArgumentType.Carry: return [negated ? 0xD0 : 0xD8];
                case ArgumentType.None: return [0xC9];
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
                            return getJumpCode(address);
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
                                        return getJumpCode(address, ArgumentType.Zero, cond.negated);
                                    case parse.Branch.NotEqual:
                                        return getJumpCode(address, ArgumentType.Zero, !cond.negated);
                                    case parse.Branch.Less:
                                        return getJumpCode(address, ArgumentType.Carry, !cond.negated);
                                    case parse.Branch.GreaterEqual:
                                        return getJumpCode(address, ArgumentType.Carry, cond.negated);
                                    case parse.Branch.Greater:
                                    case parse.Branch.LessEqual:
                                        error(
                                            "comparision '" ~ parse.getBranchName(cond.branch)
                                            ~ "' unsupported in 'when' clause of 'goto'", cond.location
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
                                        case ArgumentType.Zero:
                                            return getJumpCode(address, builtin.type, cond.negated);                                        
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
                    default:
                        error("unsupported argument to 'goto'", stmt.condition.location);
                        return [];
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
                            return getCallCode(address);
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
                                        return getCallCode(address, ArgumentType.Zero, cond.negated);
                                    case parse.Branch.NotEqual:
                                        return getCallCode(address, ArgumentType.Zero, !cond.negated);
                                    case parse.Branch.Less:
                                        return getCallCode(address, ArgumentType.Carry, !cond.negated);
                                    case parse.Branch.GreaterEqual:
                                        return getCallCode(address, ArgumentType.Carry, cond.negated);
                                    case parse.Branch.Greater:
                                    case parse.Branch.LessEqual:
                                        error(
                                            "comparision '" ~ parse.getBranchName(cond.branch)
                                            ~ "' unsupported in 'when' clause of 'call'", cond.location
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
                                        case ArgumentType.Zero:
                                            return getCallCode(address, builtin.type, cond.negated);
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
                    default:
                        error("unsupported argument to 'call'", stmt.condition.location);
                        return [];
                }
            case parse.Keyword.Return:
                if(stmt.condition is null)
                {
                    return getReturnCode();
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
                                return getReturnCode(ArgumentType.Zero, cond.negated);
                            case parse.Branch.NotEqual:
                                return getReturnCode(ArgumentType.Zero, !cond.negated);
                            case parse.Branch.Less:
                                return getReturnCode(ArgumentType.Carry, !cond.negated);
                            case parse.Branch.GreaterEqual:
                                return getReturnCode(ArgumentType.Carry, cond.negated);
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
                                case ArgumentType.Zero:
                                    return getReturnCode(builtin.type, cond.negated);
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

    ubyte[] generateComparison(compile.Program program, ast.Comparison stmt)
    {
        auto left = buildArgument(program, stmt.left);
        auto right = stmt.right ? buildArgument(program, stmt.right) : null;
        switch(left.type)
        {
            case ArgumentType.A:
                if(right is null)
                {
                    // 'or a'
                    return [0xB7];
                }
                else
                {
                    // 'cp a, expr'
                    switch(right.type)
                    {
                        case ArgumentType.Immediate:
                            uint value;
                            if(!compile.foldConstExpr(program, right.immediate, value))
                            {
                                // For now, sub a placeholder expression.
                                // At a later pass, we will ensure that the address is resolvable.
                                value = 0xFACE;
                            }
                            return [0xFE, value & 0xFF];
                        case ArgumentType.B: return [0xB8];
                        case ArgumentType.C: return [0xB9];
                        case ArgumentType.D: return [0xBA];
                        case ArgumentType.E: return [0xBB];
                        case ArgumentType.H: return [0xBC];
                        case ArgumentType.L: return [0xBD];
                        case ArgumentType.Indirection:
                            if(left.base.base.type == ArgumentType.HL)
                            {
                                return [0xBE];
                            }
                            else
                            {
                                error("indirected operand on left-hand side of '@' is not supported (only 'hl' is valid)", stmt.right.location);
                                return [];
                            }
                        case ArgumentType.A: return [0xBF];
                        default:
                            error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                            return [];
                    }
                }
            case ArgumentType.BitIndex:
                if(right is null)
                {
                    uint index;
                    if(!compile.foldConstExpr(program, left.immediate, index))
                    {
                        return [];
                    }
                    else
                    {
                        if(index > 7)
                        {
                            error("right-hand side of '@' must be in the range 0..7.", left.immediate.location);
                        }
                        ubyte r;
                        switch(left.base.type)
                        {
                            case ArgumentType.B: r = 0x0; break;
                            case ArgumentType.C: r = 0x1; break;
                            case ArgumentType.D: r = 0x2; break;
                            case ArgumentType.E: r = 0x3; break;
                            case ArgumentType.H: r = 0x4; break;
                            case ArgumentType.L: r = 0x5; break;
                            case ArgumentType.Indirection:
                                if(left.base.base.type == ArgumentType.HL)
                                {
                                    r = 0x6;
                                }
                                else
                                {
                                    error("indirected operand on left-hand side of '@' is not supported (only 'hl' is valid)", stmt.right.location);
                                    return [];
                                }
                                break;
                            case ArgumentType.A: r = 0x7; break;
                            default:
                                return [];
                        }
                        // 'bit r, i'
                        return [0xCB, (0x40 + (0x08 * index) + r) & 0xFF];
                    }
                }
                else
                {
                    error("'to' clause is unsupported for 'compare ... @ ...'", stmt.right.location);
                    return [];
                }
            default:
                return [];
        }
    }
}