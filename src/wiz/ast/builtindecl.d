module wiz.ast.builtindecl;

import wiz.lib;
import wiz.ast.lib;

class BuiltinDecl : Statement
{
    enum LocationName = "<builtin>";
    static compile.Location builtinLocation;

    static this()
    {
        builtinLocation = compile.Location(LocationName);
    }

    string _name;

    this()
    {
        super(builtinLocation);
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_name);
}