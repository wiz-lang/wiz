module wiz.ast.storage;

import wiz.lib;
import wiz.ast.lib;

class Storage : Node
{
    private parse.Keyword _storageType;
    private Expression _arraySize;

    this(parse.Keyword storageType, Expression arraySize, compile.Location location)
    {
        super(location);
        _storageType = storageType;
        _arraySize = arraySize;
    }

    mixin compile.BranchAcceptor!(_arraySize);
    mixin helper.Accessor!(_storageType, _arraySize);

    /*bool checkSize(string description, ref uint result)
    {
        if(arraySize.checkNumber(description, result))
        {
            if(storageType == parse.Keyword.WORD)
            {
                result *= 2;
            }
            return true;
        }
        return false;
    }*/
}