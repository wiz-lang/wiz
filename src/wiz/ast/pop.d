module wiz.ast.pop;

import wiz.lib;
import wiz.ast.lib;

class Pop : Expression
{
    this(compile.Location location)
    {
        super(location);
    }
    
    mixin compile.LeafAcceptor;
}