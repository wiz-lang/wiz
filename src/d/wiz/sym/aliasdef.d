module wiz.sym.aliasdef;

import wiz.lib;
import wiz.sym.lib;

class AliasDef : Definition
{
    Definition definition;

    this(ast.Node decl, Definition definition)
    {
        super(decl);
        this.definition = definition;
    }
}