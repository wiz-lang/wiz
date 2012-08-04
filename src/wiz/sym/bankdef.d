module wiz.sym.bankdef;

import wiz.lib;
import wiz.sym.lib;

class BankDef : Definition
{
    compile.Bank bank;

    this(ast.Node decl)
    {
        super(decl);
    }
}