module wiz.ast.prefix;

import wiz.lib;
import wiz.ast.lib;

class Prefix : Expression
{
    private parse.Prefix _type;
    private Expression _operand;

    this(parse.Prefix type, Expression operand, compile.Location location)
    {
        super(location);
        _type = type;
        _operand = operand;
    }

    mixin compile.BranchAcceptor!(_operand);
    mixin helper.Accessor!(_type, _operand);
}