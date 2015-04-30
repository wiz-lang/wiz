module wiz.cpu.mos6502.platform;

import wiz.lib;
import wiz.cpu.lib;
import wiz.cpu.mos6502.lib;

static import std.stdio;

class Mos6502Platform : Platform
{
    override BuiltinTable builtins()
    {
        return .builtins();
    }

    override ubyte[] generatePush(compile.Program program, ast.Push stmt)
    {
        return .generatePush(program, stmt);
    }

    override ubyte[] generateJump(compile.Program program, ast.Jump stmt)
    {
        return .generateJump(program, stmt);
    }

    override ubyte[] generateComparison(compile.Program program, ast.Comparison stmt)
    {
        return .generateComparison(program, stmt);
    }

    override ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt)
    {
        return .generateAssignment(program, stmt);
    }

    override void patch(ref ubyte[] buffer)
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

ubyte[] generatePush(compile.Program program, ast.Push stmt)
{
    Argument argument;
    ubyte[] code;
    if(stmt.intermediary)
    {
        argument = buildArgument(program, stmt.intermediary);
        // 'push x via y' -> 'y = x; push y'
        code ~= generateCalculatedAssignment(program, stmt, argument, stmt.src);
    }
    else
    {
        argument = buildArgument(program, stmt.src);
    }
    switch(argument.type)
    {
        case ArgumentType.A: return code ~ cast(ubyte[])[0x48];
        case ArgumentType.P: return code ~ cast(ubyte[])[0x08];
        default:
            error("cannot push operand " ~ argument.toString() ~ " in 'push' statement", stmt.src.location);
            return [];
    }
}

bool resolveJumpCondition(compile.Program program, bool negated, ast.Jump stmt, ref ubyte index)
{
    Argument flag;
    auto cond = stmt.condition;
    auto attr = cast(ast.Attribute) cond.expr;
    assert(cond !is null);

    if(cond.expr is null && attr is null)
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
                    ~ " unsupported in branch condition", cond.location
                );
                return false;
        }
    }
    else
    {
        if(attr is null)
        {
            error("conditional expression wasn't substituted in branch condition", cond.location);
            return false;
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
            ~ "' used in branch", cond.location
        );
        return false;
    }
}

ubyte[] ensureUnconditional(ast.Jump stmt, string context, ubyte[] code)
{
    if(stmt.condition)
    {
        error("branch condition is not allowed for " ~ context, stmt.condition.location);
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
                        size_t address;
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
                            size_t pc = bank.checkAddress(description, stmt.location);
                            size_t offset;
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
                            size_t address;
                            compile.foldWord(program, argument.immediate, program.finalized, address);
                            return [0x4C, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    return [];
                case ArgumentType.Indirection:
                    switch(argument.base.type)
                    {
                        case ArgumentType.Immediate:
                            size_t address;
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
                    size_t address;
                    compile.foldWord(program, argument.immediate, program.finalized, address);
                    return ensureUnconditional(stmt,
                        "'call " ~ argument.toString(false) ~ "'",
                        [0x20, address & 0xFF, (address >> 8) & 0xFF]
                    );
                default:
                    error("unsupported argument to 'call'", stmt.destination.location);
                    return [];
            }
        case parse.Keyword.Return: return ensureUnconditional(stmt, "'return'", [0x60]);            
        case parse.Keyword.Resume: return ensureUnconditional(stmt, "'resume'", [0x40]);
        case parse.Keyword.Nop: return ensureUnconditional(stmt, "'nop'", [0xEA]);
        default:
            error("instruction not supported", stmt.destination.location);
            return [];
    }
}

ubyte[] comparisonMissingSecondaryError(Argument left, compile.Location location)
{
    error("'to' clause is required after " ~ left.toString() ~ " in 'compare' statement", location);
    return [];
}

ubyte[] comparisonDisallowedSecondaryError(Argument left, compile.Location location)
{
    error("'to' clause is not allowed after " ~ left.toString() ~ " in 'compare' statement", location);
    return [];
}

ubyte[] comparisonBadSecondaryError(Argument left, Argument right, compile.Location location)
{
    error("cannot compare " ~ left.toString() ~ " to " ~ right.toString() ~ " in 'compare' statement", location);
    return [];
}

ubyte[] comparisonBadPrimaryError(Argument left, compile.Location location)
{
    error(left.toString() ~ " cannot be the primary term of a 'compare' statement.", location);
    return [];
}

ubyte[] generateComparison(compile.Program program, ast.Comparison stmt)
{
    auto left = buildComparisonArgument(program, stmt.left);
    auto right = stmt.right ? buildArgument(program, stmt.right) : null;
    switch(left.type)
    {
        case ArgumentType.A:
            if(right)
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        size_t value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xC9, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                size_t address;
                                bool known;
                                compile.foldWord(program, right.base.immediate, program.finalized, address, known);
                                return known && address <= 0xFF
                                    ? [0xC5, address & 0xFF]
                                    : [0xCD, address & 0xFF, (address >> 8) & 0xFF];
                            case ArgumentType.Index:
                                if(right.base.base.type == ArgumentType.Immediate
                                && right.base.secondary.type == ArgumentType.X)
                                {
                                    size_t address;
                                    compile.foldByte(program, right.base.base.immediate, program.finalized, address);
                                    return [0xC1, address & 0xFF];
                                }
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                            default:
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                        }
                    case ArgumentType.Index:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:                                        
                                size_t address;
                                bool known;
                                compile.foldWord(program, right.base.immediate, program.finalized, address, known);
                                if(right.secondary.type == ArgumentType.X)
                                {
                                    return known && address <= 0xFF
                                        ? [0xD5, address & 0xFF]
                                        : [0xDD, address & 0xFF, (address >> 8) & 0xFF];
                                }
                                if(right.secondary.type == ArgumentType.Y)
                                {
                                    return [0xD9, address & 0xFF, (address >> 8) & 0xFF];
                                }
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                            case ArgumentType.Indirection:
                                if(right.base.base.type == ArgumentType.Immediate
                                && right.secondary.type == ArgumentType.Y)
                                {
                                    size_t address;
                                    compile.foldByte(program, right.base.base.immediate, program.finalized, address);
                                    return [0xD1, address & 0xFF];
                                }
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                            default:
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                        }
                    default:
                        return comparisonBadSecondaryError(left, right, stmt.right.location);
                }
            }
            return comparisonMissingSecondaryError(left, stmt.left.location);
        case ArgumentType.X:
            if(right)
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        size_t value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xE0, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                size_t address;
                                bool known;
                                compile.foldWord(program, right.base.immediate, program.finalized, address, known);
                                return known && address <= 0xFF
                                    ? [0xE4, address & 0xFF]
                                    : [0xEC, address & 0xFF, (address >> 8) & 0xFF];
                            default:
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                        }
                    default:
                        return comparisonBadSecondaryError(left, right, stmt.right.location);
                }
            }
            return comparisonMissingSecondaryError(left, stmt.left.location);
        case ArgumentType.Y:
            if(right)
            {
                // 'compare a to expr' -> 'cmp a, expr'
                switch(right.type)
                {
                    case ArgumentType.Immediate:
                        size_t value;
                        compile.foldByte(program, right.immediate, program.finalized, value);
                        return [0xC0, value & 0xFF];
                    case ArgumentType.Indirection:
                        switch(right.base.type)
                        {
                            case ArgumentType.Immediate:
                                size_t address;
                                bool known;
                                compile.foldWord(program, right.base.immediate, program.finalized, address, known);
                                return known && address <= 0xFF
                                    ? [0xC4, address & 0xFF]
                                    : [0xCC, address & 0xFF, (address >> 8) & 0xFF];
                            default:
                                return comparisonBadSecondaryError(left, right, stmt.right.location);
                        }
                    default:
                        return comparisonBadSecondaryError(left, right, stmt.right.location);
                }
            }
            return comparisonMissingSecondaryError(left, stmt.left.location);
        case ArgumentType.BitAnd:
            // 'compare a & [mem]' -> 'bit mem'
            if(right is null)
            {
                if(left.base is null)
                {
                    return comparisonBadPrimaryError(left, stmt.left.location);
                }
                switch(left.base.type)
                {
                    case ArgumentType.Indirection:
                        if(left.base.base.type != ArgumentType.Immediate)
                        {
                            return comparisonBadPrimaryError(left, stmt.left.location);
                        }
                        size_t address;
                        bool known;
                        compile.foldWord(program, left.base.base.immediate, program.finalized, address, known);
                        return known && address <= 0xFF
                            ? [0x24, address & 0xFF]
                            : [0x2C, address & 0xFF, (address >> 8) & 0xFF];
                    default:
                        return comparisonBadPrimaryError(left, stmt.left.location);
                }
            }
            return comparisonDisallowedSecondaryError(left, stmt.right.location);
        default:
            return comparisonBadPrimaryError(left, stmt.left.location);
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

ubyte[] postfixOperandError(parse.Postfix type, Argument operand, compile.Location location)
{
    error(operand.toString() ~ " cannot be operand of " ~ parse.getPostfixName(type) ~ " operator.", location);
    return [];
}

ubyte[] generatePostfixAssignment(compile.Program program, ast.Assignment stmt)
{
    auto dest = buildArgument(program, stmt.dest);
    if(!dest) return [];
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
                    size_t address;
                    bool known;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                    final switch(stmt.postfix)
                    {
                        case parse.Postfix.Inc:
                            return known && address <= 0xFF
                                ? [0xE6, address & 0xFF]
                                : [0xEE, address & 0xFF, (address >> 8) & 0xFF];
                        case parse.Postfix.Dec:
                            return known && address <= 0xFF
                                ? [0xC6, address & 0xFF]
                                : [0xCE, address & 0xFF, (address >> 8) & 0xFF];
                    }
                default:
                    return postfixOperandError(stmt.postfix, dest, stmt.dest.location);
            }
        case ArgumentType.Index:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:                                        
                    size_t address;
                    bool known;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                    if(dest.secondary.type == ArgumentType.X)
                    {
                        final switch(stmt.postfix)
                        {
                            case parse.Postfix.Inc:
                                return known && address <= 0xFF
                                    ? [0xF6, address & 0xFF]
                                    : [0xFE, address & 0xFF, (address >> 8) & 0xFF];
                            case parse.Postfix.Dec:
                                return known && address <= 0xFF
                                    ? [0xD6, address & 0xFF]
                                    : [0xDE, address & 0xFF, (address >> 8) & 0xFF];
                        }
                    }
                    return postfixOperandError(stmt.postfix, dest, stmt.dest.location);
                default:
                    return postfixOperandError(stmt.postfix, dest, stmt.dest.location);
            }
        default:
            return postfixOperandError(stmt.postfix, dest, stmt.dest.location);
    }
}

ubyte[] generateCalculatedAssignment(compile.Program program, ast.Statement stmt, Argument dest, ast.Expression src)
{
    if(dest is null)
    {
        return [];
    }

    if(auto infix = cast(ast.Infix) src)
    {
        size_t result;
        ast.Expression lastConstant;
        bool known;
        if(!compile.tryFoldExpression(program, infix, false, program.finalized, result, lastConstant, known))
        {
            return [];
        }
        ast.Expression loadsrc = null;
        if(lastConstant is null)
        {
            loadsrc = infix.operands[0];
        }
        else
        {
            loadsrc = new ast.Number(parse.Token.Integer, result, lastConstant.location);
        }
        ubyte[] code = getLoad(program, stmt, dest, loadsrc);
        bool found = lastConstant is null || lastConstant is infix.operands[0];
        foreach(i, type; infix.types)
        {
            auto node = infix.operands[i + 1];
            if(node is lastConstant)
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
    return getLoad(program, stmt, dest, src);
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
        case ArgumentType.A:
            switch(operand.type)
            {
                // 'a = imm' -> 'lda #imm'
                case ArgumentType.Immediate:
                    switch(type)
                    {
                        case parse.Infix.ShiftL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= [0x0A];
                            }
                            return code;
                        case parse.Infix.ShiftR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= [0x4A];
                            }
                            return code;
                        case parse.Infix.RotateL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= [0x2A];
                            }
                            return code;
                        case parse.Infix.RotateR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= [0x6A];
                            }
                            return code;
                        default:
                    }
                    size_t value;
                    compile.foldByte(program, operand.immediate, program.finalized, value);
                    switch(type)
                    {
                        case parse.Infix.Or: return [0x09, value & 0xFF];
                        case parse.Infix.And: return [0x29, value & 0xFF];
                        case parse.Infix.Xor: return [0x49, value & 0xFF];
                        case parse.Infix.Add: return [0x18, 0x69, value & 0xFF];
                        case parse.Infix.Sub: return [0x38, 0xE9, value & 0xFF];
                        case parse.Infix.AddC: return [0x69, value & 0xFF];
                        case parse.Infix.SubC: return [0xE9, value & 0xFF];
                        default:
                            return operatorError(type, dest, node.location);
                    }
                case ArgumentType.Indirection:
                    switch(operand.base.type)
                    {
                        // 'a = [addr]' -> 'lda addr'
                        case ArgumentType.Immediate:
                            size_t address;
                            bool known;
                            compile.foldWord(program, operand.base.immediate, program.finalized, address, known);
                            switch(type)
                            {
                                case parse.Infix.Or:
                                    return known && address <= 0xFF
                                        ? [0x05, address & 0xFF]
                                        : [0x0D, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.And:
                                    return known && address <= 0xFF
                                        ? [0x25, address & 0xFF]
                                        : [0x2D, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.Xor:
                                    return known && address <= 0xFF
                                        ? [0x45, address & 0xFF]
                                        : [0x4D, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.Add:
                                    return known && address <= 0xFF
                                        ? [0x18, 0x65, address & 0xFF]
                                        : [0x18, 0x6D, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.Sub:
                                    return known && address <= 0xFF
                                        ? [0x38, 0xE5, address & 0xFF]
                                        : [0x38, 0xED, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.AddC:
                                    return known && address <= 0xFF
                                        ? [0x65, address & 0xFF]
                                        : [0x6D, address & 0xFF, (address >> 8) & 0xFF];
                                case parse.Infix.SubC:
                                    return known && address <= 0xFF
                                        ? [0xE5, address & 0xFF]
                                        : [0xED, address & 0xFF, (address >> 8) & 0xFF];
                                default:
                                    return operatorError(type, dest, node.location);
                            }
                        case ArgumentType.Index:
                            auto index = operand.base;
                            // 'a = [[addr:x]]' -> 'lda [addr, x]'
                            if(index.base.type == ArgumentType.Immediate
                            && index.secondary.type == ArgumentType.X)
                            {
                                size_t address;
                                compile.foldByte(program, index.base.immediate, program.finalized, address);
                                switch(type)
                                {
                                    case parse.Infix.Or: return [0x01, address & 0xFF];
                                    case parse.Infix.And: return [0x21, address & 0xFF];
                                    case parse.Infix.Xor: return [0x41, address & 0xFF];
                                    case parse.Infix.Add: return [0x18, 0x61, address & 0xFF];
                                    case parse.Infix.Sub: return [0x38, 0xE1, address & 0xFF];
                                    case parse.Infix.AddC: return [0x61, address & 0xFF];
                                    case parse.Infix.SubC: return [0xE1, address & 0xFF];
                                    default:
                                        return operatorError(type, dest, node.location);
                                }
                            }
                            return operandError(type, dest, operand, node.location);
                        default:
                            return operandError(type, dest, operand, node.location);
                    }
                case ArgumentType.Index:
                    switch(operand.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            size_t address;
                            bool known;
                            compile.foldWord(program, operand.base.immediate, program.finalized, address, known);
                            // 'a = [addr:x]' -> 'lda addr, x'
                            if(operand.secondary.type == ArgumentType.X)
                            {
                                switch(type)
                                {
                                    case parse.Infix.Or:
                                        return known && address <= 0xFF
                                            ? [0x15, address & 0xFF]
                                            : [0x1D, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.And:
                                        return known && address <= 0xFF
                                            ? [0x35, address & 0xFF]
                                            : [0x3D, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Xor:
                                        return known && address <= 0xFF
                                            ? [0x55, address & 0xFF]
                                            : [0x5D, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Add:
                                        return known && address <= 0xFF
                                            ? [0x18, 0x75, address & 0xFF]
                                            : [0x18, 0x7D, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Sub:
                                        return known && address <= 0xFF
                                            ? [0x38, 0xF5, address & 0xFF]
                                            : [0x38, 0xFD, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.AddC:
                                        return known && address <= 0xFF
                                            ? [0x75, address & 0xFF]
                                            : [0x7D, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.SubC:
                                        return known && address <= 0xFF
                                            ? [0xF5, address & 0xFF]
                                            : [0xFD, address & 0xFF, (address >> 8) & 0xFF];
                                    default:
                                        return operatorError(type, dest, node.location);
                                }
                            }
                            // 'a = [load:y]' -> 'lda addr, y'
                            if(operand.secondary.type == ArgumentType.Y)
                            {
                                switch(type)
                                {
                                    case parse.Infix.Or: return [0x19, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.And: return [0x39, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Xor: return [0x59, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Add: return [0x18, 0x79, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.Sub: return [0x38, 0xF9, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.AddC: return [0x79, address & 0xFF, (address >> 8) & 0xFF];
                                    case parse.Infix.SubC: return [0xF9, address & 0xFF, (address >> 8) & 0xFF];
                                    default:
                                        return operatorError(type, dest, node.location);
                                }
                            }
                            return operandError(type, dest, operand, node.location);
                        case ArgumentType.Indirection:
                            // 'a = [[addr]:y]' -> 'lda [addr], y'
                            if(operand.base.base.type == ArgumentType.Immediate
                            && operand.secondary.type == ArgumentType.Y)
                            {
                                size_t address;
                                compile.foldByte(program, operand.base.base.immediate, program.finalized, address);
                                switch(type)
                                {
                                    case parse.Infix.Or: return [0x11, address & 0xFF];
                                    case parse.Infix.And: return [0x31, address & 0xFF];
                                    case parse.Infix.Xor: return [0x51, address & 0xFF];
                                    case parse.Infix.Add: return [0x18, 0x71, address & 0xFF];
                                    case parse.Infix.Sub: return [0x38, 0xF1, address & 0xFF];
                                    case parse.Infix.AddC: return [0x71, address & 0xFF];
                                    case parse.Infix.SubC: return [0xF1, address & 0xFF];
                                    default:
                                        return operatorError(type, dest, node.location);
                                }
                            }
                            return operandError(type, dest, operand, node.location);
                        default:
                            return operandError(type, dest, operand, node.location);
                    }
                default:
                    return operandError(type, dest, operand, node.location);
            }
        case ArgumentType.Indirection:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    size_t address;
                    bool known;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                    switch(type)
                    {
                        case parse.Infix.ShiftL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x06, address & 0xFF]
                                    : [0x0E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.ShiftR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x46, address & 0xFF]
                                    : [0x4E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.RotateL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x26, address & 0xFF]
                                    : [0x2E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.RotateR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x66, address & 0xFF]
                                    : [0x6E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        default:
                            return operatorError(type, dest, node.location);
                    }
                default:
                    return operandError(type, dest, operand, node.location);
            }
            break;
        case ArgumentType.Index:
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    if(dest.secondary.type != ArgumentType.X)
                    {
                        return operatorError(type, dest, node.location);
                    }
                    size_t address;
                    bool known;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                    switch(type)
                    {
                        case parse.Infix.ShiftL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x16, address & 0xFF]
                                    : [0x1E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.ShiftR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x56, address & 0xFF]
                                    : [0x5E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.RotateL:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x36, address & 0xFF]
                                    : [0x3E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        case parse.Infix.RotateR:
                            size_t value;
                            if(!compile.foldBitIndex(program, operand.immediate, program.finalized, value))
                            {
                                return [];
                            }
                            ubyte[] code;
                            while(value--)
                            {
                                code ~= known && address <= 0xFF
                                    ? [0x76, address & 0xFF]
                                    : [0x7E, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return code;
                        default:
                            return operatorError(type, dest, node.location);
                    }
                    return operatorError(type, dest, node.location);
                default:
                    return operandError(type, dest, operand, node.location);
            }
            break;
        default:
            return operandError(type, dest, operand, node.location);
    }
}

ubyte[] invalidAssignmentDestError(Argument dest, compile.Location location)
{
    error("assignment '=' of " ~ (dest ? dest.toString() : "???") ~ " is invalid.", location);
    return [];
}

ubyte[] invalidAssignmentError(Argument dest, Argument load, compile.Location location)
{
    error("invalid assignment '=' of " ~ (dest ? dest.toString() : "???") ~ " to " ~ (load ? load.toString() : "???") ~ ".", location);
    return [];
}

ubyte[] getLoad(compile.Program program, ast.Statement stmt, Argument dest, ast.Expression loadsrc)
{
    if(auto load = buildArgument(program, loadsrc))
    {
        return getPrefixLoad(program, stmt, dest, load);
    }
    return [];
}

ubyte[] getPrefixLoad(compile.Program program, ast.Statement stmt, Argument dest, Argument load)
{
    if(!dest || !load) {
        return invalidAssignmentError(dest, load, stmt.location);
    }
    switch(load.type)
    {
        // 'a = ~a' -> 'cpl a'
        case ArgumentType.Not:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x49, 0xFF];
            }
            return invalidAssignmentError(dest, load, stmt.location);
        // 'a = -a' -> 'eor a, #$FF; clc; adc a, #1'
        case ArgumentType.Negated:
            if(dest.type == ArgumentType.A)
            {
                return getPrefixLoad(program, stmt, dest, load.base) ~ cast(ubyte[])[0x49, 0xFF, 0x18, 0x69, 0x01];
            }
            return invalidAssignmentError(dest, load, stmt.location);
        default:
            return getBaseLoad(program, stmt, dest, load);
    }
}

ubyte[] getBaseLoad(compile.Program program, ast.Statement stmt, Argument dest, Argument load)
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
                    size_t value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA9, value & 0xFF];
                case ArgumentType.Indirection:
                    if(!load.base) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        // 'a = [addr]' -> 'lda addr'
                        case ArgumentType.Immediate:
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            return known && address <= 0xFF
                                ? [0xA5, address & 0xFF]
                                : [0xAD, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Index:
                            auto index = load.base;
                            // 'a = [[addr:x]]' -> 'lda [addr, x]'
                            if(index.base
                            && index.secondary
                            && index.base.type == ArgumentType.Immediate
                            && index.secondary.type == ArgumentType.X)
                            {
                                size_t address;
                                compile.foldByte(program, index.base.immediate, program.finalized, address);
                                return [0xA1, address & 0xFF];
                            }
                            return invalidAssignmentError(dest, load, stmt.location);
                        // 'a = [x]' -> 'lda 0, x' 
                        case ArgumentType.X: return [0xB5, 0];
                        // 'a = [y]' -> 'lda 0, y' 
                        case ArgumentType.Y: return [0xB9, 0, 0];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    if(!load.base || !load.secondary) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            // 'a = [addr:x]' -> 'lda addr, x'
                            if(load.secondary.type == ArgumentType.X)
                            {
                                return known && address <= 0xFF
                                    ? [0xB5, address & 0xFF]
                                    : [0xBD, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            // 'a = [load:y]' -> 'lda addr, y'
                            if(load.secondary.type == ArgumentType.Y)
                            {
                                return [0xB9, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return invalidAssignmentError(dest, load, stmt.location);
                        case ArgumentType.Indirection:
                            // 'a = [[addr]:y]' -> 'lda [addr], y'
                            if(load.base.base
                            && load.base.base.type == ArgumentType.Immediate
                            && load.secondary.type == ArgumentType.Y)
                            {
                                size_t address;
                                compile.foldByte(program, load.base.base.immediate, program.finalized, address);
                                return [0xB1, address & 0xFF];
                            }
                            return invalidAssignmentError(dest, load, stmt.location);
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
                    size_t value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA2, value & 0xFF];
                case ArgumentType.Indirection:
                    if(!load.base) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        // 'x = [addr]' -> 'ldx addr'
                        case ArgumentType.Immediate:
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            return known && address <= 0xFF
                                ? [0xA6, address & 0xFF]
                                : [0xAE, address & 0xFF, (address >> 8) & 0xFF];
                        // 'x = [y]' -> 'ldx 0, y'
                        case ArgumentType.Y: return [0xB6, 0];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    if(!load.base || !load.secondary) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            // 'x = [addr:y]' -> 'lda addr, y'
                            if(load.secondary.type == ArgumentType.Y)
                            {
                                return known && address <= 0xFF
                                    ? [0xB6, address & 0xFF]
                                    : [0xBE, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return invalidAssignmentError(dest, load, stmt.location);
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
                    size_t value;
                    compile.foldByte(program, load.immediate, program.finalized, value);
                    return [0xA0, value & 0xFF];
                case ArgumentType.Indirection:
                    if(!load.base) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        // 'y = [addr]' -> 'ldy addr'
                        case ArgumentType.Immediate:
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            return known && address <= 0xFF
                                ? [0xA4, address & 0xFF]
                                : [0xAC, address & 0xFF, (address >> 8) & 0xFF];
                        // 'y = [x]' -> 'ldy 0, x'
                        case ArgumentType.X: return [0xB4, 0];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    if(!load.base) return invalidAssignmentError(dest, load, stmt.location);
                    switch(load.base.type)
                    {
                        case ArgumentType.Immediate:                                        
                            size_t address;
                            bool known;
                            compile.foldWord(program, load.base.immediate, program.finalized, address, known);
                            // 'y = [addr:x]' -> 'ldy addr, x'
                            if(load.secondary.type == ArgumentType.X)
                            {
                                return known && address <= 0xFF
                                    ? [0xB4, address & 0xFF]
                                    : [0xBC, address & 0xFF, (address >> 8) & 0xFF];
                            }
                            return invalidAssignmentError(dest, load, stmt.location);
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
            if(!dest.base) return invalidAssignmentError(dest, load, stmt.location);
            switch(dest.base.type)
            {
                // '[addr] = a' -> 'sta addr'
                // '[addr] = x' -> 'stx addr'
                // '[addr] = y' -> 'sty addr'
                case ArgumentType.Immediate:
                    size_t address;
                    bool known;
                    compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                    switch(load.type)
                    {
                        case ArgumentType.A:
                            return known && address <= 0xFF
                                ? [0x85, address & 0xFF]
                                : [0x8D, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.X:
                            return known && address <= 0xFF
                                ? [0x86, address & 0xFF]
                                : [0x8E, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Y:
                            return known && address <= 0xFF
                                ? [0x84, address & 0xFF]
                                : [0x8C, address & 0xFF, (address >> 8) & 0xFF];
                        case ArgumentType.Indirection:
                            if(load.base.type == ArgumentType.Immediate)
                            {
                                size_t match;
                                compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                compile.foldWord(program, load.base.immediate, program.finalized, match);
                                return (match == address) ? [] : invalidAssignmentError(dest, load, stmt.location);
                            }
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Index:
                    auto index = dest.base;
                    // '[[addr:x]] = a' -> 'sta [addr, x]'
                    if(index.base
                    && index.secondary
                    && index.base.type == ArgumentType.Immediate
                    && index.secondary.type == ArgumentType.X)
                    {
                        size_t address;
                        compile.foldByte(program, index.base.immediate, program.finalized, address);
                        if(load.type == ArgumentType.A)
                        {
                            return [0x81, address & 0xFF];
                        }
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                    return invalidAssignmentDestError(dest, stmt.location);
                case ArgumentType.X:
                    switch(load.type)
                    {
                        // '[x] = a' -> 'sta 0, x'
                        case ArgumentType.A: return [0x95, 0];
                        // '[x] = y' -> 'sty 0, x'
                        case ArgumentType.Y: return [0x94, 0];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                case ArgumentType.Y:
                    switch(load.type)
                    {
                        // '[y] = a' -> 'sta 0, x'
                        case ArgumentType.A: return [0x99, 0, 0];
                        // '[y] = x' -> 'stx 0, y'
                        case ArgumentType.X: return [0x96, 0];
                        default:
                            return invalidAssignmentError(dest, load, stmt.location);
                    }
                default:
                    return invalidAssignmentDestError(dest, stmt.location);
            }
        case ArgumentType.Index:
            if(!dest.base || !dest.secondary) return invalidAssignmentError(dest, load, stmt.location);
            switch(dest.base.type)
            {
                case ArgumentType.Immediate:
                    bool known;
                    size_t address;
                    // '[addr:x] = a' -> 'sta addr, x'
                    // '[addr:x] = y' -> 'sty addr, x'
                    if(dest.secondary.type == ArgumentType.X)
                    {
                        switch(load.type)
                        {
                            case ArgumentType.A:
                                compile.foldWord(program, dest.base.immediate, program.finalized, address, known);
                                return known && address <= 0xFF
                                    ? [0x95, address & 0xFF]
                                    : [0x9D, address & 0xFF, (address >> 8) & 0xFF];
                            case ArgumentType.Y:
                                compile.foldByte(program, dest.base.immediate, program.finalized, address);
                                return [0x94, address & 0xFF];
                            case ArgumentType.Index:
                                if(load.base.type == ArgumentType.Immediate && load.secondary.type == ArgumentType.X)
                                {
                                    size_t match;
                                    compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                    compile.foldWord(program, load.base.immediate, program.finalized, match);
                                    return (match == address) ? [] : invalidAssignmentError(dest, load, stmt.location);
                                }
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    // '[addr:y] = a' -> 'sta addr, y'
                    // '[addr:y] = x' -> 'stx addr, y'
                    if(dest.secondary.type == ArgumentType.Y)
                    {
                        switch(load.type)
                        {
                            case ArgumentType.A:
                                compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                return [0x99, address & 0xFF, (address >> 8) & 0xFF];
                            case ArgumentType.Y:
                                compile.foldByte(program, dest.base.immediate, program.finalized, address);
                                return [0x96, address & 0xFF];
                            case ArgumentType.Index:
                                if(load.base.type == ArgumentType.Immediate && load.secondary.type == ArgumentType.Y)
                                {
                                    size_t match;
                                    compile.foldWord(program, dest.base.immediate, program.finalized, address);
                                    compile.foldWord(program, load.base.immediate, program.finalized, match);
                                    return (match == address) ? [] : invalidAssignmentError(dest, load, stmt.location);
                                }
                            default:
                                return invalidAssignmentError(dest, load, stmt.location);
                        }
                    }
                    return invalidAssignmentDestError(dest, stmt.location);
                case ArgumentType.Indirection:
                    // '[[addr]:y] = a' -> 'sta [addr], y'
                    if(dest.base.base
                    && dest.base.base.type == ArgumentType.Immediate
                    && dest.secondary.type == ArgumentType.Y)
                    {
                        size_t address;
                        compile.foldByte(program, dest.base.base.immediate, program.finalized, address);
                        if(load.type == ArgumentType.A)
                        {
                            return [0x91, address & 0xFF];
                        }
                        return invalidAssignmentError(dest, load, stmt.location);
                    }
                    return invalidAssignmentDestError(dest, stmt.location);
                default:
                    return invalidAssignmentDestError(dest, stmt.location);
            }
        case ArgumentType.Decimal:
            switch(load.type)
            {
                // 'decimal = 0' -> 'cld'
                // 'decimal = 1' -> 'sed'
                case ArgumentType.Immediate:
                    size_t value;
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
                    size_t value;
                    if(!compile.foldBit(program, load.immediate, program.finalized, value))
                    {
                        return [];
                    }
                    if(value == 0)
                    {
                        return [0xB8];
                    }
                    error("invalid assignment '=' of " ~ dest.toString() ~ " to non-zero value.", stmt.location);
                    return [];
                default:
                    return invalidAssignmentError(dest, load, stmt.location);
            }
        case ArgumentType.Carry:
            switch(load.type)
            {
                // 'carry = 0' -> 'clc'
                // 'carry = 1' -> 'sec'
                case ArgumentType.Immediate:
                    size_t value;
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
                    size_t value;
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