module wiz.sym.labeldef;

import wiz.lib;
import wiz.sym.lib;

class LabelDef : Definition
{
    bool hasAddress;
    uint address;
    
    this(ast.Node decl)
    {
        super(decl);
    }
}