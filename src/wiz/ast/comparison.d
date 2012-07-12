module wiz.ast.comparison;

import wiz.lib;
import wiz.ast.lib;

class Comparison : Statement
{
    private:
        Expression left, right;
        
    public:
        this(Expression left, Expression right, compile.Location location)
        {
            super(StatementType.COMPARISON, location);
            this.left = left;
            this.right = right;
        }

        this(Expression left, compile.Location location)
        {
            super(StatementType.COMPARISON, location);
            this.left = left;
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