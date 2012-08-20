module wiz.ast.vardecl;

import wiz.lib;
import wiz.ast.lib;

class VarDecl : Statement
{
    string[] _names;
    Storage _storage;

    this(string[] names, Storage storage, compile.Location location)
    {
        super(location);
        _names = names;
        _storage = storage;
    }

    mixin compile.BranchAcceptor!(_storage);
    mixin helper.Accessor!(_names, _storage);
}