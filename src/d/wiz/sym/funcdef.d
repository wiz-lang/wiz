module wiz.sym.funcdef;

import wiz.lib;
import wiz.sym.lib;

class FuncDef : Definition
{
    compile.Environment environment;
    this(ast.Node decl, compile.Environment environment)
    {
        super(decl);
        this.environment = environment;
    }
}