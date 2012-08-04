module wiz.ast.constdecl;

import wiz.lib;
import wiz.ast.lib;

class ConstDecl : Statement
{
    private string _name;
    private Expression _value;
    private uint _offset;

    this(string name, Expression value, compile.Location location)
    {
        super(location);
        _name = name;
        _value = value;
        _offset = 0;
    }
    
    this(string name, Expression value, uint offset, compile.Location location)
    {
        super(location);
        _name = name;
        _value = value;
        _offset = offset;
    }

    mixin compile.BranchAcceptor!(_value);
    mixin helper.Accessor!(_name, _value, _offset);
}