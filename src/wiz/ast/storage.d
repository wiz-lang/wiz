module wiz.ast.storage;

import wiz.lib;
import wiz.ast.lib;

class Storage : Node
{
    private:
        parse.Keyword storageType;
        Expression arraySize;

    public:
        this(parse.Keyword storageType, Expression arraySize, compile.Location location)
        {
            super(location);
            this.storageType = storageType;
            this.arraySize = arraySize;
        }
}