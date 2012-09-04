module wiz.ast.unroll;

import wiz.lib;
import wiz.ast.lib;

class Unroll : Statement
{
    private Expression _repetitions;
    private Statement _stmt;

    this(Expression repetitions, Statement stmt, compile.Location location)
    {
        super(location);
        _repetitions = repetitions;
        _stmt = stmt;
    }

    mixin compile.BranchAcceptor!(_repetitions);
    mixin helper.Accessor!(_repetitions, _stmt);
}