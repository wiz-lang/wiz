module wiz.ast.infix;

import wiz.lib;
import wiz.ast.lib;

class Infix : Expression
{
    private parse.Token _infixType;
    private Expression _left, _right;

    this(parse.Token infixType, Expression left, Expression right, compile.Location location)
    {
        super(location);
        _infixType = infixType;
        _left = left;
        _right = right;
    }

    mixin compile.BranchAcceptor!(_left, _right);
    mixin helper.Accessor!(_infixType, _left, _right);
}