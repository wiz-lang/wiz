module wiz.sym.funcdef;

import wiz.lib;
import wiz.sym.lib;

class FuncDef : Definition
{
    this(ast.Node decl)
    {
        super(decl);
    }
}