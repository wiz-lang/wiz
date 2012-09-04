module wiz.ast.inlinedecl;

import wiz.lib;
import wiz.ast.lib;

class InlineDecl : Statement
{
    private string _name;
    private Statement _stmt;
    private uint _offset;

    this(string name, Statement stmt, compile.Location location)
    {
        super(location);
        _name = name;
        _stmt = stmt;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_name, _stmt);
}