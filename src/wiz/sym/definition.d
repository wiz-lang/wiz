module wiz.sym.definition;

import wiz.lib;
import wiz.sym.lib;

class Definition
{
    ast.Node decl;

    this(ast.Node decl)
    {
        this.decl = decl;
    }
}