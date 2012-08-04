module wiz.ast.bankdecl;

import wiz.lib;
import wiz.ast.lib;

class BankDecl : Statement
{
    private string[] _names;
    private string _bankType;
    private Expression _size;

    this(string[] names, string bankType, Expression size, compile.Location location)
    {
        super(location);
        _names.length = names.length;
        foreach(i, name; names)
        {
            _names[i] = "bank " ~ name;
        }
        _bankType = bankType;
        _size = size;
    }

    mixin compile.BranchAcceptor!(_size);
    mixin helper.Accessor!(_names, _bankType, _size);

    /*void aggregate()
    {
        foreach(name; names)
        {
            if(bankType == "rom" || bankType == "ram")
            {
                uint result;
                if(size.checkNumber("bank size", result))
                {
                    _bank = new compile.Bank(bankType == "rom", result);
                    compile.environment.put("bank " ~ name, this);
                    compile.program.addBank(_bank);
                }
            }
            else
            {
                error("unknown bank type '" ~ bankType ~ "' in bank definition", location);
            }
        }
    }*/
}