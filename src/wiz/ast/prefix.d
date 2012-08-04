module wiz.ast.prefix;

import wiz.lib;
import wiz.ast.lib;

class Prefix : Expression
{
    private parse.Token _prefixType;
    private Expression _operand;

    this(parse.Token prefixType, Expression operand, compile.Location location)
    {
        super(location);
        _prefixType = prefixType;
        _operand = operand;
    }

    mixin compile.BranchAcceptor!(_operand);
    mixin helper.Accessor!(_prefixType, _operand);
}