module wiz.sym.definition;

import wiz.lib;
import wiz.sym.lib;

class Definition
{
    ast.Node decl;
    bool visited;

    this(ast.Node decl)
    {
        this.decl = decl;
    }
}