module wiz.ast.data;

import wiz.lib;
import wiz.ast.lib;

class Data : Statement
{
    private
        Storage storage;
        DataItem[] items;
    public:
        this(Storage storage, DataItem[] items, compile.Location location)
        {
            super(StatementType.DATA, location);
            this.storage = storage;
            this.items = items;
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

class DataItem : Node
{
    private:
        Expression value;
        
    public:
        this(Expression value, compile.Location location)
        {
            super(location);
            this.value = value;
        }
}