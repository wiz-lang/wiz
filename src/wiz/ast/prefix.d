module wiz.ast.prefix;

import wiz.lib;
import wiz.ast.lib;

class Prefix : Expression
{
    private:
        parse.Token prefixType;
        Expression operand;
    public:
        this(parse.Token prefixType, Expression operand, compile.Location location)
        {
            super(ExpressionType.PREFIX, location);
            this.operand = operand;
        }
}