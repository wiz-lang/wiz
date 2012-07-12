module wiz.ast.expression;

import wiz.lib;
import wiz.ast.lib;

enum ExpressionType
{
    POP, 
    INFIX,
    NUMBER,
    PREFIX,
    STRING,
    POSTFIX,
    ATTRIBUTE,
}

class Expression : Node
{
    private:
        ExpressionType expressionType;
    public:
        this(ExpressionType expressionType, compile.Location location)
        {
            super(location);
            this.expressionType = expressionType;
        }
}