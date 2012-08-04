module wiz.sym.vardef;

import wiz.lib;
import wiz.sym.lib;

class VarDef : Definition
{
    uint address;
    
    this(ast.Node decl)
    {
        super(decl);
    }
}