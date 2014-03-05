module wiz.ast.comparison;

import wiz.lib;
import wiz.ast.lib;

class Comparison : Statement
{
    private Expression _left, _right;

    this(Expression left, Expression right, compile.Location location)
    {
        super(location);
        _left = left;
        _right = right;
    }

    this(Expression left, compile.Location location)
    {
        super(location);
        _left = left;
    }
        
    mixin compile.BranchAcceptor!(_left, _right);
    mixin helper.Accessor!(_left, _right);
}