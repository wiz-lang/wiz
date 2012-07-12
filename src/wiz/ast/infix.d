module wiz.ast.infix;

import wiz.lib;
import wiz.ast.lib;

class Infix : Expression
{
    private:
        parse.Token infixType;
        Expression left, right;
    public:
        this(parse.Token infixType, Expression left, Expression right, compile.Location location)
        {
            super(ExpressionType.INFIX, location);
            this.left = left;
            this.right = right;
        }
}