module wiz.ast.inlinefor;

import wiz.lib;
import wiz.ast.lib;

class InlineFor : Statement
{
    private string _name;
    private Expression _startExpression;
    private Expression _endExpression;
    private Expression _stepExpression;
    private Block _block;

    private bool expanded;

    this(string name, Expression startExpression, Expression endExpression, Expression stepExpression, Block block, compile.Location location)
    {
        super(location);
        _name = name;
        _startExpression = startExpression;
        _endExpression = endExpression;
        _stepExpression = stepExpression;
        _block = block;
    }

    bool expand(compile.Program program)
    {
        if(expanded)
        {
            return false;
        }

        expanded = true;

        size_t start;
        size_t end;
        size_t step = 1;

        if(!compile.foldExpression(program, startExpression, true, start)
        || !compile.foldExpression(program, endExpression, true, end)
        || stepExpression && !compile.foldExpression(program, stepExpression, true, step))
        {
            return true;
        }

        if(step == 0)
        {
            compile.error("'inline for' with step size of 0 will never terminate.", stepExpression ? stepExpression.location : location);
            return true;
        }
        if(start >= Expression.Max)
        {
            compile.error("'inline for' has invalid start value.", startExpression.location);
            return true;
        }
        if(start >= Expression.Max)
        {
            compile.error("'inline for' has invalid end value.", endExpression.location);
            return true;
        }

        Statement[] code;
        for(size_t i = start; (step > 0 && i <= end || step < 0 && i >= end); i += step)
        {
            Statement[] statements;
            statements ~= new LetDecl(name, new Number(parse.Token.Integer, i, block.location), block.location);
            statements ~= _block.statements;
            code ~= new Block(statements, location);
        }
        _block = new Block(code, location);

        return true;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_name, _startExpression, _endExpression, _stepExpression, _block);
}