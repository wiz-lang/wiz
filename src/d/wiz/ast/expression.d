module wiz.ast.expression;

import wiz.lib;
import wiz.ast.lib;

class Expression : Node
{
    enum Max = 65535;
    this(compile.Location location)
    {
        super(location);
    }
}