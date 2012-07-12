module wiz.ast.enumeration;

import wiz.lib;
import wiz.ast.lib;

class Enumeration : Statement
{
    private:
        Constant[] constants;

    public:
        this(Constant[] constants, compile.Location location)
        {
            super(StatementType.ENUMERATION, location);
            this.constants = constants;
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