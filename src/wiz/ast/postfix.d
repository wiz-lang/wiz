module wiz.ast.postfix;

import wiz.lib;
import wiz.ast.lib;

class Postfix : Expression
{
    private parse.Token _postfixType;
    private Expression _operand;

    this(parse.Token postfixType, Expression operand, compile.Location location)
    {
        super(location);
        _postfixType = postfixType;
        _operand = operand;
    }

    mixin compile.BranchAcceptor!(_operand);
    mixin helper.Accessor!(_postfixType, _operand);
}