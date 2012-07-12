module wiz.ast.bank;

import wiz.lib;
import wiz.ast.lib;

class Bank : Statement
{
    private:
        string[] names;
        string bankType;
        Expression size;
    public:
        this(string[] names, string bankType, Expression size, compile.Location location)
        {
            super(StatementType.BANK, location);
            this.names = names;
            this.bankType = bankType;
            this.size = size;
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