module wiz.ast.bankdecl;

import wiz.lib;
import wiz.ast.lib;

class BankDecl : Statement
{
    private string[] _names;
    private string _type;
    private Expression _size;

    this(string[] names, string type, Expression size, compile.Location location)
    {
        super(location);
        _names.length = names.length;
        foreach(i, name; names)
        {
            _names[i] = "bank " ~ name;
        }
        _type = type;
        _size = size;
    }

    mixin compile.BranchAcceptor!(_size);
    mixin helper.Accessor!(_names, _type, _size);
}