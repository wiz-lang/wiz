module wiz.ast.postfix;

import wiz.lib;
import wiz.ast.lib;

class Postfix : Expression
{
    private parse.Postfix _type;
    private Expression _operand;

    this(parse.Postfix type, Expression operand, compile.Location location)
    {
        super(location);
        _type = type;
        _operand = operand;
    }

    mixin compile.BranchAcceptor!(_operand);
    mixin helper.Accessor!(_type, _operand);
}