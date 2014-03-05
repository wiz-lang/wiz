module wiz.ast.relocation;

import wiz.lib;
import wiz.ast.lib;

class Relocation : Statement
{
    private string _name;
    private string _mangledName;
    private Expression _dest;

    this(string name, Expression dest, compile.Location location)
    {
        super(location);
        _name = name;
        _mangledName = "bank " ~ name;
        _dest = dest;
    }

    mixin compile.BranchAcceptor!(_dest);
    mixin helper.Accessor!(_name, _mangledName, _dest);
}