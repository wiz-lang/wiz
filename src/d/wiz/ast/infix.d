module wiz.ast.infix;

import wiz.lib;
import wiz.ast.lib;

class Infix : Expression
{
    private parse.Infix[] _types;
    private Expression[] _operands;

    this(parse.Infix[] types, Expression[] operands, compile.Location location)
    {
        super(location);
        _types = types;
        _operands = operands;
    }

    this(parse.Infix type, Expression left, Expression right, compile.Location location)
    {
        this([type], [left, right], location);
    }

    mixin compile.BranchAcceptor!(_operands);
    mixin helper.Accessor!(_types, _operands);
}