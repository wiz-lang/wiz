module wiz.ast.data;

import wiz.lib;
import wiz.ast.lib;

class Data : Statement
{
    private Storage _storage;
    private Expression[] _items;
    public ubyte[] data;

    this(Storage storage, Expression[] items, compile.Location location)
    {
        super(location);
        _storage = storage;
        _items = items;
    }

    mixin compile.BranchAcceptor!(_storage, _items);
    mixin helper.Accessor!(_storage, _items);
}