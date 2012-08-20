module wiz.ast.assignment;

import wiz.lib;
import wiz.ast.lib;

class Assignment : Statement
{
    private Expression _dest, _intermediary, _src;
    private parse.Postfix _postfix;

    this(Expression dest, Expression src, compile.Location location)
    {
        super(location);
        _dest = dest;
        _src = src;
    }

    this(Expression dest, Expression intermediary, Expression src, compile.Location location)
    {
        super(location);
        _dest = dest;
        _intermediary = intermediary;
        _src = src;
    }

    this(Expression dest, parse.Postfix postfix, compile.Location location)
    {
        super(location);
        _dest = dest;
        _postfix = postfix;
    }

    mixin compile.BranchAcceptor!(_dest, _intermediary, _src);
    mixin helper.Accessor!(_dest, _intermediary, _src, _postfix);
}