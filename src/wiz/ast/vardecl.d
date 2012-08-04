module wiz.ast.vardecl;

import wiz.lib;
import wiz.ast.lib;

class VarDecl : Statement
{
    string[] _names;
    Storage _storage;

    this(string[] names, Storage storage, compile.Location location)
    {
        super(location);
        _names = names;
        _storage = storage;
    }

    mixin compile.BranchAcceptor!(_storage);
    mixin helper.Accessor!(_names, _storage);

    /*void aggregate()
    {
        enum DESCRIPTION = "variable declaration";
        foreach(name; names)
        {
            uint size;
            if(storage.checkSize("array width", size))
            {
                compile.Bank bank = compile.program.checkActiveBank(DESCRIPTION, location);
                _address = bank.checkAddress(DESCRIPTION, location);
                bank.reserveVirtual(DESCRIPTION, size, location);

                compile.environment.put(name, this);
            }
        }
    }*/
}