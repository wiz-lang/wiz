module wiz.ast.jump;

import wiz.lib;
import wiz.ast.lib;

class Jump : Statement
{
    private:
        parse.Keyword type;
        Expression destination;
        JumpCondition condition;
        
    public:
        this(parse.Keyword type, compile.Location location)
        {
            super(StatementType.JUMP, location);
            this.type = type;
        }

        this(parse.Keyword type, JumpCondition condition, compile.Location location)
        {
            this(type, location);
            this.condition = condition;
        }

        this(parse.Keyword type, Expression destination, JumpCondition condition, compile.Location location)
        {
            this(type, condition, location);
            this.destination = destination;
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

class JumpCondition : Node
{
    private:
        bool negated;
        string flag;

    public:
        this(bool negated, string flag, compile.Location location)
        {
            super(location);
            this.negated = negated;
            this.flag = flag;
        }
}