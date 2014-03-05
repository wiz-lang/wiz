module wiz.sym.constdef;

import wiz.lib;
import wiz.sym.lib;

class ConstDef : Definition
{
    compile.Environment environment;
    this(ast.Node decl, compile.Environment environment)
    {
        super(decl);
        this.environment = environment;
    }
}