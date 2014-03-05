module wiz.ast.node;

import wiz.lib;

class Node
{
    private compile.Location _location;

    this(compile.Location location)
    {
        _location = location;
    }

    mixin compile.AbstractAcceptor;
    mixin helper.Accessor!(_location);
}