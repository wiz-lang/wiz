module wiz.ast.number;

import wiz.lib;
import wiz.ast.lib;

class Number : Expression
{
    parse.Token _numberType;
    size_t _value;

    this(parse.Token numberType, size_t value, compile.Location location)
    {
        super(location);
        _numberType = numberType;
        _value = value;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_numberType, _value);
}