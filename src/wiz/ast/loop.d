module wiz.ast.loop;

import wiz.lib;
import wiz.ast.lib;

class Loop : Statement
{
    private:
        Block block;

    public:
        this(Block block, compile.Location location)
        {
            super(StatementType.LOOP, location);
            this.block = block;
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