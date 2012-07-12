module wiz.ast.string;

import wiz.lib;
import wiz.ast.lib;

class String : Expression
{
    private:
        string value;

    public:
        this(string value, compile.Location location)
        {
            super(ExpressionType.STRING, location);
            this.value = value;
        }
}