module wiz.sym.packagedef;

import wiz.lib;
import wiz.sym.lib;

class PackageDef : Definition
{
    compile.Environment environment;

    this(ast.Node decl, compile.Environment environment)
    {
        super(decl);
        this.environment = environment;
    }
}