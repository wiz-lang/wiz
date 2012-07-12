module wiz.ast.constant;

import wiz.lib;
import wiz.ast.lib;

class Constant : Statement
{
    private:
        string name;
        Expression value;
        uint offset;
        
    public:
        this(string name, Expression value, compile.Location location)
        {
            super(StatementType.CONSTANT, location);
            this.name = name;
            this.value = value;
            this.offset = 0;
        }
        
        this(string name, Expression value, uint offset, compile.Location location)
        {
            super(StatementType.CONSTANT, location);
            this.name = name;
            this.value = value;
            this.offset = offset;
        }

        void aggregate()
        {
            compile.getActiveEnv().put(new def.Constant(name, this, location));
        }

        void validate()
        {
        }

        void generate()
        {
        }
}