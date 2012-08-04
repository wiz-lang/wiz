module wiz.ast.enumdecl;

import wiz.lib;
import wiz.ast.lib;

class EnumDecl : Statement
{
    private ConstDecl[] _constants;

    this(ConstDecl[] constants, compile.Location location)
    {
        super(location);
        _constants = constants;
    }

    mixin compile.BranchAcceptor!(_constants);
    mixin helper.Accessor!(_constants);
}