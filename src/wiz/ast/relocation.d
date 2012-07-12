module wiz.ast.relocation;

import wiz.lib;
import wiz.ast.lib;

class Relocation : Statement
{
    private:
        string name;
        Expression dest;

    public:
        this(string name, Expression dest, compile.Location location)
        {
            super(StatementType.RELOCATION, location);
            this.name = name;
            this.dest = dest;
        }

        void aggregate()
        {

        }
        
        void validate()
        {

        }
        
        void generate()
        {

        }
}