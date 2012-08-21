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
                            if(dest.base.base.type == ArgumentType.HL)
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
                            if(dest.base.base.type == ArgumentType.HL)
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
        // TODO: fold constant left-hand of expressions.
        // TODO: giant fucking assignment compatibility instruction lookup table.
        if(dest is null)
        {
            return [];
        }
        final switch(dest.type)
        {
            case ArgumentType.A:
                // 'a = <>a' -> 'swap a'
                // 'a = ~a' -> 'cpl a'
                // 'a = r' -> 'ld a, r'
                // 'a = n' -> 'ld a, n'
                // 'a = [hl]' -> 'ld a, [hl]'
                // 'a = [bc]' -> 'ld a, [bc]'
                // 'a = [de]' -> 'ld a, [de]'
                // 'a = [n]' -> 'ld a, [n]'
                // 'a = [0xFFnn]' -> 'ldh a, [nn]'
                // 'a = [0xFF00:c]' -> 'ldh a, [c]'
                // 'a = [hl++]' -> 'ld a, [hl+]'
                // 'a = [hl--]' -> 'ld a, [hl-]'
                return [];
            case ArgumentType.B:
                // 'b = <>b' -> 'swap b'
                // 'b = r' -> 'ld b, r'
                // 'b = n' -> 'ld b, n'
                // 'b = [hl]' -> 'ld b, [hl]'
                return [];
            case ArgumentType.C:
                // 'c = <>c' -> 'swap c'
                // 'c = r' -> 'ld c, r'
                // 'c = n' -> 'ld c, n'
                // 'c = [hl]' -> 'ld c, [hl]'
                return [];
            case ArgumentType.D:
                // 'd = <>d' -> 'swap d'
                // 'd = r' -> 'ld d, r'
                // 'd = n' -> 'ld d, n'
                // 'd = [hl]' -> 'ld d, [hl]'
                return [];
            case ArgumentType.E:
                // 'e = <>e' -> 'swap e'
                // 'e = r' -> 'ld e, r'
                // 'e = n' -> 'ld e, n'
                // 'e = [hl]' -> 'ld e, [hl]'
                return [];
            case ArgumentType.H:
                // 'h = <>h' -> 'swap h'
                // 'h = r' -> 'ld h, r'
                // 'h = n' -> 'ld h, n'
                // 'h = [hl]' -> 'ld h, [hl]'
                return [];
            case ArgumentType.L:
                // 'l = <>l' -> 'swap l'
                // 'l = r' -> 'ld l, r'
                // 'l = n' -> 'ld l, n'
                // 'l = [hl]' -> 'ld l, [hl]'
                return [];
            case ArgumentType.Indirection:
                switch(dest.base.type)
                {
                    case ArgumentType.Immediate:
                        // '[n] = a' -> 'ld [n], a'
                        // '[0xFFnn] = a' -> 'ldh [nn], a'
                        error("TODO: assignment to indirected immediate", stmt.location);
                        return [];
                    case ArgumentType.BC:
                        // '[bc] = a' -> 'ld [bc], a'
                        if(auto load = buildArgument(program, src))
                        {
                            if(load.type == ArgumentType.A)
                            {
                                return [0x02];
                            }
                        }
                        error("Invalid initializer in '[bc] = ...'", stmt.location);
                        return [];
                    case ArgumentType.DE:
                        // '[de] = a' -> 'ld [de], a'
                        if(auto load = buildArgument(program, src))
                        {
                            if(load.type == ArgumentType.A)
                            {
                                return [0x12];
                            }
                        }
                        error("Invalid initializer in '[de] = ...'", stmt.location);
                        return [];
                    case ArgumentType.HL:
                        bool swap = false;
                        if(auto load = buildArgument(program, src))
                        {
                            // '[hl] = <>v' -> 'hl = v; hl = <>hl'
                            if(load.type == ArgumentType.Swap)
                            {
                                load = load.base;
                                swap = true;
                            }
                            // '[hl] = [hl]'
                            if(load.type == dest.type && load.base.type == dest.base.type)
                            {
                                return swap ? [0x36] : [];
                            }
                            // '[hl] = a' -> 'ld [hl], a'
                            if(load.type == ArgumentType.A)
                            {
                                return swap ? [0x36, 0x77] : [0x77];
                            }
                        }
                        error("Invalid initializer in '[hl] = " ~ (swap ? "<> " : "") ~ "...'", stmt.location);
                        return [];
                    default:
                        error("Invalid assignment to indirected expression.", stmt.location);
                        return [];
                }
            case ArgumentType.AF:
                // 'af = pop' -> 'pop af'
                return [];
            case ArgumentType.BC:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        case ArgumentType.Immediate:
                            // 'bc = n' -> 'ld bc, n'
                            error("TODO: assignment of 'bc' to immediate", stmt.location);
                            return [];
                        case ArgumentType.Pop:
                            // 'bc = pop' -> 'pop bc'
                            return [0xC1];
                        default:
                    }
                }
                error("Invalid assignment to 'bc'.", stmt.location);
                return [];
            case ArgumentType.DE:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        case ArgumentType.Immediate:
                            // 'de = n' -> 'ld de, n'
                            error("TODO: assignment of 'de' to immediate", stmt.location);
                            return [];
                        case ArgumentType.Pop:
                            // 'de = pop' -> 'pop de'
                            return [0xD1];
                        default:
                    }
                }
                error("Invalid assignment to 'de'.", stmt.location);
                return [];
            case ArgumentType.HL:
                if(auto load = buildArgument(program, src))
                {
                    // TODO: 'hl = sp' -> 'ld hl, sp + k'
                    switch(load.type)
                    {
                        case ArgumentType.Immediate:
                            // 'hl = n' -> 'ld hl, n'
                            error("TODO: assignment of 'hl' to immediate", stmt.location);
                            return [];
                        case ArgumentType.Pop:
                            // 'hl = pop' -> 'pop hl'
                            return [0xE1];
                        default:
                    }
                }
                error("Invalid assignment to 'hl'.", stmt.location);
                return [];
            case ArgumentType.SP:
                if(auto load = buildArgument(program, src))
                {
                    switch(load.type)
                    {
                        case ArgumentType.Immediate:
                            // 'sp = n' -> 'ld sp, n'
                            error("TODO: assignment of 'sp' to immediate", stmt.location);
                            return [];
                        case ArgumentType.HL:
                            // 'sp = hl' -> 'ld sp, hl'
                            return [0xF9];
                        default:
                    }
                }
                error("Invalid assignment to 'hl'.", stmt.location);
                return [];
            case ArgumentType.IndirectionInc:
                // '[hl++] = a' -> 'ld [hl+], a'
                return [];
            case ArgumentType.IndirectionDec:
                // '[hl--] = a' -> 'ld [hl-], a'
                return [];
            case ArgumentType.PositiveIndex:
                // '[FF00:c]' -> 'ldh [c], a'
                return [];
            case ArgumentType.NegativeIndex:
                error("Negative indexing is not supported.", stmt.location);
                return [];
            case ArgumentType.BitIndex:
                // 'r@i = 0' -> 'res r, i'
                // 'r@i = 1' -> 'set r, i'
                return [];
            case ArgumentType.Interrupt:
                // 'interrupt = 0' -> 'di'
                // 'interrupt = 1' -> 'ei'
                return [];
            case ArgumentType.Immediate:
                error("assignment '=' to immediate constant is invalid.", stmt.location);
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
                // 'carry = 1' -> 'scf'
                // 'carry = carry ^ 1' -> 'ccf'
                // 'carry = 0' -> 'scf; ccf'
                error("assignment '=' to 'carry' flag is invalid.", stmt.location);
                return [];
            case ArgumentType.Pop:
                error("'pop' operator on left hand of assignment '=' is invalid.", stmt.location);
                return [];
            case ArgumentType.None:
                return [];
        }
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
                                    error("indirected operand on left-hand side of '@' is not supported (only '[hl] @ ...' is valid)", stmt.right.location);
                                    return [];
                                }
                                break;
                            case ArgumentType.A: r = 0x7; break;
                            default:
                                return [];
                        }
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