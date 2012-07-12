module wiz.ast.label;

import wiz.lib;
import wiz.ast.lib;

class Label : Statement
{
    private:
        string name;
    public:
        this(string name, compile.Location location)
        {
            super(StatementType.LABEL, location);
            this.name = name;
        }

        void aggregate()
        {
            compile.getActiveEnv().put(new def.Label(name, this, location));
        }

        void validate()
        {
        }

        void generate()
        {
        }
}