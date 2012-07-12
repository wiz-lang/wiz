module wiz.ast.variable;

import wiz.lib;
import wiz.ast.lib;

class Variable : Statement
{
    private:
        string[] names;
        Storage storage;
    public:
        this(string[] names, Storage storage, compile.Location location)
        {
            super(StatementType.VARIABLE, location);
            this.names = names;
            this.storage = storage;
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