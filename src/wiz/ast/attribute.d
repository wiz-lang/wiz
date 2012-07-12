module wiz.ast.attribute;

import wiz.lib;
import wiz.ast.lib;

class Attribute : Expression
{
    private:
        string[] pieces;
        
    public:
        this(string[] pieces, compile.Location location)
        {
            super(ExpressionType.ATTRIBUTE, location);
            this.pieces = pieces;
        }
        
        string getFullName()
        {
            return std.string.join(pieces, ".");
        }
}