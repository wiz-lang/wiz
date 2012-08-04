module wiz.ast.string;

import wiz.lib;
import wiz.ast.lib;

class String : Expression
{    
    string _value;

    this(string value, compile.Location location)
    {
        super(location);
        _value = value;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_value);
}