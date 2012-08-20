module wiz.ast.infix;

import wiz.lib;
import wiz.ast.lib;

class Infix : Expression
{
    private parse.Infix _type;
    private Expression _left, _right;

    this(parse.Infix type, Expression left, Expression right, compile.Location location)
    {
        super(location);
        _type = type;
        _left = left;
        _right = right;
    }

    mixin compile.BranchAcceptor!(_left, _right);
    mixin helper.Accessor!(_type, _left, _right);
}