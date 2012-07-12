module wiz.ast.command;

import wiz.lib;
import wiz.ast.lib;

class Command : Statement
{
    private:
        parse.Keyword command;
        Expression argument;

    public:
        this(parse.Keyword command, ast.Expression argument, compile.Location location)
        {
            super(StatementType.COMMAND, location);
            this.command = command;
            this.argument = argument;
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