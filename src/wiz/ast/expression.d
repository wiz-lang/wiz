module wiz.ast.expression;

import wiz.lib;
import wiz.ast.lib;

class Expression : Node
{
    this(compile.Location location)
    {
        super(location);
    }

    mixin compile.AbstractAcceptor;
}