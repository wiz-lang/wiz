module wiz.ast.assignment;

import wiz.lib;
import wiz.ast.lib;

class Assignment : Statement
{
    private:
        Expression dest, intermediary, src;
        parse.Token postfix;

    public:
        this(Expression dest, Expression src, compile.Location location)
        {
            super(StatementType.ASSIGNMENT, location);
            this.dest = dest;
            this.src = src;
        }

        this(Expression dest, Expression intermediary, Expression src, compile.Location location)
        {
            super(StatementType.ASSIGNMENT, location);
            this.dest = dest;
            this.intermediary = intermediary;
            this.src = src;
        }

        this(Expression dest, parse.Token postfix, compile.Location location)
        {
            super(StatementType.ASSIGNMENT, location);
            this.dest = dest;
            this.postfix = postfix;
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