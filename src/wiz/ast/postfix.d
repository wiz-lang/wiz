module wiz.ast.postfix;

import wiz.lib;
import wiz.ast.lib;

class Postfix : Expression
{
    private:
        parse.Token postfixType;
        Expression operand;
    public:
        this(parse.Token postfixType, Expression operand, compile.Location location)
        {
            super(ExpressionType.POSTFIX, location);
            this.operand = operand;
        }
}