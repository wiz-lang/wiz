module wiz.ast.statement;

import wiz.lib;
import wiz.ast.lib;

abstract class Statement : Node
{
    this(compile.Location location)
    {
        super(location);
    }

    mixin compile.AbstractAcceptor;
}