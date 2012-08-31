module wiz.cpu.gameboy.platform;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.gameboy.lib;

class GameboyPlatform : Platform
{
    BuiltinTable builtins()
    {
        return .builtins();
    }

    ubyte[] generateCommand(compile.Program program, ast.Command stmt)
    {
        return .generateCommand(program, stmt);
    }

    ubyte[] generateJump(compile.Program program, ast.Jump stmt)
    {
        return .generateJump(program, stmt);
    }

    ubyte[] generateComparison(compile.Program program, ast.Comparison stmt)
    {
        return .generateComparison(program, stmt);
    }

    ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt)
    {
        return .generateAssignment(program, stmt);
    }
}


Platform.BuiltinTable builtins()
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

bool resolveJumpCondition(compile.Program program, ast.Jump stmt, string context, ref ubyte index)
{
    Argument flag;
    auto cond = stmt.condition;
    auto attr = cond.attr;
    bool negated = cond.negated;
    assert(cond !is null);

    if(attr is null)
    {
        final switch(cond.branch)
        {
            case parse.Branch.Equal:
                flag = new Argument(ArgumentType.Zero);
                break;
            case parse.Branch.NotEqual:
                flag = new Argument(ArgumentType.Zero);
                negated = !negated;
                break;
            case parse.Branch.Less:
                flag = new Argument(ArgumentType.Carry);
                negated = !negated;
                break;
            case parse.Branch.GreaterEqual:
                flag = new Argument(ArgumentType.Carry);
                break;
            case parse.Branch.Greater:
            case parse.Branch.LessEqual:
                error(
                    "comparision '" ~ parse.getBranchName(cond.branch)
                    ~ "' unsupported in 'when' clause of " ~ context, cond.location
                );
                return false;
        }
    }
    else
    {
        auto def = compile.resolveAttribute(program, attr);
        if(auto builtin = cast(Builtin) def)
        {
            switch(builtin.type)
            {
                case ArgumentType.Carry:
                case ArgumentType.Zero:
                    flag = new Argument(builtin.type);
                    break;
                default:
            }
        }
    }
    if(flag)
    {
        index = flag.getFlagIndex(negated);
        return true;
    }
    else
    {
        error(
            "unrecognized condition '" ~ attr.fullName()
            ~ "' used in 'when' clause of " ~ context, cond.location
        );
        return false;
    }
}

ubyte[] ensureUnconditional(ast.Jump stmt, string context, ubyte[] code)
{
    if(stmt.condition)
    {
        error("'when' clause is not allowed for " ~ context, stmt.condition.location);
        return [];
    }
    else
    {
        return code;
    }
}

ubyte[] generateJump(compile.Program program, ast.Jump stmt)
{
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
                    if(stmt.far)
                    {
                        compile.foldWord(program, argument.immediate, address, true);
                        if(stmt.condition)
                        {
                            ubyte index;
                            if(resolveJumpCondition(program, stmt, "'goto!'", index))
                            {
                                return [(0xC2 + index * 0x08) & 0xFF, address & 0xFF, (address >> 8) & 0xFF];
                            }
                        }
                        else
                        {
                            return [0xC3, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    else
                    {
                        enum description = "relative jump";
                        auto bank = program.checkBank(description, stmt.location);
                        uint pc = bank.checkAddress(description, stmt.location);
                        uint result;
                        compile.foldRelativeByte(program, stmt.destination,
                            "relative jump distance",
                            "rewrite the branch, shorten the gaps in your code, or add a '!' far indicator.",
                            pc, result, true
                        );
                        if(stmt.condition)
                        {
                            ubyte index;
                            if(resolveJumpCondition(program, stmt, "'goto'", index))
                            {
                                return [(0x20 + index * 0x08) & 0xFF, address & 0xFF, (address >> 8) & 0xFF];
                            }
                        }
                        else
                        {
                            return [0x18, address & 0xFF];
                        }
                    }
                    return [];
                case ArgumentType.HL:
                    if(stmt.condition is null)
                    {
                        return [0xE9];
                    }
                    else
                    {
                        error("'goto hl' does not support 'when' clause", stmt.condition.location);
                    }
                    return [];
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
                    compile.foldWord(program, argument.immediate, address, true);
                    if(stmt.condition)
                    {
                        ubyte index;
                        if(resolveJumpCondition(program, stmt, "'call'", index))
                        {
                            return [(0xC4 + index * 0x08) & 0xFF, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    else
                    {
                        return [0xCD, address & 0xFF, (address >> 8) & 0xFF];
                    }
                    return [];
                default:
                    error("unsupported argument to 'call'", stmt.condition.location);
                    return [];
            }
        case parse.Keyword.Return:
            if(stmt.condition)
            {
                ubyte index;
                if(resolveJumpCondition(program, stmt, "'return'", index))
                {
                    return [(0xC0 + index * 0x08) & 0xFF];
                }
            }
            else
            {
                return [0xC9];
            }
            return [];
        case parse.Keyword.Break:
            // TODO
            return [];
        case parse.Keyword.Continue:
            // TODO
            return [];
        case parse.Keyword.While:
            // TODO
            return [];
        case parse.Keyword.Until:
            // TODO
            return [];
        case parse.Keyword.Resume: return ensureUnconditional(stmt, "'resume'", [0xD9]);
        case parse.Keyword.Abort: return ensureUnconditional(stmt,  "'abort'",[0x40]);
        case parse.Keyword.Sleep: return ensureUnconditional(stmt,  "'sleep'",[0x76]);
        case parse.Keyword.Suspend: return ensureUnconditional(stmt,  "'suspend'", [0x10, 0x00]);
        case parse.Keyword.Nop: return ensureUnconditional(stmt, "'nop'", [0x00]);
        default:
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
                        compile.foldWord(program, right.immediate, value, true);
                        return [0xFE, value & 0xFF];
                    case ArgumentType.B:
                    case ArgumentType.C:
                    case ArgumentType.D:
                    case ArgumentType.E:
                    case ArgumentType.H:
                    case ArgumentType.L:
                        return [(0xB8 + left.getRegisterIndex()) & 0xFF];
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
                if(!compile.foldBitIndex(program, left.immediate, index, true))
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
                return [0xCB, (0x40 + index * 0x08 + left.getRegisterIndex()) & 0xFF];
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

ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt)
{
    if(stmt.src is null)
    {
        return generatePostfixAssignment(program, stmt);
    }
    else
    {
        auto dest = buildArgument(program, stmt.dest);
        if(stmt.intermediary)
        {
            auto intermediary = buildArgument(program, stmt.intermediary);
            return // 'x = y via z' -> 'z = y; x = z'
                generateCalculatedAssignment(program, stmt, intermediary, stmt.src)
                ~ generateCalculatedAssignment(program, stmt, dest, stmt.intermediary);
        }
        else
        {
            return generateCalculatedAssignment(program, stmt, dest, stmt.src);
        }
    }
}

ubyte[] generatePostfixAssignment(compile.Program program, ast.Assignment stmt)
{
    auto dest = buildArgument(program, stmt.dest);
    auto operatorIndex = [
        parse.Postfix.Inc: 0,
        parse.Postfix.Dec: 1
    ][stmt.postfix];
    auto operatorName = [
        parse.Postfix.Inc: "'++'",
        parse.Postfix.Dec: "'--'"
    ][stmt.postfix];

    switch(dest.type)
    {
        case ArgumentType.A:
        case ArgumentType.B:
        case ArgumentType.C:
        case ArgumentType.D:
        case ArgumentType.E:
        case ArgumentType.H:
        case ArgumentType.L:
            return [(0x04 + operatorIndex + dest.getRegisterIndex() * 0x10) & 0xFF];
        case ArgumentType.Indirection:
            if(dest.base.type == ArgumentType.HL)
            {
                return [(0x04 + operatorIndex + dest.getRegisterIndex() * 0x10) & 0xFF];
            }
            else
            {
                error(stmt.dest.toString() ~ " cannot be operand of " ~ operatorName, stmt.dest.location);
                return [];
            }
        case ArgumentType.BC:
        case ArgumentType.DE:
        case ArgumentType.HL:
        case ArgumentType.SP:
            return [(0x03 + (operatorIndex * 0x08) + dest.getPairIndex() * 0x10) & 0xFF];
        default:
            error(stmt.dest.toString() ~ " cannot be operand of " ~ operatorName, stmt.dest.location);
            return [];
    }
}

ubyte[] generateCalculatedAssignment(compile.Program program, ast.Assignment stmt, Argument dest, ast.Expression src)
{
    if(dest is null)
    {
        return [];
    }

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
        ubyte[] code = getLoad(program, stmt, dest, loadsrc);
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
                if(i == 0 && patchStackPointerLoadOffset(program, type, node, dest, operand, loadsrc, code))
                {
                    continue;
                }
                code ~= getModify(program, type, node, dest, operand, loadsrc);
            }
        }
        return code;
    }
    else
    {
        return getLoad(program, stmt, dest, src);
    }
}

bool patchStackPointerLoadOffset(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand, ast.Expression loadsrc, ubyte[] code)
{
    if(dest.type != ArgumentType.HL)
    {
        return false;
    }
    switch(type)
    {
        case parse.Infix.Add: break;
        case parse.Infix.Sub: break;
        default: return false;
    }
    switch(operand.type)
    {
        case ArgumentType.Immediate:
            auto load = buildArgument(program, loadsrc);
            if(load is null || load.type != ArgumentType.SP)
            {
                return false;
            }

            uint value;
            compile.foldSignedByte(program, operand.immediate, type == parse.Infix.Sub, value, true);
            // Monkey patch 'hl = sp + 00'
            code[code.length - 1] = value & 0xFF;
            return true;
        default:
            return false;
    }
}

ubyte[] operatorError(parse.Infix type, Argument dest, compile.Location location)
{
    error("infix operator " ~ parse.getInfixName(type) ~ " cannot be used in assignment '=' to " ~ dest.toString() ~ ".", location);
    return [];
}

ubyte[] operandError(parse.Infix type, Argument dest, Argument operand, compile.Location location)
{
    error(operand.toString() ~ " cannot be operand of " ~ parse.getInfixName(type) ~ " in assignment '=' to " ~ dest.toString() ~ ".", location);
    return [];
}

ubyte[] getModify(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand, ast.Expression loadsrc)
{
    switch(dest.type)
    {
        case ArgumentType.A:
            switch(type)
            {
                case parse.Infix.Add, parse.Infix.AddC,
                    parse.Infix.Sub, parse.Infix.SubC,
                    parse.Infix.And, parse.Infix.Xor, parse.Infix.Or:
                    return getAccumulatorArithmetic(program, type, node, dest, operand);
                case parse.Infix.ShiftL:
                case parse.Infix.ShiftR:
                case parse.Infix.ArithShiftL:
                case parse.Infix.ArithShiftR:
                    return getRegisterShift(program, type, node, dest, operand);
                case parse.Infix.RotateL:
                case parse.Infix.RotateR:
                case parse.Infix.RotateLC:
                case parse.Infix.RotateRC:
                    return getAccumulatorShift(program, type, node, dest, operand);
                default:
                    return operatorError(type, dest, node.location);
            }
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
                    return getRegisterShift(program, type, node, dest, operand);
                default:
                   return operatorError(type, dest, node.location);
            }
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
                            return getRegisterShift(program, type, node, dest, operand);
                        default:
                            return operatorError(type, dest, node.location);
                    }
                default:
                    return operatorError(type, dest, node.location);
            }
        case ArgumentType.HL:
            switch(type)
            {
                case parse.Infix.Add:
                    if(operand.type == ArgumentType.Immediate)
                    {
                        return operandError(type, dest, operand, node.location);
                    }
                    else
                    {
                        return getHighLowArithmetic(program, type, node, dest, operand);
                    }
                case parse.Infix.Sub:
                    return operatorError(type, dest, node.location);
                default:
                    return operatorError(type, dest, node.location);
            }
        case ArgumentType.SP:
            switch(type)
            {
                case parse.Infix.Add:
                case parse.Infix.Sub:
                    return getStackPointerArithmetic(program, type, node, dest, operand);
                default:
                    return operatorError(type, dest, node.location);
            }
        case ArgumentType.Carry:
            switch(type)
            {
                case parse.Infix.Xor:
                    assert(0); // TODO
                default:
                    return operatorError(type, dest, node.location);
            }
            return [];
        default:
            return operatorError(type, dest, node.location);
    }
}

ubyte[] getAccumulatorArithmetic(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    auto operatorIndex = cast(ubyte) [
        parse.Infix.Add: 0,
        parse.Infix.AddC: 1,
        parse.Infix.Sub: 2,
        parse.Infix.SubC: 3,
        parse.Infix.And: 4,
        parse.Infix.Xor: 5,
        parse.Infix.Or: 6,
    ][type];
    switch(operand.type)
    {
        case ArgumentType.Immediate:
            uint value;
            compile.foldByte(program, operand.immediate, value, true);
            return [(0xC6 + operatorIndex * 0x08) & 0xFF, value & 0xFF];
        case ArgumentType.A:
        case ArgumentType.B:
        case ArgumentType.C:
        case ArgumentType.D:
        case ArgumentType.E:
        case ArgumentType.H:
        case ArgumentType.L:
            return [(0x80 + operatorIndex * 0x08 + operand.getRegisterIndex()) & 0xFF];
        case ArgumentType.Indirection:
            switch(operand.base.type)
            {
                // 'r = [hl]' -> 'ld r, [hl]'
                case ArgumentType.HL:
                    return [(0x80 + operatorIndex * 0x08 + operand.getRegisterIndex()) & 0xFF];
                    break;
                default:
                    return operandError(type, dest, operand, node.location);
            }
            break;
        default:
            return operandError(type, dest, operand, node.location);
    }
}


ubyte[] getHighLowArithmetic(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    switch(operand.type)
    {
        case ArgumentType.BC:
        case ArgumentType.DE:
        case ArgumentType.HL:
        case ArgumentType.SP:
            return [(0x09 + operand.getPairIndex() * 0x10) & 0xFF];
            break;
        default:
            return operandError(type, dest, operand, node.location);
    }
}

ubyte[] getStackPointerArithmetic(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    switch(operand.type)
    {
        case ArgumentType.Immediate:
            uint value;
            compile.foldSignedByte(program, operand.immediate, type == parse.Infix.Sub, value, true);
            return [0xE8, value & 0xFF];
        default:
            return operandError(type, dest, operand, node.location);
    }
}

ubyte[] getAccumulatorShift(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    auto operatorIndex = cast(ubyte) [
        parse.Infix.RotateLC: 0,
        parse.Infix.RotateRC: 1,
        parse.Infix.RotateL: 2,
        parse.Infix.RotateR: 3,
    ][type];
    switch(operand.type)
    {
        case ArgumentType.Immediate:
            uint value;
            if(!compile.foldBitIndex(program, operand.immediate, value, true))
            {
                return [];
            }
            ubyte[] code;
            while(value--)
            {
                code ~= [(0x07 + operatorIndex * 0x08) & 0xFF];
            }
            return code;
        default:
            return operandError(type, dest, operand, node.location);
    }
}

ubyte[] getRegisterShift(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    auto operatorIndex = cast(ubyte) [
        parse.Infix.RotateLC: 0,
        parse.Infix.RotateRC: 1,
        parse.Infix.RotateL: 2,
        parse.Infix.RotateR: 3,
        parse.Infix.ArithShiftL: 4,
        parse.Infix.ArithShiftR: 5,
        parse.Infix.ShiftL: 4, // Logical shl == arith shl
        parse.Infix.ShiftL: 7,
    ][type];
    switch(operand.type)
    {
        case ArgumentType.Immediate:
            uint value;
            if(!compile.foldBitIndex(program, operand.immediate, value, true))
            {
                return [];
            }
            ubyte[] code;
            while(value--)
            {
                code ~= [0xCB, (operatorIndex * 0x08 + dest.getRegisterIndex()) & 0xFF];
            }
            return code;
        default:
            return operandError(type, dest, operand, node.location);
    }
}

ubyte[] getRegisterLoadImmediate(compile.Program program, ast.Assignment stmt, Argument dest, Argument load)
{
    uint value;
    compile.foldByte(program, load.immediate, value, true);
    return [(0x06 + dest.getRegisterIndex() * 0x08) & 0xFF, value & 0xFF];
}

ubyte[] getRegisterLoadRegister(compile.Program program, ast.Assignment stmt, Argument dest, Argument load)
{
    // ld r, r2, where r != r2 (we treat load self as empty code)
    if(dest.getRegisterIndex() != load.getRegisterIndex())
    {
        return [(0x40 + dest.getRegisterIndex() * 0x08 + load.getRegisterIndex()) & 0xFF];
    }
    return [];
}

ubyte[] getPairLoadImmediate(compile.Program program, ast.Assignment stmt, Argument dest, Argument load)
{
    uint value;
    compile.foldWord(program, load.immediate, value, true);
    return [(0x01 + dest.getPairIndex() * 0x10) & 0xFF, value & 0xFF, (value >> 8) & 0xFF];
}

ubyte[] getPairLoadPop(compile.Program program, ast.Assignment stmt, Argument dest)
{
    return [(0xC1 + dest.getPairIndex() * 0x10) & 0xFF];
}

ubyte[] getHighLowLoadPair(compile.Program program, ast.Assignment stmt, Argument load)
{
    return [0x21, 0x00, 0x00, (0x09 + load.getPairIndex() * 0x10) & 0xFF];
}

ubyte[] getAccumulatorLoadIndirectImmediate(compile.Program program, ast.Assignment stmt, Argument load)
{
    uint value;
    compile.foldWord(program, load.base.immediate, value, true);
    // 'a = [0xFFnn]' -> 'ldh a, [nn]'
    if((value & 0xFF00) == 0xFF00)
    {
        return [0xF0, value & 0xFF];
    }
    // 'a = [nnnn]' -> 'ld a, [nnnn]'
    else
    {
        return [0xFA, value & 0xFF, (value >> 8) & 0xFF];
    }
}

ubyte[] getIndirectImmediateLoadAccumulator(compile.Program program, ast.Assignment stmt, Argument dest)
{
    uint value;
    compile.foldWord(program, dest.base.immediate, value, true);
    // '[0xFFnn] = a' -> 'ldh [nn], a'
    if((value & 0xFF00) == 0xFF00)
    {
        return [0xF0, value & 0xFF];
    }
    // '[nnnn] = a' -> 'ld [nnnn], a'
    else
    {
        return [0xFA, value & 0xFF, (value >> 8) & 0xFF];
    }
}

ubyte[] getAccumulatorLoadIndirectPair(compile.Program program, ast.Assignment stmt, Argument load)
{
    return [(0x0A + load.getPairIndex() * 0x10) & 0xFF];
}

ubyte[] getIndirectPairLoadAccumulator(compile.Program program, ast.Assignment stmt, Argument dest)
{
    return [(0x02 + dest.getPairIndex() * 0x10) & 0xFF];
}

ubyte[] getAccumulatorLoadIndirectIncrement(compile.Program program, ast.Assignment stmt, Argument load)
{
    return [cast(ubyte) [
        ArgumentType.IndirectionInc: 0x2A,
        ArgumentType.IndirectionDec: 0x3A,
    ][load.type]];
}

ubyte[] getIndirectIncrementLoadAccumulator(compile.Program program, ast.Assignment stmt, Argument dest)
{
    return [cast(ubyte) [
        ArgumentType.IndirectionInc: 0x22,
        ArgumentType.IndirectionDec: 0x32,
    ][dest.type]];
}

ubyte[] invalidAssignmentDestError(Argument dest, compile.Location location)
{
    error("assignment '=' to " ~ dest.toString() ~ " is invalid.", location);
    return [];
}

ubyte[] invalidAssignmentError(Argument dest, Argument load, compile.Location location)
{
    error("invalid assignment '=' of " ~ dest.toString() ~ " to " ~ load.toString() ~ ".", location);
    return [];
}

ubyte[] getLoad(compile.Program program, ast.Assignment stmt, Argument dest, ast.Expression loadsrc)
{
    if(auto load = buildArgument(program, loadsrc))
    {
        return getPrefixLoad(program, stmt, dest, load);
    }
    else
    {
        return [];
    }
}

ubyte[] getPrefixLoad(compile.Program program, ast.Assignment stmt, Argument dest, Argument load)
{
    switch(load.type)
    {
        // 'r = <>r' -> 'swap r'
        case ArgumentType.Swap:
            switch(dest.type)
            {
                case ArgumentType.A, ArgumentType.B, ArgumentType.C, ArgumentType.D,
                    ArgumentType.E, ArgumentType.H, ArgumentType.L:
                        return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0xCB, (0x30 + dest.getRegisterIndex()) & 0xFF];
                case ArgumentType.Indirection:
                    if(dest.base.type == ArgumentType.HL)
                    {
                        return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0xCB, (0x30 + dest.getRegisterIndex()) & 0xFF];
                    }
                default:
                    return [];
            }
        // 'a = ~a' -> 'cpl a'
        case ArgumentType.Not:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x2F];
            }
            else
            {
                return invalidAssignmentError(dest, load, stmt.location);
            }
        // 'a = -a' -> 'cpl a, inc a'
        // 'carry = ~carry' -> 'ccf'
        case ArgumentType.Negated:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x2F, 0x3C];
            }
            if(dest.type == ArgumentType.Not)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x3F];
            }
            else
            {
                return invalidAssignmentError(dest, load, stmt.location);
            }
        default:
            return getBaseLoad(program, stmt, dest, load);
    }
}

ubyte[] getBaseLoad(compile.Program program, ast.Assignment stmt, Argument dest, Argument load)
{
    final switch(dest.type)
    {
        case ArgumentType.A:
            switch(load.type)
            {
                // 'a = n' -> 'ld a, n'
                case ArgumentType.Immediate:
                    return getRegisterLoadImmediate(program, stmt, dest, load);
                // 'a = a' -> (nothing)
                // 'a = r' -> 'ld a, r'
                case ArgumentType.A:
                case ArgumentType.B:
                case ArgumentType.D:
                case ArgumentType.E:
                case ArgumentType.H:
                case ArgumentType.L:
                    return getRegisterLoadRegister(program, stmt, dest, load);
                case ArgumentType.Indirection:
                    switch(load.base.type)
                    {
                        case ArgumentType.Immediate:
                            return getAccumulatorLoadIndirectImmediate(program, stmt, load);
                        // 'a = [bc]' -> 'ld a, [bc]'
                        // 'a = [de]' -> 'ld a, [de]'
                        case ArgumentType.BC, ArgumentType.DE:
                            return getAccumulatorLoadIndirectPair(program, stmt, load);
                        // 'a = [hl]' -> 'ld a, [hl]'
                        case ArgumentType.HL: 
                            return getRegisterLoadRegister(program, stmt, dest, load);
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.IndirectionInc:
                case ArgumentType.IndirectionDec:
                    switch(load.base.type)
                    {
                        // 'a = [hl++]' -> 'ldi a, [hl]',
                        // 'a = [hl--]' -> 'ldd a, [hl]',
                        case ArgumentType.HL:
                            return getAccumulatorLoadIndirectIncrement(program, stmt, load);
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.PositiveIndex:
                    uint value;
                    compile.foldWord(program, load.immediate, value, true);

                    if(value != 0xFF00 || load.base.type != ArgumentType.C)
                    {
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                    else
                    {
                        return [0xF2];
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.B:
        case ArgumentType.C:
        case ArgumentType.D:
        case ArgumentType.E:
        case ArgumentType.H:
        case ArgumentType.L:
            switch(load.type)
            {
                // 'r = n' -> 'ld r, n'
                case ArgumentType.Immediate:
                    return getRegisterLoadImmediate(program, stmt, dest, load);
                // 'r = r' -> (nothing)
                // 'r = r2' -> 'ld r, r2'
                case ArgumentType.A: 
                case ArgumentType.B:
                case ArgumentType.C:
                case ArgumentType.D:
                case ArgumentType.E:
                case ArgumentType.H:
                case ArgumentType.L:
                    return getRegisterLoadRegister(program, stmt, dest, load);
                case ArgumentType.Indirection:
                    switch(load.base.type)
                    {
                        // 'r = [hl]' -> 'ld r, [hl]'
                        case ArgumentType.HL:
                            return getRegisterLoadRegister(program, stmt, dest, load);
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Indirection:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    if(load.type == ArgumentType.A)
                    {
                        return getIndirectImmediateLoadAccumulator(program, stmt, dest);
                    }
                    else
                    {
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                // '[bc] = a' -> 'ld [bc], a'
                // '[de] = a' -> 'ld [de], a'
                case ArgumentType.BC, ArgumentType.DE:
                    if(load.type == ArgumentType.A)
                    {
                        return getIndirectPairLoadAccumulator(program, stmt, load);
                    }
                    else
                    {
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.HL:
                    switch(load.type)
                    {
                        // '[hl] = n' -> 'ld [hl], n'
                        case ArgumentType.Immediate:
                            return getRegisterLoadImmediate(program, stmt, dest, load);
                        // '[hl] = r' -> 'ld [hl], r'
                        case ArgumentType.A: 
                        case ArgumentType.B:
                        case ArgumentType.C:
                        case ArgumentType.D:
                        case ArgumentType.E:
                        case ArgumentType.H:
                        case ArgumentType.L:
                            return getRegisterLoadRegister(program, stmt, dest, load);
                        case ArgumentType.Indirection:
                            switch(load.base.type)
                            {
                                // '[hl] = [hl]' -> (nothing)
                                case ArgumentType.HL:
                                    return [];
                                default:
                                    return invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.AF:
            switch(load.type)
            {
                // 'af = pop' -> 'pop af'
                case ArgumentType.Pop:
                    return getPairLoadPop(program, stmt, dest);
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.BC:
        case ArgumentType.DE:
        case ArgumentType.HL:
            switch(load.type)
            {
                // 'rr = n' -> 'ld rr, n'
                case ArgumentType.Immediate:
                    return getPairLoadImmediate(program, stmt, dest, load);
                // 'rr = pop' -> 'pop rr'
                case ArgumentType.Pop:
                    return getPairLoadPop(program, stmt, dest);
                // 'hl = sp' -> 'ld hl, sp+0x0000'
                case ArgumentType.SP:
                    if(dest.type == ArgumentType.HL)
                    {
                        // 'hl = sp' -> 'ldhl sp,0'
                        return [0xF8, 0x00];
                    }
                    else
                    {
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                // 'rr = rr' -> (none)
                // 'hl = rr' -> 'ld hl, 0x0000; add hl, rr'
                case ArgumentType.BC:
                case ArgumentType.DE:
                case ArgumentType.HL:
                    if(dest.type != load.type)
                    {
                        if(dest.type == ArgumentType.HL)
                        {
                            return getHighLowLoadPair(program, stmt, load);
                        }
                        else 
                        {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    else
                    {
                        return [];
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.SP:
            switch(load.type)
            {
                // 'sp = n' -> 'ld sp, n'
                case ArgumentType.Immediate:
                    return getPairLoadImmediate(program, stmt, dest, load);
                // sp = sp -> (none)
                case ArgumentType.SP:
                    return [];
                // 'sp = hl' -> 'ld sp, hl'
                case ArgumentType.HL:
                    return [0xF9];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.IndirectionInc, ArgumentType.IndirectionDec:
            switch(load.type)
            {
                case ArgumentType.A:
                    // '[hl++] = a' -> 'ld [hl+], a'
                    return [0x22];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.PositiveIndex:
            uint value;
            compile.foldWord(program, dest.immediate, value, true);

            if(value != 0xFF00 || dest.base.type != ArgumentType.C)
            {
                error("assignment '=' to indexed memory location other than '[0xFF00:c]' is invalid.", stmt.location);
                return [];
            }
            switch(load.type)
            {
                // '[FF00:c]' -> 'ldh [c], a'
                case ArgumentType.A:
                    return [0xE2];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.BitIndex:
            uint index;
            if(!compile.foldBitIndex(program, dest.immediate, index, true))
            {
                return [];
            }
            switch(dest.base.type)
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
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                    break;
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
            switch(load.type)
            {
                // 'r@i = 0' -> 'res r, i'
                // 'r@i = 1' -> 'set r, i'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, value, true))
                    {
                        return [];
                    }
                    if(value == 0)
                    {
                        return [0xCB, (0x80 + index * 0x08 + dest.base.getRegisterIndex()) & 0xFF];
                    }
                    else
                    {
                        return [0xCB, (0xC0 + index * 0x08 + dest.base.getRegisterIndex()) & 0xFF];
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Interrupt:
            switch(load.type)
            {
                // 'interrupt = 0' -> 'di'
                // 'interrupt = 1' -> 'ei'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, value, true))
                    {
                        return [];
                    }
                    if(value == 0)
                    {
                        return [0xF3];
                    }
                    else
                    {
                        return [0xFB];
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Pop:
        case ArgumentType.Immediate:
        case ArgumentType.Not:
        case ArgumentType.Negated:
        case ArgumentType.Swap:
        case ArgumentType.F:
        case ArgumentType.Zero:
        case ArgumentType.NegativeIndex:
            return invalidAssignmentDestError(dest, stmt.location);
        case ArgumentType.Carry:
            switch(load.type)
            {
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, value, true))
                    {
                        return [];
                    }
                    // 'carry = 0' -> 'scf; ccf'    
                    if(value == 0)
                    {
                        return [0x37, 0x3F];
                    }
                    // 'carry = 1' -> 'scf'
                    else
                    {
                        return [0x37];
                    }
                // 'carry = carry' -> (none)
                case ArgumentType.Carry:
                    return [];
                default:
                    return invalidAssignmentDestError(dest, stmt.location);
            }
            break;
        case ArgumentType.None:
            assert(0);
    }
    //return [];
}