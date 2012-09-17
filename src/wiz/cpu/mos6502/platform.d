module wiz.cpu.mos6502.platform;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.mos6502.lib;

static import std.stdio;

class Mos6502Platform : Platform
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

    void patch(ref ubyte[] buffer)
    {
    }
}

Platform.BuiltinTable builtins()
{
    return [
        "a": new Builtin(ArgumentType.A),
        "x": new Builtin(ArgumentType.X),
        "y": new Builtin(ArgumentType.Y),
        "s": new Builtin(ArgumentType.S),
        "p": new Builtin(ArgumentType.P),
        "zero": new Builtin(ArgumentType.Zero),
        "negative": new Builtin(ArgumentType.Negative),
        "decimal": new Builtin(ArgumentType.Decimal),
        "overflow": new Builtin(ArgumentType.Overflow),
        "carry": new Builtin(ArgumentType.Carry),
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
                case ArgumentType.A: return [0x48];
                case ArgumentType.P: return [0x08];
                default: return [];
            }
        default:
            return [];
    }
}

bool resolveJumpCondition(compile.Program program, bool negated, ast.Jump stmt, ref ubyte index)
{
    Argument flag;
    auto cond = stmt.condition;
    auto attr = cond.attr;
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
                    "comparision " ~ parse.getBranchName(cond.branch)
                    ~ " unsupported in 'when' clause", cond.location
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
                case ArgumentType.Zero:
                case ArgumentType.Negative:
                case ArgumentType.Overflow:
                case ArgumentType.Carry:
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
            ~ "' used in 'when' clause", cond.location
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
                    if(stmt.far)
                    {
                        uint address;
                        compile.foldWord(program, argument.immediate, program.finalized, address);
                        if(stmt.condition)
                        {
                            ubyte index;
                            if(resolveJumpCondition(program, !stmt.condition.negated, stmt, index))
                            {
                                return [
                                    (0x10 + index * 0x20) & 0xFF, 3,
                                    0x4C, address & 0xFF, (address >> 8) & 0xFF
                                ];
                            }
                        }
                        else
                        {
                            return [0x4C, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    else
                    {
                        if(stmt.condition)
                        {
                            enum description = "relative jump";
                            auto bank = program.checkBank(description, stmt.location);
                            uint pc = bank.checkAddress(description, stmt.location);
                            uint offset;
                            compile.foldRelativeByte(program, stmt.destination,
                                "relative jump distance",
                                "rewrite the branch, shorten the gaps in your code, or add a '!' far indicator.",
                                pc + 2, program.finalized, offset
                            );
                            ubyte index;
                            if(resolveJumpCondition(program, stmt.condition.negated, stmt, index))
                            {
                                return [(0x10 + index * 0x20) & 0xFF, offset & 0xFF];
                            }
                        }
                        else
                        {
                            uint address;
                            compile.foldWord(program, argument.immediate, program.finalized, address);
                            return [0x4C, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    return [];
                case ArgumentType.Indirection:
                    switch(argument.base.type)
                    {
                        case ArgumentType.Immediate:
                            uint address;
                            compile.foldWord(program, argument.base.immediate, program.finalized, address);
                            return ensureUnconditional(stmt,
                                "'goto " ~ argument.toString(false) ~ "'",
                                [0x6C, address & 0xFF, (address >> 8) & 0xFF]
                            );
                        default:
                            error(argument.toString() ~ " is unsupported argument to 'goto'.", stmt.destination.location);
                            return [];
                    }
                    return [];
                default:
                    error(argument.toString() ~ " is unsupported argument to 'goto'.", stmt.destination.location);
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
                    compile.foldWord(program, argument.immediate, program.finalized, address);
                    return ensureUnconditional(stmt,
                        "'call " ~ argument.toString(false) ~ "'",
                        [0x20, address & 0xFF, (address >> 8) & 0xFF]
                    );
                default:
                    error("unsupported argument to 'call'", stmt.destination.location);
                    return [];
            }
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
        case parse.Keyword.Return: return ensureUnconditional(stmt, "'return'", [0x60]);            
        case parse.Keyword.Resume: return ensureUnconditional(stmt, "'resume'", [0x40]);
        case parse.Keyword.Nop: return ensureUnconditional(stmt, "'nop'", [0xEA]);
        default:
            error("instruction not supported", stmt.destination.location);
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
                error("'to' clause is required after 'compare a'", stmt.right.location);
                return [];
            }
            else
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        uint value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xC9, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                uint address;
                                compile.foldWord(program, right.base.immediate, program.finalized, address);
                                return address < 0xFF
                                    ? [0xC5, address & 0xFF]
                                    : [0xCD, address & 0xFF, (address >> 8) & 0xFF];
                            case ArgumentType.Indirection:
                                switch(right.base.base.type)
                                {
                                    case ArgumentType.Index:
                                        auto index = right.base.base;
                                        if(index.base.type == ArgumentType.Immediate
                                        && index.secondary.type == ArgumentType.X)
                                        {
                                            uint address;
                                            compile.foldByte(program, index.base.immediate, program.finalized, address);
                                            return [0xC1, address & 0xFF];
                                        }
                                        else
                                        {
                                            error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                            return [];
                                        }
                                    default:
                                        error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                        return [];
                                }
                            case ArgumentType.Index:
                                auto index = right.base;
                                switch(index.base.type)
                                {
                                    case ArgumentType.Immediate:                                        
                                        uint address;
                                        compile.foldWord(program, index.base.immediate, program.finalized, address);
                                        if(index.secondary.type == ArgumentType.X)
                                        {
                                            return address < 0xFF
                                                ? [0xD5, address & 0xFF]
                                                : [0xDD, address & 0xFF, (address >> 8) & 0xFF];
                                        }
                                        else if(index.secondary.type == ArgumentType.Y)
                                        {
                                            return [0xD9, address & 0xFF, (address >> 8) & 0xFF];
                                        }
                                        else
                                        {
                                            error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                            return [];
                                        }
                                    case ArgumentType.Indirection:
                                        if(index.secondary.type == ArgumentType.Y)
                                        {
                                            uint address;
                                            compile.foldByte(program, index.base.immediate, program.finalized, address);
                                            return [0xD1, address & 0xFF];
                                        }
                                        else
                                        {
                                            error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                            return [];
                                        }
                                    default:
                                        error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                        return [];
                                }
                            default:
                                error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                                return [];
                        }
                    default:
                        error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.right.location);
                        return [];
                }
            }
        case ArgumentType.X:
            if(right is null)
            {
                error("'to' clause is required after 'compare x'", stmt.right.location);
                return [];
            }
            else
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        uint value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xE0, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                uint address;
                                compile.foldWord(program, right.base.immediate, program.finalized, address);
                                return address < 0xFF
                                    ? [0xE4, address & 0xFF]
                                    : [0xEC, address & 0xFF, (address >> 8) & 0xFF];
                            default:
                                error("unsupported operand in 'to' clause of 'compare x to ...'", stmt.right.location);
                                return [];
                        }
                    default:
                        error("unsupported operand in 'to' clause of 'compare x to ...'", stmt.right.location);
                        return [];
                }
            }
        case ArgumentType.Y:
            if(right is null)
            {
                error("'to' clause is required after 'compare y'", stmt.right.location);
                return [];
            }
            else
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        uint value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xC0, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                uint address;
                                compile.foldWord(program, right.base.immediate, program.finalized, address);
                                return address < 0xFF
                                    ? [0xC4, address & 0xFF]
                                    : [0xCC, address & 0xFF, (address >> 8) & 0xFF];
                            default:
                                error("unsupported operand in 'to' clause of 'compare y to ...'", stmt.right.location);
                                return [];
                        }
                    default:
                        error("unsupported operand in 'to' clause of 'compare y to ...'", stmt.right.location);
                        return [];
                }
            }
        case ArgumentType.BitAnd:
            // 'compare a & [mem]' -> 'bit mem'
            if(right is null)
            {
                switch(left.base.type)
                {
                    case ArgumentType.Indirection:
                        if(left.base.base.type != ArgumentType.Immediate)
                        {
                            error("unsupported operand on right-hand side of '&'", stmt.right.location);
                            return [];
                        }
                        uint address;
                        compile.foldWord(program, left.base.base.immediate, program.finalized, address);
                        return address < 0xFF
                            ? [0x24, address & 0xFF]
                            : [0x2C, address & 0xFF, (address >> 8) & 0xFF];
                    default:
                        error("unsupported operand on right-hand side of '&'", stmt.right.location);
                        return [];
                }
            }
            else
            {
                error("'to' clause is unsupported for 'compare ... & ...'", stmt.right.location);
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
    auto operatorName = [
        parse.Postfix.Inc: "'++'",
        parse.Postfix.Dec: "'--'"
    ][stmt.postfix];
    switch(dest.type)
    {
        case ArgumentType.A:
            final switch(stmt.postfix)
            {
                case parse.Postfix.Inc: return [0x18, 0x69, 0x01];
                case parse.Postfix.Dec: return [0x38, 0xE9, 0x01];
            }
        case ArgumentType.X:
            final switch(stmt.postfix)
            {
                case parse.Postfix.Inc: return [0xE8];
                case parse.Postfix.Dec: return [0xCA];
            }
        case ArgumentType.Y:
            final switch(stmt.postfix)
            {
                case parse.Postfix.Inc: return [0xC8];
                case parse.Postfix.Dec: return [0x88];
            }
        case ArgumentType.Indirection:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    uint address;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address);
                    final switch(stmt.postfix)
                    {
                        case parse.Postfix.Inc:
                            return address < 0xFF
                                ? [0xE6, address & 0xFF]
                                : [0xEE, address & 0xFF, (address >> 8) & 0xFF];
                        case parse.Postfix.Dec:
                            return address < 0xFF
                                ? [0xC6, address & 0xFF]
                                : [0xCE, address & 0xFF, (address >> 8) & 0xFF];
                    }
                case ArgumentType.Index:
                    auto index = dest.base;
                    switch(index.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            uint address;
                            compile.foldWord(program, index.base.immediate, program.finalized, address);
                            if(index.secondary.type == ArgumentType.X)
                            {
                                final switch(stmt.postfix)
                                {
                                    case parse.Postfix.Inc:
                                        return address < 0xFF
                                            ? [0xF6, address & 0xFF]
                                            : [0xFE, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Postfix.Dec:
                                        return address < 0xFF
                                            ? [0xD6, address & 0xFF]
                                            : [0xDE, address & 0xFF, (address >> 8) & 0xFF];
                                }
                            }
                            else
                            {
                                error("unsupported operand in 'to' clause of 'compare a to ...'", stmt.dest.location);
                                return [];
                            }
                        default:
                            error(dest.toString() ~ " cannot be operand of " ~ operatorName, stmt.dest.location);
                            return [];
                    }
                default:
                    error(dest.toString() ~ " cannot be operand of " ~ operatorName, stmt.dest.location);
                    return [];
            }
        default:
            error(dest.toString() ~ " cannot be operand of " ~ operatorName, stmt.dest.location);
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
        if(!compile.tryFoldConstant(program, infix, false, program.finalized, result, constTail))
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
                code ~= getModify(program, type, node, dest, operand);
            }
        }
        return code;
    }
    else
    {
        return getLoad(program, stmt, dest, src);
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

ubyte[] getModify(compile.Program program, parse.Infix type, ast.Expression node, Argument dest, Argument operand)
{
    switch(dest.type)
    {
        default:
            return operatorError(type, dest, node.location);
    }
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
        // 'a = ~a' -> 'cpl a'
        case ArgumentType.Not:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x49, 0xFF];
            }
            else
            {
                return invalidAssignmentError(dest, load, stmt.location);
            }
        // 'a = -a' -> 'eor a, #$FF; clc; adc a, #1'
        case ArgumentType.Negated:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x49, 0xFF, 0x18, 0x69, 0x01];
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
                // 'a = a' -> (none)
                case ArgumentType.A: return [];
                // 'a = x' -> 'tax'
                case ArgumentType.X: return [0x8A];
                // 'a = x' -> 'tay'
                case ArgumentType.Y: return [0x98];
                // 'a = pop' -> 'pla'
                case ArgumentType.Pop: return [0x68];
                // 'a = imm' -> 'lda #imm'
                case ArgumentType.Immediate:
                    uint value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA9, value & 0xFF];
                case ArgumentType.Indirection:
                    switch(load.base.type)
                    {
                        // 'a = [addr]' -> 'lda addr'
                        case ArgumentType.Immediate:
                            uint address;
                            compile.foldWord(program, load.base.immediate, program.finalized, address);
                            return address < 0xFF
                                ? [0xA5, address & 0xFF]
                                : [0xCD, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Index:
                            auto index = load.base;
                            // 'a = [[addr:x]]' -> 'lda [addr, x]'
                            if(index.base.type == ArgumentType.Immediate
                            && index.secondary.type == ArgumentType.X)
                            {
                                uint address;
                                compile.foldByte(program, index.base.immediate, program.finalized, address);
                                return [0xA1, address & 0xFF];
                            }
                            else
                            {
                                return invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    switch(load.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            uint address;
                            compile.foldWord(program, load.base.immediate, program.finalized, address);
                            // 'a = [addr:x]' -> 'lda addr, x'
                            if(load.secondary.type == ArgumentType.X)
                            {
                                return address < 0xFF
                                    ? [0xB5, address & 0xFF]
                                    : [0xBD, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            // 'a = [load:y]' -> 'lda addr, y'
                            else if(load.secondary.type == ArgumentType.Y)
                            {
                                return [0xB9, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            else
                            {
                                return invalidAssignmentError(dest, load, stmt.location);
                            }
                        case ArgumentType.Indirection:
                            // 'a = [[addr]:y]' -> 'lda [addr], y'
                            if(load.base.base.type == ArgumentType.Immediate
                            && load.secondary.type == ArgumentType.Y)
                            {
                                uint address;
                                compile.foldByte(program, load.base.base.immediate, program.finalized, address);
                                return [0xB1, address & 0xFF];
                            }
                            else
                            {
                                return invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.X:
            switch(load.type)
            {
                // 'x = x' -> (none)
                case ArgumentType.X: return [];
                // 'x = a' -> 'tax'
                case ArgumentType.A: return [0xAA];
                // 'x = s' -> 'tsx'
                case ArgumentType.S: return [0xBA];
                // 'x = imm' -> 'ldx #imm'
                case ArgumentType.Immediate:
                    uint value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA2, value & 0xFF];
                case ArgumentType.Indirection:
                    switch(load.base.type)
                    {
                        // 'x = [addr]' -> 'ldx addr'
                        case ArgumentType.Immediate:
                            uint address;
                            compile.foldWord(program, load.base.immediate, program.finalized, address);
                            return address < 0xFF
                                ? [0xA6, address & 0xFF]
                                : [0xAE, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Index:
                            auto index = load.base;
                            switch(index.base.type)
                            {
                                case ArgumentType.Immediate:                                        
                                    uint address;
                                    compile.foldWord(program, index.base.immediate, program.finalized, address);
                                    // 'x = [addr:y]' -> 'lda addr, y'
                                    if(index.secondary.type == ArgumentType.Y)
                                    {
                                        return address < 0xFF
                                            ? [0xB6, address & 0xFF]
                                            : [0xBE, address & 0xFF, (address >> 8) & 0xFF];
                                    }
                                    else
                                    {
                                        return invalidAssignmentError(dest, load, stmt.location);
                                    }
                                default:
                                    return invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Y:
            switch(load.type)
            {
                // 'y = y' -> (none)
                case ArgumentType.Y: return [];
                // 'y = y' -> 'tay'
                case ArgumentType.A: return [0xA8];
                // 'y = imm' -> 'ldy #imm'
                case ArgumentType.Immediate:
                    uint value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA0, value & 0xFF];
                case ArgumentType.Indirection:
                    switch(load.base.type)
                    {
                        // 'y = [addr]' -> 'ldy addr'
                        case ArgumentType.Immediate:
                            uint address;
                            compile.foldWord(program, load.base.immediate, program.finalized, address);
                            return address < 0xFF
                                ? [0xA4, address & 0xFF]
                                : [0xAC, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Index:
                            auto index = load.base;
                            switch(index.base.type)
                            {
                                case ArgumentType.Immediate:                                        
                                    uint address;
                                    compile.foldWord(program, index.base.immediate, program.finalized, address);
                                    // 'y = [addr:x]' -> 'ldy addr, x'
                                    if(index.secondary.type == ArgumentType.X)
                                    {
                                        return address < 0xFF
                                            ? [0xB4, address & 0xFF]
                                            : [0xBC, address & 0xFF, (address >> 8) & 0xFF];
                                    }
                                    else
                                    {
                                        return invalidAssignmentError(dest, load, stmt.location);
                                    }
                                default:
                                    return invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.S:
            switch(load.type)
            {
                // 's = s' -> (none)
                case ArgumentType.S: return [];
                // 's = x' -> 'txs'
                case ArgumentType.X: return [0x9A];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Indirection:
            switch(dest.base.type)
            {
                // 'a = [addr]' -> 'lda addr'
                case ArgumentType.Immediate:
                    uint address;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address);
                    switch(load.type)
                    {
                        case ArgumentType.A:
                            return address < 0xFF
                                ? [0x85, address & 0xFF]
                                : [0x8D, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.X:
                            return address < 0xFF
                                ? [0x86, address & 0xFF]
                                : [0x8E, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Y:
                            return address < 0xFF
                                ? [0x84, address & 0xFF]
                                : [0x8C, address & 0xFF, (address >> 8) & 0xFF];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    auto index = dest.base;
                    // 'a = [[addr:x]]' -> 'lda [addr, x]'
                    if(index.base.type == ArgumentType.Immediate
                    && index.secondary.type == ArgumentType.X)
                    {
                        uint address;
                        compile.foldByte(program, index.base.immediate, program.finalized, address);
                        if(load.type == ArgumentType.A)
                        {
                            return [0x81, address & 0xFF];
                        }
                        else
                        {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    else
                    {
                        return invalidAssignmentDestError(dest, stmt.location);
                    }
                default:
                    return invalidAssignmentDestError(dest, stmt.location);
            }
        case ArgumentType.Index:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    uint address;
                    // 'a = [addr:x]' -> 'lda addr, x'
                    if(dest.secondary.type == ArgumentType.X)
                    {
                        switch(load.type)
                        {
                            case ArgumentType.A:
                                compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                return address < 0xFF
                                    ? [0x95, address & 0xFF]
                                    : [0x9D, address & 0xFF, (address >> 8) & 0xFF];
                            case ArgumentType.Y:
                                compile.foldByte(program, dest.base.immediate, program.finalized, address);
                                return [0x94, address & 0xFF];
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    // 'a = [addr:y]' -> 'lda addr, y'
                    else if(dest.secondary.type == ArgumentType.Y)
                    {
                        switch(load.type)
                        {
                            case ArgumentType.A:
                                compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                return [0x99, address & 0xFF];
                            case ArgumentType.Y:
                                compile.foldByte(program, dest.base.immediate, program.finalized, address);
                                return [0x96, address & 0xFF];
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    else
                    {
                        return invalidAssignmentDestError(dest, stmt.location);
                    }
                case ArgumentType.Indirection:
                    // 'a = [[addr]:y]' -> 'lda [addr], y'
                    if(dest.base.base.type == ArgumentType.Immediate
                    && dest.secondary.type == ArgumentType.Y)
                    {
                        uint address;
                        compile.foldByte(program, dest.base.base.immediate, program.finalized, address);
                        if(load.type == ArgumentType.A)
                        {
                            return [0x91, address & 0xFF];
                        }
                        else
                        {
                            return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    else
                    {
                        return invalidAssignmentDestError(dest, stmt.location);
                    }
                default:
                    return invalidAssignmentDestError(dest, stmt.location);
            }
        case ArgumentType.Decimal:
            switch(load.type)
            {
                // 'decimal = 0' -> 'cld'
                // 'decimal = 1' -> 'sed'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, program.finalized, value))
                    {
                        return [];
                    }
                    return value == 0 ? [0xD8] : [0xF8];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Overflow:
            switch(load.type)
            {
                // 'overflow = 0' -> 'clv'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, program.finalized, value))
                    {
                        return [];
                    }
                    if(value == 0)
                    {
                        return [0xB8];
                    }
                    else
                    {
                        error("invalid assignment '=' of " ~ dest.toString() ~ " to non-zero value.", stmt.location);
                        return [];
                    }
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Carry:
            switch(load.type)
            {
                // 'carry = 0' -> 'clc'
                // 'carry = 1' -> 'sec'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, program.finalized, value))
                    {
                        return [];
                    }
                    return value == 0 ? [0x18] : [0x38];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Interrupt:
            switch(load.type)
            {
                // 'interrupt = 0' -> 'cli'
                // 'interrupt = 1' -> 'sei'
                case ArgumentType.Immediate:
                    uint value;
                    if(!compile.foldBit(program, load.immediate, program.finalized, value))
                    {
                        return [];
                    }
                    return value == 0 ? [0x58] : [0x78];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.P:
            switch(load.type)
            {
                // 'a = pop' -> 'plp'
                case ArgumentType.Pop: return [0x28];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Zero:
        case ArgumentType.Negative:
        case ArgumentType.Immediate:
        case ArgumentType.BitAnd:
        case ArgumentType.Not:
        case ArgumentType.Negated:
        case ArgumentType.Pop:
            return invalidAssignmentDestError(dest, stmt.location);
        case ArgumentType.None:
            assert(0);
    }
}