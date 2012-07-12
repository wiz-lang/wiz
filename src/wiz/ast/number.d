module wiz.ast.number;

import wiz.lib;
import wiz.ast.lib;

class Number : Expression
{
    private:
        parse.Token numberType;
        uint value;
    public:
        this(parse.Token numberType, uint value, compile.Location location)
        {
            super(ExpressionType.NUMBER, location);
            this.numberType = numberType;
            this.value = value;
        }
}