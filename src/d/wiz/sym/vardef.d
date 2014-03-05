module wiz.sym.vardef;

import wiz.lib;
import wiz.sym.lib;

class VarDef : Definition
{
    bool hasAddress;
    uint address;
    
    this(ast.Node decl)
    {
        super(decl);
    }
}