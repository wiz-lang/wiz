module wiz.ast.pop;

import wiz.lib;
import wiz.ast.lib;

class Pop : Expression
{
    public:
        this(compile.Location location)
        {
            super(ExpressionType.POP, location);
        }
}