module wiz.ast.storage;

import wiz.lib;
import wiz.ast.lib;

class Storage : Node
{
    private parse.Keyword _type;
    private Expression _size;

    this(parse.Keyword type, Expression size, compile.Location location)
    {
        super(location);
        _type = type;
        _size = size;
    }

    mixin compile.BranchAcceptor!(_size);
    mixin helper.Accessor!(_type, _size);
}