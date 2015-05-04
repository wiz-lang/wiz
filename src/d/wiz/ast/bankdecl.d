module wiz.ast.bankdecl;

import wiz.lib;
import wiz.ast.lib;

class BankDecl : Statement
{
    private string _name;
    private string _type;
    private Expression _index;
    private Expression _size;

    this(string name, string type, Expression index, Expression size, compile.Location location)
    {
        super(location);
        _name = "bank " ~ name;
        _type = type;
        _index = index;
        _size = size;
    }

    mixin compile.BranchAcceptor!(_index, _size);
    mixin helper.Accessor!(_name, _type, _index, _size);
}