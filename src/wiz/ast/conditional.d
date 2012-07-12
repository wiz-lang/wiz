module wiz.ast.conditional;

import wiz.lib;
import wiz.ast.lib;

class Conditional : Statement
{
    private:
        JumpCondition trigger;
        Statement action;
        Statement alternative;
        
    public:
        this(JumpCondition trigger, Statement action, compile.Location location)
        {
            super(StatementType.CONDITIONAL, location);
            this.trigger = trigger;
            this.action = action;
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

        void setAlternative(Statement alternative)
        {
            this.alternative = alternative;
        }
}