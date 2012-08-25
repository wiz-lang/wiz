module wiz.cpu.gameboy;

import wiz.lib;
import wiz.cpu.lib;

static import std.array;

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

    ubyte getRegisterIndex(Argument dest)
    {
        switch(dest.type)
        {
            case ArgumentType.B: return 0x0; break;
            case ArgumentType.C: return 0x1; break;
            case ArgumentType.D: return 0x2; break;
            case ArgumentType.E: return 0x3; break;
            case ArgumentType.H: return 0x4; break;
            case ArgumentType.L: return 0x5; break;
            case ArgumentType.Indirection:
                if(dest.base.type == ArgumentType.HL)
                {
                    return 0x6;
                }
                break;
            case ArgumentType.A: return 0x7; break;
            default:
        }
        assert(0);
    }

    ubyte getPairIndex(Argument dest)
    {
        switch(dest.type)
        {
            case ArgumentType.BC: return 0x0; break;
            case ArgumentType.DE: return 0x1; break;
            case ArgumentType.HL: return 0x2; break;
            case ArgumentType.SP: return 0x3; break;
            default:
        }
        assert(0);
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

            if(infix.type == parse.Infix.Colon)
            {
                if(auto attr = cast(ast.Attribute) infix.right)
                {
                    auto def = compile.resolveAttribute(program, attr);
                    if(!def)
                    {
                        return null;
                    }
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
            if(infix.type == parse.Token.At)
            {
                if(auto attr = cast(ast.Attribute) infix.left)
                {
                    auto def = compile.resolveAttribute(program, attr);
                    if(!def)
                    {
                        return null;
                    }
                    if(auto builtin = cast(Builtin) def)
                    {
                        return new Argument(ArgumentType.BitIndex, infix.right, new Argument(builtin.type));
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
        ubyte[] getRelJumpCode(uint address, ArgumentType type = ArgumentType.None, bool negated = false)
        {
            ubyte opcode;
            switch(type)
            {
                case ArgumentType.Zero: opcode = negated ? 0x20 : 0x28; break;
                case ArgumentType.Carry: opcode = negated ? 0x30 : 0x38; break;
                case ArgumentType.None: opcode = 0x18; break;
                default:
                    assert(0);
            }
            return [opcode, address & 0xFF];
        }

        ubyte[] getAbsJumpCode(uint address, ArgumentType type = ArgumentType.None, bool negated = false)
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

        ubyte[] getJumpCode(bool far, uint address, ArgumentType type = ArgumentType.None, bool negated = false)
        {
            if(far)
            {
                return getAbsJumpCode(address, type, negated);
            }
            else
            {
                return getRelJumpCode(address, type, negated);
            }
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

        bool resolveBranchFlags(string context, ast.JumpCondition cond, ref ArgumentType flag, ref bool negated)
        {
            final switch(cond.branch)
            {
                case parse.Branch.Equal:
                    flag = ArgumentType.Zero;
                    return true;
                case parse.Branch.NotEqual:
                    flag = ArgumentType.Zero;
                    negated = false;
                    return true;
                case parse.Branch.Less:
                    flag = ArgumentType.Carry;
                    negated = false;
                    return true;
                case parse.Branch.GreaterEqual:
                    flag = ArgumentType.Carry;
                    return true;
                case parse.Branch.Greater:
                case parse.Branch.LessEqual:
                    error(
                        "comparision '" ~ parse.getBranchName(cond.branch)
                        ~ "' unsupported in 'when' clause of " ~ context, cond.location
                    );
                    return false;
            }
        }

        switch(stmt.type)
        {
            case parse.Keyword.Goto:
                auto argument = buildArgument(program, stmt.destination);
                if(!argument)
                {
                    return [];
                }
                switch(argument.type)
                {
                    case ArgumentType.Immediate:
                        uint address;
                        compile.foldConstExpr(program, argument.immediate, address);
                        if(stmt.condition is null)
                        {
                            return getJumpCode(stmt.far, address);
                        }
                        else
                        {
                            auto cond = stmt.condition;
                            if(cond.attr is null)
                            {
                                ArgumentType flag;
                                bool negated;
                                if(resolveBranchFlags("'goto'", cond, flag, negated))
                                {
                                    return getJumpCode(stmt.far, address, flag, negated);
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
                                            return getJumpCode(stmt.far, address, builtin.type, cond.negated);                                        
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
                if(!argument)
                {
                    return [];
                }
                switch(argument.type)
                {
                    case ArgumentType.Immediate:
                        uint address;
                        compile.foldConstExpr(program, argument.immediate, address);
                        if(stmt.condition is null)
                        {
                            return getCallCode(address);
                        }
                        else
                        {
                            auto cond = stmt.condition;
                            if(cond.attr is null)
                            {
                                ArgumentType flag;
                                bool negated;
                                if(resolveBranchFlags("'call'", cond, flag, negated))
                                {
                                    return getCallCode(address, ArgumentType.Zero, cond.negated);
                                }
                                return [];
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
        auto dest = buildArgument(program, stmt.dest);
        if(stmt.src is null)
        {
            final switch(stmt.postfix)
            {
                case parse.Postfix.Inc:
                    // 'inc dest'
                    switch(dest.type)
                    {
                        case ArgumentType.B: return [0x04];
                        case ArgumentType.C: return [0x0C];
                        case ArgumentType.D: return [0x14];
                        case ArgumentType.E: return [0x1C];
                        case ArgumentType.H: return [0x24];
                        case ArgumentType.L: return [0x2C];
                        case ArgumentType.Indirection:
                            if(dest.base.type == ArgumentType.HL)
                            {
                                return [0x34];
                            }
                            else
                            {
                                error("'++' on indirected operand is not supported (only '[hl]++' is valid)", stmt.dest.location);
                                return [];
                            }
                        case ArgumentType.A: return [0x3C];
                        case ArgumentType.BC: return [0x03];
                        case ArgumentType.DE: return [0x13];
                        case ArgumentType.HL: return [0x23];
                        case ArgumentType.SP: return [0x33];
                        default:
                            error("unsupported operand of '++'", stmt.dest.location);
                            return [];
                    }
                case parse.Postfix.Dec:
                    // 'dec dest'
                    switch(dest.type)
                    {
                        case ArgumentType.B: return [0x05];
                        case ArgumentType.C: return [0x0D];
                        case ArgumentType.D: return [0x15];
                        case ArgumentType.E: return [0x1D];
                        case ArgumentType.H: return [0x25];
                        case ArgumentType.L: return [0x2D];
                        case ArgumentType.Indirection:
                            if(dest.base.type == ArgumentType.HL)
                            {
                                return [0x35];
                            }
                            else
                            {
                                error("'--' on indirected operand is not supported (only '[hl]--' is valid)", stmt.dest.location);
                                return [];
                            }
                        case ArgumentType.A: return [0x3D];
                        case ArgumentType.BC: return [0x0B];
                        case ArgumentType.DE: return [0x1B];
                        case ArgumentType.HL: return [0x2B];
                        case ArgumentType.SP: return [0x3B];
                        default:
                            error("unsupported operand of '--'", stmt.dest.location);
                            return [];
                    }
            }
        }
        else
        {
            if(stmt.intermediary)
            {
                auto intermediary = buildArgument(program, stmt.intermediary);
                return // 'x = y via z' -> 'z = y; x = z'
                    generateCalculation(program, stmt, intermediary, stmt.src)
                    ~ generateLoad(program, stmt, dest, stmt.intermediary);
            }
            else
            {
                return generateCalculation(program, stmt, dest, stmt.src);
            }
        }
        return [];
    }

    ubyte[] generateCalculation(compile.Program program, ast.Assignment stmt, Argument dest, ast.Expression src)
    {
        ubyte[] code = generateLoad(program, stmt, dest, src);
        if(dest is null)
        {
            return [];
        }
        // TODO: This code will be gigantic.
        // a:
        //      + (add), - (sub), +# (adc), -# (sbc),
        //      & (and), | (or), ^ (xor),
        //      <<< (sla), <<- (sla), >>> (srl) >>- (sra)
        //      <<< (rla), <<<# (rlca), >>> (rra), >>># (rrca).
        // r / [hl]:
        //      <<< (sla), <<- (sla), >>> (srl) >>- (sra)
        //      <<< (rl), <<<# (rlc), >>> (rr), >>># (rrc).
        // carry:
        //      ^ (ccf)
        return code;
    }

    ubyte[] generateLoad(compile.Program program, ast.Assignment stmt, Argument dest, ast.Expression src)
    {
        ubyte[] invalidAssignment(string desc)
        {
            error("invalid assignment '=' to " ~ desc ~ ".", stmt.location);
            return [];
        }

        // A sequence of instructions, to be executed in reverse order
        // This way, prefix operators run after the dest has been loaded with values they act on.
        ubyte[][] code;

        // TODO: fold constant left-hand of expressions.
        if(dest is null)
        {
            return [];
        }
        final switch(dest.type)
        {
            case ArgumentType.A:
                if(auto load = buildArgument(program, src))
                {
                    while(true)
                    {
                        if(load.type == ArgumentType.Swap)
                        {
                            // 'a = <>a' -> 'swap a'
                            code ~= [0xCB, 0x37];
                            load = load.base;
                            continue;
                        }
                        if(load.type == ArgumentType.Not)
                        {
                            // 'a = ~a' -> 'cpl a'
                            code ~= [0x2F];
                            load = load.base;
                            continue;
                        }
                        break;
                    }
                    switch(load.type)
                    {
                        // 'a = n' -> 'ld a, n'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            code ~= [0x3E, value & 0xFF];
                            break;
                        // 'a = a' -> (nothing)
                        case ArgumentType.A:
                            break;
                        // 'a = r' -> 'ld a, r'
                        case ArgumentType.B:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            ubyte sourceIndex = getRegisterIndex(load);
                            code ~= [(0x78 + sourceIndex) & 0xFF];
                            break;
                        case ArgumentType.Indirection:
                            switch(load.base.type)
                            {
                                case ArgumentType.Immediate:
                                    uint value;
                                    compile.foldConstExpr(program, load.base.immediate, value);
                                    // 'a = [0xFFnn]' -> 'ldh a, [nn]'
                                    if((value & 0xFF00) == 0xFF00)
                                    {
                                        code ~= [0xF0, value & 0xFF];
                                    }
                                    // 'a = [nnnn]' -> 'ld a, [nnnn]'
                                    else
                                    {
                                        code ~= [0xFA, value & 0xFF, (value >> 8) & 0xFF];
                                    }
                                    break;
                                // 'a = [hl]' -> 'ld a, [hl]'
                                case ArgumentType.HL: code ~= [0x7E]; break;
                                // 'a = [bc]' -> 'ld a, [bc]'
                                case ArgumentType.BC: code ~= [0x0A]; break;
                                // 'a = [de]' -> 'ld a, [de]'
                                case ArgumentType.DE: code ~= [0x1A]; break;
                                default: return invalidAssignment("'a'");
                            }
                            break;
                        case ArgumentType.IndirectionInc:
                            switch(load.base.type)
                            {
                                // 'a = [hl++]' -> 'ldi a, [hl]'
                                case ArgumentType.HL: code ~= [0x2A]; break;
                                default: return invalidAssignment("'a'");
                            }
                            break;
                        case ArgumentType.IndirectionDec:
                            switch(load.base.type)
                            {
                                // 'a = [hl--]' -> 'ldd a, [hl]'
                                case ArgumentType.HL: code ~= [0x2A]; break;
                                default: return invalidAssignment("'a'");
                            }
                            break;
                        case ArgumentType.PositiveIndex:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);

                            if(value != 0xFF00 || load.base.type != ArgumentType.C)
                            {
                                return invalidAssignment("'a'");
                            }
                            else
                            {
                                code ~= [0xF2];
                            }
                            break;
                        case ArgumentType.NegativeIndex:
                            error("negative indexing is not supported.", stmt.location);
                            break;
                        default: return invalidAssignment("'a'");
                    }
                }
                break;
            case ArgumentType.B:
            case ArgumentType.C:
            case ArgumentType.D:
            case ArgumentType.E:
            case ArgumentType.H:
            case ArgumentType.L:
                if(auto load = buildArgument(program, src))
                {
                    ubyte destIndex = getRegisterIndex(dest);
                    while(true)
                    {
                        if(load.type == ArgumentType.Swap)
                        {
                            // 'r = <>r' -> 'swap r'
                            code ~= [0xCB, (0x20 + destIndex) & 0xFF];
                            load = load.base;
                            continue;
                        }
                        break;
                    }
                    switch(load.type)
                    {
                        // 'r = n' -> 'ld r, n'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            code ~= [(0x06 + destIndex * 8) & 0xFF, value & 0xFF];
                            break;
                        // 'r = r' -> (nothing)
                        // 'r = r2' -> 'ld r, r2'
                        case ArgumentType.A: 
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            ubyte sourceIndex = getRegisterIndex(load);
                            if(sourceIndex != destIndex)
                            {
                                code ~= [(0x40 + destIndex * 8 + sourceIndex) & 0xFF];
                            }
                            break;
                        case ArgumentType.Indirection:
                            switch(load.base.type)
                            {
                                // 'r = [hl]' -> 'ld r, [hl]'
                                case ArgumentType.HL:
                                    ubyte sourceIndex = getRegisterIndex(load);
                                    code ~= [(0x40 + destIndex * 8 + sourceIndex) & 0xFF];
                                    break;
                                default: return invalidAssignment("register");
                            }
                            break;
                        default: return invalidAssignment("register");
                    }
                }
                break;
            case ArgumentType.Indirection:
                switch(dest.base.type)
                {
                    case ArgumentType.Immediate:
                        if(auto load = buildArgument(program, src))
                        {
                            if(load.type == ArgumentType.A)
                            {
                                uint value;
                                compile.foldConstExpr(program, dest.base.immediate, value);
                                // '[0xFFnn] = a' -> 'ldh [nn], a'
                                if((value & 0xFF00) == 0xFF00)
                                {
                                    code ~= [0xE0, value & 0xFF];
                                }
                                // '[nnnn] = a' -> 'ld [nnnn], a'
                                else
                                {
                                    code ~= [0xEA, value & 0xFF, (value >> 8) & 0xFF];
                                }
                                break;
                            }
                            else
                            {
                                return invalidAssignment("indirected immediate (can only be assigned 'a')");
                            }
                        }
                        break;
                    case ArgumentType.BC:
                        // '[bc] = a' -> 'ld [bc], a'
                        if(auto load = buildArgument(program, src))
                        {
                            if(load.type == ArgumentType.A)
                            {
                                code ~= [0x02];
                            }
                        }
                        else
                        {
                            return invalidAssignment("'[bc]' (can only be assigned 'a')");
                        }
                        break;
                    case ArgumentType.DE:
                        // '[de] = a' -> 'ld [de], a'
                        if(auto load = buildArgument(program, src))
                        {
                            if(load.type == ArgumentType.A)
                            {
                                code ~= [0x12];
                            }
                        }
                        else
                        {
                            return invalidAssignment("'[bc]' (can only be assigned 'a')");
                        }
                        break;
                    case ArgumentType.HL:
                        ubyte destIndex = getRegisterIndex(dest);
                        if(auto load = buildArgument(program, src))
                        {
                            while(true)
                            {
                                if(load.type == ArgumentType.Swap)
                                {
                                    // 'r = <>r' -> 'swap r'
                                    code ~= [0xCB, (0x20 + destIndex) & 0xFF];
                                    load = load.base;
                                    continue;
                                }
                                break;
                            }
                            switch(load.type)
                            {
                                // '[hl] = n' -> 'ld [hl], n'
                                case ArgumentType.Immediate:
                                    uint value;
                                    compile.foldConstExpr(program, load.immediate, value);
                                    code ~= [(0x06 + destIndex * 8) & 0xFF, value & 0xFF];
                                    break;
                                // '[hl] = r' -> 'ld [hl], r'
                                case ArgumentType.A: 
                                case ArgumentType.B:
                                case ArgumentType.C:
                                case ArgumentType.D:
                                case ArgumentType.E:
                                case ArgumentType.H:
                                case ArgumentType.L:
                                    ubyte sourceIndex = getRegisterIndex(load);
                                    if(sourceIndex != destIndex)
                                    {
                                        code ~= [(0x40 + destIndex * 8 + sourceIndex) & 0xFF];
                                    }
                                    break;
                                case ArgumentType.Indirection:
                                    switch(load.base.type)
                                    {
                                        // '[hl] = [hl]' -> (nothing)
                                        case ArgumentType.HL:
                                            break;
                                        default: return invalidAssignment("'[hl]'");
                                    }
                                    break;
                                default:
                                    return invalidAssignment("'[hl]'");
                            }
                        }
                        else
                        {
                            return invalidAssignment("'[hl]'");
                        }
                        break;
                    default:
                        error("invalid assignment '=' to indirected expression.", stmt.location);
                        return [];
                }
                break;
            case ArgumentType.AF:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        // 'af = pop' -> 'pop af'
                        case ArgumentType.Pop:
                            code ~= [0xF1];
                            break;
                        default:
                            return invalidAssignment("'af'");
                    }
                }
                break;
            case ArgumentType.BC:
            case ArgumentType.DE:
            case ArgumentType.HL:
                if(auto load = buildArgument(program, src))
                {
                    ubyte destIndex = getPairIndex(dest);
                    switch(load.type)
                    {
                        // 'rr = n' -> 'ld rr, n'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            code ~= [(0x01 + destIndex * 16) & 0xFF, value & 0xFF, (value >> 8) & 0xFF];
                            break;
                        // 'rr = pop' -> 'pop rr'
                        case ArgumentType.Pop:
                            code ~= [(0xC1 + destIndex * 16) & 0xFF];
                            break;
                        case ArgumentType.SP:
                            if(dest.type == ArgumentType.HL)
                            {
                                // 'hl = sp' -> 'ldhl sp,0'
                                // (TODO: grab additive constant directly after sp. 'hl = sp + k' -> 'ldhl sp, k'
                                code ~= [0xF8, 0x00];
                            }
                            else
                            {
                                return invalidAssignment("register pair (only 'hl' or 'sp' can be assigned to 'sp')");
                            }
                            break;
                        default:
                            return invalidAssignment("register pair");
                    }
                }
                break;
            case ArgumentType.SP:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        // 'sp = n' -> 'ld sp, n'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            code ~= [(0x01 + getPairIndex(dest) * 16) & 0xFF, value & 0xFF, (value >> 8) & 0xFF];
                            break;
                        // 'sp = hl' -> 'ld sp, hl'
                        case ArgumentType.HL:
                            code ~= [0xF9];
                            break;
                        default:
                            return invalidAssignment("'sp'");
                    }
                }
                break;
            case ArgumentType.IndirectionInc:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        case ArgumentType.A:
                            // '[hl++] = a' -> 'ld [hl+], a'
                            code ~= [0x22];
                            break;
                        default:
                            return invalidAssignment("'[hl++]'");
                    }
                }
                break;
            case ArgumentType.IndirectionDec:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        case ArgumentType.A:
                            // '[hl--] = a' -> 'ld [hl-], a'
                            code ~= [0x32];
                            break;
                        default:
                            return invalidAssignment("'[hl--]'");
                    }
                }
                break;
            case ArgumentType.PositiveIndex:
                uint value;
                compile.foldConstExpr(program, dest.immediate, value);

                if(value != 0xFF00 || dest.base.type != ArgumentType.C)
                {
                    error("assignment '=' to indexed memory location other than '[0xFF00:c]' is invalid.", stmt.location);
                    return [];
                }
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        // '[FF00:c]' -> 'ldh [c], a'
                        case ArgumentType.A:
                            code ~= [0xE2];
                            break;
                        default:
                            return invalidAssignment("'[0xFF00:c]'");
                    }
                }
                break;
            case ArgumentType.NegativeIndex:
                error("negative indexing is not supported.", stmt.location);
                return [];
            case ArgumentType.BitIndex:
                uint index;
                compile.foldConstExpr(program, dest.immediate, index);
                if(index > 7)
                {
                    error("right-hand side of '@' must be in the range 0..7.", dest.immediate.location);
                }
                dest = dest.base;
                switch(dest.type)
                {
                    case ArgumentType.A: break;
                    case ArgumentType.B: break;
                    case ArgumentType.C: break;
                    case ArgumentType.D: break;
                    case ArgumentType.E: break;
                    case ArgumentType.H: break;
                    case ArgumentType.L: break;
                    case ArgumentType.Indirection:
                        if(dest.base.type != ArgumentType.HL)
                        {
                            error("indirected operand on left-hand side of '@' is not supported (only '[hl] @ ...' is valid)", stmt.location);
                            return [];
                        }
                        break;
                    default:
                        error("unsupported operand on left-hand side of '@'", stmt.location);
                        return [];
                }
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        // 'r@i = 0' -> 'res r, i'
                        // 'r@i = 1' -> 'set r, i'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            if(value == 0)
                            {
                                code ~= [0xCB, (0x80 + index * 8 + getRegisterIndex(dest)) & 0xFF];
                            }
                            else if(value == 1)
                            {
                                code ~= [0xCB, (0xC0 + index * 8 + getRegisterIndex(dest)) & 0xFF];
                            }
                            else
                            {
                                return invalidAssignment("bit-indexed register (immediate must be 0 or 1, not " ~ std.conv.to!string(value) ~ ")");
                            }
                            break;
                        default:
                            return invalidAssignment("bit-indexed register");
                    }
                }
                break;
            case ArgumentType.Interrupt:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        // 'interrupt = 0' -> 'di'
                        // 'interrupt = 1' -> 'ei'
                        case ArgumentType.Immediate:
                            uint value;
                            compile.foldConstExpr(program, load.immediate, value);
                            if(value == 0)
                            {
                                code ~= [0xF3];
                            }
                            else if(value == 1)
                            {
                                code ~= [0xFB];
                            }
                            else
                            {
                                return invalidAssignment("'interrupt' (immediate must be 0 or 1, not " ~ std.conv.to!string(value) ~ ")");
                            }
                            break;
                        default:
                            return invalidAssignment("'interrupt'");
                    }
                }
                break;
            case ArgumentType.Immediate:
                error("assignment '=' to immediate constant is invalid.", stmt.location);
                return [];
            case ArgumentType.Not:
                error("assignment '=' to not expression '~' is invalid.", stmt.location);
                return [];
            case ArgumentType.Swap:
                error("assignment '=' to swap expression '<>' is invalid.", stmt.location);
                return [];
            case ArgumentType.F:
                error("assignment '=' to 'f' register is invalid.", stmt.location);
                return [];
            case ArgumentType.Zero:
                error("assignment '=' to 'zero' flag is invalid.", stmt.location);
                return [];
            case ArgumentType.Carry:
                if(auto load = buildArgument(program, src))
                {
                    while(true)
                    {
                        if(load.type == ArgumentType.Not)
                        {
                            // 'carry = ~carry' -> 'ccf'
                            code ~= [0x3F];
                            load = load.base;
                            continue;
                        }
                        break;
                    }
                    switch(load.type)
                    {
                        case ArgumentType.Immediate:
                            uint value;
                            if(compile.foldConstExpr(program, load.immediate, value))
                            {
                                // 'carry = 0' -> 'scf; ccf'    
                                if(value == 0)
                                {
                                    code ~= [0x37, 0x3F];
                                    
                                }
                                // 'carry = 1' -> 'scf'
                                else if(value == 1)
                                {
                                    code ~= [0x37];
                                }
                                else
                                {
                                    return invalidAssignment("'carry' (immediate must be 0 or 1, not " ~ std.conv.to!string(value) ~ ")");
                                }
                            }
                            break;
                        case ArgumentType.Carry:
                            break;
                        default:
                    }
                }
                break;
            case ArgumentType.Pop:
                error("'pop' operator on left hand of assignment '=' is invalid.", stmt.location);
                return [];
            case ArgumentType.None:
                return [];
        }
        return std.array.join(code.reverse);
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
                    // 'compare a' -> 'or a'
                    return [0xB7];
                }
                else
                {
                    // 'compare a to expr' -> 'cp a, expr'
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
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            return [(0xB8 + getRegisterIndex(left)) & 0xFF];
                        case ArgumentType.Indirection:
                            if(right.base.type == ArgumentType.HL)
                            {
                                return [0xBE];
                            }
                            else
                            {
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
                if(right is null)
                {
                    uint index;
                    compile.foldConstExpr(program, left.immediate, index);
                    if(index > 7)
                    {
                        error("right-hand side of '@' must be in the range 0..7.", left.immediate.location);
                    }
                    left = left.base;
                    switch(left.type)
                    {
                        case ArgumentType.A: break;
                        case ArgumentType.B: break;
                        case ArgumentType.C: break;
                        case ArgumentType.D: break;
                        case ArgumentType.E: break;
                        case ArgumentType.H: break;
                        case ArgumentType.L: break;
                        case ArgumentType.Indirection:
                            if(left.base.type != ArgumentType.HL)
                            {
                                error("indirected operand on left-hand side of '@' is not supported (only '[hl] @ ...' is valid)", stmt.right.location);
                                return [];
                            }
                            break;
                        default:
                            error("unsupported operand on left-hand side of '@'", stmt.right.location);
                            return [];
                    }
                    return [0xCB, (0x40 + index * 8 + getRegisterIndex(left)) & 0xFF];
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