module wiz.ast.block;

static import std.stdio;
static import std.string;

import wiz.lib;
import wiz.ast.lib;

class Block : Statement
{
    private string _name;
    private Statement[] _statements;

    this(Statement[] statements, compile.Location location)
    {
        super(location);
        _statements = statements;
    }
    
    this(string name, Statement[] statements, compile.Location location)
    {
        this(statements, location);
        _name = name;
    }

    mixin compile.BranchAcceptor!(_statements);
    mixin helper.Accessor!(_name, _statements);
}
