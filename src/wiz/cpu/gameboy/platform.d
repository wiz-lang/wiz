module wiz.cpu.gameboy.platform;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.gameboy.lib;

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
            enum description = "'goto'";
            auto bank = program.checkBank(description, stmt.location);
            uint pc = bank.checkAddress(description, stmt.location);
            if(address < pc)
            {
                if(pc - address < 128)
                {
                    // TODO YELL AT THEM
                }
                return [opcode, (~(pc - address) + 1) & 0xFF];
            }
            else
            {
                if(address - pc < 127)
                {
                    // TODO YELL AT THEM
                }
                return [opcode, (address - pc) & 0xFF];
            }
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
                        foldWord(program, argument.immediate, address, true);
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
                        foldWord(program, argument.immediate, address, true);
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
                        case ArgumentType.A:
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            return [(0x05 + getRegisterIndex(dest) * 0x08) & 0xFF];
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
                        case ArgumentType.BC:
                        case ArgumentType.DE:
                        case ArgumentType.HL:
                        case ArgumentType.SP:
                            return [(0x03 + getPairIndex(dest) * 0x10) & 0xFF];
                        default:
                            error("unsupported operand of '++'", stmt.dest.location);
                            return [];
                    }
                case parse.Postfix.Dec:
                    // 'dec dest'
                    switch(dest.type)
                    {
                        case ArgumentType.A:
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            return [(0x05 + getRegisterIndex(dest) * 0x08) & 0xFF];
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
                        case ArgumentType.BC:
                        case ArgumentType.DE:
                        case ArgumentType.HL:
                        case ArgumentType.SP:
                            return [(0x0B + getPairIndex(dest) * 0x10) & 0xFF];
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
        if(dest is null)
        {
            return [];
        }

        // TODO: fold constant left part of src expressions.
        if(auto infix = cast(ast.Infix) src)
        {
            uint result;
            ast.Expression constTail;
            if(!compile.tryFoldConstant(program, infix, result, constTail, false, true))
            {
                return [];
            }
            ast.Expression loadsrc = null;
            if(constTail is null)
            {
                loadsrc = infix.operands[0];
            }
            else
            {
                loadsrc = new ast.Number(parse.Token.Integer, result, constTail.location);
            }
            ubyte[] code = generateLoad(program, stmt, dest, loadsrc);
            bool found = constTail is null || constTail is infix.operands[0];
            foreach(i, type; infix.types)
            {                
                auto node = infix.operands[i + 1];
                if(node is constTail)
                {
                    found = true;
                }
                else if(found)
                {
                    auto operand = buildArgument(program, node);
                    if(operand is null)
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
                    // hl:
                    //      + (add)
                    // carry:
                    //      ^ (ccf)
                    switch(dest.type)
                    {
                        case ArgumentType.A:
                            switch(type)
                            {
                                case parse.Infix.Add, parse.Infix.AddC,
                                    parse.Infix.Sub, parse.Infix.SubC,
                                    parse.Infix.And, parse.Infix.Xor, parse.Infix.Or:
                                    ubyte operatorIndex = 0;
                                    switch(type)
                                    {
                                        case parse.Infix.Add: operatorIndex = 0; break;
                                        case parse.Infix.AddC: operatorIndex = 1; break;
                                        case parse.Infix.Sub: operatorIndex = 2; break;
                                        case parse.Infix.SubC: operatorIndex = 3; break;
                                        case parse.Infix.And: operatorIndex = 4; break;
                                        case parse.Infix.Xor: operatorIndex = 5; break;
                                        case parse.Infix.Or: operatorIndex = 6; break;
                                        default: assert(0);
                                    }
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            foldByte(program, operand.immediate, value, true);
                                            code ~= [(0xC6 + operatorIndex * 0x08) & 0xFF, value & 0xFF];
                                            break;
                                        case ArgumentType.A:
                                        case ArgumentType.B:
                                        case ArgumentType.C:
                                        case ArgumentType.D:
                                        case ArgumentType.E:
                                        case ArgumentType.H:
                                        case ArgumentType.L:
                                            code ~= [(0x80 + operatorIndex * 0x08 + getRegisterIndex(operand)) & 0xFF];
                                            break;
                                        case ArgumentType.Indirection:
                                            switch(operand.base.type)
                                            {
                                                // 'r = [hl]' -> 'ld r, [hl]'
                                                case ArgumentType.HL:
                                                    code ~= [(0x80 + operatorIndex * 0x08 + getRegisterIndex(operand)) & 0xFF];
                                                    break;
                                                default:
                                                    error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'a'.", node.location);
                                                    return [];
                                            }
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'a'.", node.location);
                                            return [];
                                    }
                                    break;
                                case parse.Infix.ShiftL:
                                case parse.Infix.ShiftR:
                                case parse.Infix.ArithShiftL:
                                case parse.Infix.ArithShiftR:
                                    ubyte operatorIndex = 0;
                                    switch(type)
                                    {
                                        case parse.Infix.ArithShiftL: operatorIndex = 0; break;
                                        case parse.Infix.ArithShiftR: operatorIndex = 1; break;
                                        case parse.Infix.ShiftL: operatorIndex = 0; break; // Logical shl == arith shl
                                        case parse.Infix.ShiftR: operatorIndex = 3; break;
                                        default: assert(0);
                                    }
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            if(!foldBitIndex(program, operand.immediate, value, true))
                                            {
                                                return [];
                                            }
                                            while(value--)
                                            {
                                                code ~= [0xCB, (0x27 + operatorIndex * 0x08) & 0xFF];
                                            }
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'a'.", node.location);
                                            return [];
                                    }
                                    break;
                                case parse.Infix.RotateL:
                                case parse.Infix.RotateR:
                                case parse.Infix.RotateLC:
                                case parse.Infix.RotateRC:
                                    ubyte operatorIndex = 0;
                                    switch(type)
                                    {
                                        case parse.Infix.RotateLC: operatorIndex = 0; break;
                                        case parse.Infix.RotateRC: operatorIndex = 1; break;
                                        case parse.Infix.RotateL: operatorIndex = 2; break;
                                        case parse.Infix.RotateR: operatorIndex = 3; break;
                                        default: assert(0);
                                    }
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            if(!foldBitIndex(program, operand.immediate, value, true))
                                            {
                                                return [];
                                            }
                                            while(value--)
                                            {
                                                code ~= [(0x07 + operatorIndex * 0x08) & 0xFF];
                                            }
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'a'.", node.location);
                                            return [];
                                    }
                                    break;
                                default:
                                    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to 'a'.", node.location);
                                    return [];
                            }
                            break;
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            switch(type)
                            {
                                case parse.Infix.RotateLC:
                                case parse.Infix.RotateRC:
                                case parse.Infix.RotateL:
                                case parse.Infix.RotateR:
                                case parse.Infix.ShiftL:
                                case parse.Infix.ShiftR:
                                case parse.Infix.ArithShiftL:
                                case parse.Infix.ArithShiftR:
                                    ubyte operatorIndex = 0;
                                    switch(type)
                                    {
                                        case parse.Infix.RotateLC: operatorIndex = 0; break;
                                        case parse.Infix.RotateRC: operatorIndex = 1; break;
                                        case parse.Infix.RotateL: operatorIndex = 2; break;
                                        case parse.Infix.RotateR: operatorIndex = 3; break;
                                        case parse.Infix.ArithShiftL: operatorIndex = 4; break;
                                        case parse.Infix.ArithShiftR: operatorIndex = 5; break;
                                        case parse.Infix.ShiftL: operatorIndex = 4; break; // Logical shl == arith shl
                                        case parse.Infix.ShiftR: operatorIndex = 7; break;
                                        default: assert(0);
                                    }
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            if(!foldBitIndex(program, operand.immediate, value, true))
                                            {
                                                return [];
                                            }
                                            while(value--)
                                            {
                                                code ~= [0xCB, (operatorIndex * 0x08 + getRegisterIndex(dest)) & 0xFF];
                                            }
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'a'.", node.location);
                                            return [];
                                    }
                                    break;
                                default:
                                    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to register.", node.location);
                                    return [];
                            }
                            break;
                        case ArgumentType.Indirection:
                            switch(dest.base.type)
                            {
                                // 'r = [hl]' -> 'ld r, [hl]'
                                case ArgumentType.HL:
                                    switch(type)
                                    {
                                        case parse.Infix.RotateLC:
                                        case parse.Infix.RotateRC:
                                        case parse.Infix.RotateL:
                                        case parse.Infix.RotateR:
                                        case parse.Infix.ShiftL:
                                        case parse.Infix.ShiftR:
                                        case parse.Infix.ArithShiftL:
                                        case parse.Infix.ArithShiftR:
                                            ubyte operatorIndex = 0;
                                            switch(type)
                                            {
                                                case parse.Infix.RotateLC: operatorIndex = 0; break;
                                                case parse.Infix.RotateRC: operatorIndex = 1; break;
                                                case parse.Infix.RotateL: operatorIndex = 2; break;
                                                case parse.Infix.RotateR: operatorIndex = 3; break;
                                                case parse.Infix.ArithShiftL: operatorIndex = 4; break;
                                                case parse.Infix.ArithShiftR: operatorIndex = 5; break;
                                                case parse.Infix.ShiftL: operatorIndex = 4; break; // Logical shl == arith shl
                                                case parse.Infix.ShiftR: operatorIndex = 7; break;
                                                default: assert(0);
                                            }
                                            switch(operand.type)
                                            {
                                                case ArgumentType.Immediate:
                                                    uint value;
                                                    if(!foldBitIndex(program, operand.immediate, value, true))
                                                    {
                                                        return [];
                                                    }
                                                    while(value--)
                                                    {
                                                        code ~= [0xCB, (operatorIndex * 0x08 + getRegisterIndex(dest)) & 0xFF];
                                                    }
                                                    break;
                                                default:
                                                    error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to '[hl]'.", node.location);
                                                    return [];
                                            }
                                        default:
                                            error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to '[hl]'.", node.location);
                                            return [];
                                    }
                                    break;
                                default:
                                    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to indirected term.", node.location);
                                    return [];
                            }
                            break;
                        case ArgumentType.HL:
                            switch(type)
                            {
                                case parse.Infix.Add:
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            auto load = buildArgument(program, loadsrc);
                                            if(i != 0 || load is null)
                                            {
                                                error("invalid addition of constant '+' in assignment '=' to 'hl' (only allowed form is directly after 'hl = sp + ...')", node.location);
                                            }
                                            else
                                            {
                                                uint value;
                                                compile.foldConstant(program, operand.immediate, value, true);
                                                if(value > 127)
                                                {
                                                    error(
                                                        "invalid offset of +" ~ std.conv.to!string(value)
                                                        ~ " used as addition '+'"
                                                        ~ " in assignment '=' to 'hl = sp + ...', should be in range -128..127",
                                                        node.location
                                                    );
                                                    return [];
                                                }
                                                // Monkey patch 'hl = sp + 00'
                                                code[code.length - 1] = value & 0xFF;
                                            }
                                            break;
                                        case ArgumentType.BC:
                                        case ArgumentType.DE:
                                        case ArgumentType.HL:
                                        case ArgumentType.SP:
                                            code ~= [(0x09 + getPairIndex(operand) * 0x10) & 0xFF];
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'hl'.", node.location);
                                            return [];
                                    }
                                    break;
                                case parse.Infix.Sub:
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            auto load = buildArgument(program, loadsrc);
                                            if(i != 0 || load is null)
                                            {
                                                error("invalid subtraction '-' of constant in assignment '=' to 'hl' (only allowed form is directly after 'hl = sp - ...')", node.location);
                                            }
                                            else
                                            {
                                                uint value;
                                                compile.foldConstant(program, operand.immediate, value, true);
                                                if(value > 128)
                                                {
                                                    error(
                                                        "invalid offset of -" ~ std.conv.to!string(value)
                                                        ~ " used as addition '-'"
                                                        ~ " in assignment '=' to 'hl = sp - ...', should be in range -128..127",
                                                        node.location
                                                    );
                                                    return [];
                                                }
                                                // Monkey patch 'hl = sp + 00'
                                                code[code.length - 1] = (~value + 1) & 0xFF;
                                            }
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'hl'.", node.location);
                                            return [];
                                    }
                                    break;
                                default:
                                    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to 'hl'.", node.location);
                                    return [];
                            }
                            break;
                        case ArgumentType.SP:
                            switch(type)
                            {
                                case parse.Infix.Add:
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            compile.foldConstant(program, operand.immediate, value, true);
                                            if(value > 127)
                                            {
                                                error(
                                                    "invalid offset of +" ~ std.conv.to!string(value)
                                                    ~ " used as addition '+'"
                                                    ~ " in assignment '=' to 'sp', should be in range -128..127",
                                                    node.location
                                                );
                                                return [];
                                            }
                                            code ~= [0xE8, value & 0xFF];
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'sp'.", node.location);
                                            return [];
                                    }
                                    break;
                                case parse.Infix.Sub:
                                    switch(operand.type)
                                    {
                                        case ArgumentType.Immediate:
                                            uint value;
                                            compile.foldConstant(program, operand.immediate, value, true);
                                            if(value > 128)
                                            {
                                                error(
                                                    "invalid offset of -" ~ std.conv.to!string(value)
                                                    ~ " used as subtraction '-'"
                                                    ~ " in assignment '=' to 'sp', should be in range -128..127",
                                                    node.location
                                                );
                                                return [];
                                            }
                                            code ~= [0xE8, (~value + 1) & 0xFF];
                                            break;
                                        default:
                                            error("invalid operand to " ~ parse.getInfixName(type) ~ " in assignment '=' to 'sp'.", node.location);
                                            return [];
                                    }
                                    break;
                                default:
                                    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to 'sp'.", node.location);
                                    return [];
                            }
                            break;
                        case ArgumentType.Carry:
                            break;
                        default:
                    }
                }
            }
            return code;
        }
        else
        {
            return generateLoad(program, stmt, dest, src);
        }
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
                            foldByte(program, load.immediate, value, true);
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
                                    foldWord(program, load.base.immediate, value, true);
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
                            foldWord(program, load.immediate, value, true);

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
                            foldByte(program, load.immediate, value, true);
                            code ~= [(0x06 + destIndex * 0x08) & 0xFF, value & 0xFF];
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
                                code ~= [(0x40 + destIndex * 0x08 + sourceIndex) & 0xFF];
                            }
                            break;
                        case ArgumentType.Indirection:
                            switch(load.base.type)
                            {
                                // 'r = [hl]' -> 'ld r, [hl]'
                                case ArgumentType.HL:
                                    ubyte sourceIndex = getRegisterIndex(load);
                                    code ~= [(0x40 + destIndex * 0x08 + sourceIndex) & 0xFF];
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
                                foldWord(program, dest.base.immediate, value, true);
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
                                    foldByte(program, load.immediate, value, true);
                                    code ~= [(0x06 + destIndex * 0x08) & 0xFF, value & 0xFF];
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
                                        code ~= [(0x40 + destIndex * 0x08 + sourceIndex) & 0xFF];
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
                            foldWord(program, load.immediate, value, true);
                            code ~= [(0x01 + destIndex * 0x10) & 0xFF, value & 0xFF, (value >> 8) & 0xFF];
                            break;
                        // 'rr = pop' -> 'pop rr'
                        case ArgumentType.Pop:
                            code ~= [(0xC1 + destIndex * 0x10) & 0xFF];
                            break;
                        case ArgumentType.BC:
                        case ArgumentType.DE:
                        case ArgumentType.HL:
                            if(dest.type != load.type)
                            {
                                if(dest.type == ArgumentType.HL)
                                {
                                    switch(load.type)
                                    {
                                        // 'hl = rr' -> 'ld hl, 0x0000; add hl, rr'
                                        case ArgumentType.BC:
                                        case ArgumentType.DE:
                                        case ArgumentType.SP:
                                            code ~= [0x21, 0x00, 0x00, (0x09 + getPairIndex(dest) * 0x10) & 0xFF];
                                            break;
                                        default: assert(0);
                                    }
                                }
                                else 
                                {
                                    return invalidAssignment("register pair");
                                }
                            }
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
                            foldWord(program, load.immediate, value, true);
                            code ~= [(0x01 + getPairIndex(dest) * 0x10) & 0xFF, value & 0xFF, (value >> 8) & 0xFF];
                            break;
                        // sp = sp -> (none)
                        case ArgumentType.SP: break;
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
                foldWord(program, dest.immediate, value, true);

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
                if(!foldBitIndex(program, dest.immediate, index, true))
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
                            if(!foldBit(program, load.immediate, value, true))
                            {
                                return [];
                            }
                            if(value == 0)
                            {
                                code ~= [0xCB, (0x80 + index * 0x08 + getRegisterIndex(dest)) & 0xFF];
                            }
                            else if(value == 1)
                            {
                                code ~= [0xCB, (0xC0 + index * 0x08 + getRegisterIndex(dest)) & 0xFF];
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
                            if(!foldBit(program, load.immediate, value, true))
                            {
                                return [];
                            }
                            if(value == 0)
                            {
                                code ~= [0xF3];
                            }
                            else if(value == 1)
                            {
                                code ~= [0xFB];
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
                            if(!foldBit(program, load.immediate, value, true))
                            {
                                return [];
                            }
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
                            foldWord(program, right.immediate, value, true);
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
                    if(!foldBitIndex(program, left.immediate, index, true))
                    {
                        return [];
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
                    return [0xCB, (0x40 + index * 0x08 + getRegisterIndex(left)) & 0xFF];
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