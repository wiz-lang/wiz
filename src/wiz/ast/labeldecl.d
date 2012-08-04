module wiz.ast.labeldecl;

import wiz.lib;
import wiz.ast.lib;

class LabelDecl : Statement
{
    private string _name;

    this(string name, compile.Location location)
    {
        super(location);
        _name = name;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_name);
}