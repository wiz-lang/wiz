module wiz.ast.data;

import wiz.lib;
import wiz.ast.lib;

class Data : Statement
{
    private Storage _storage;
    private DataItem[] _items;

    this(Storage storage, DataItem[] items, compile.Location location)
    {
        super(location);
        _storage = storage;
        _items = items;
    }

    mixin compile.BranchAcceptor!(_storage, _items);
}

class DataItem : Node
{
    private Expression _value;

    this(Expression value, compile.Location location)
    {
        super(location);
        _value = value;
    }

    mixin compile.BranchAcceptor!(_value);
    mixin helper.Accessor!(_value);
}