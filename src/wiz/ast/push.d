module wiz.ast.push;

import wiz.lib;
import wiz.ast.lib;

class Push : Statement
{
    private Expression _src, _intermediary;

    this(ast.Expression src, compile.Location location)
    {
        super(location);
        _src = src;
    }

    this(ast.Expression src, ast.Expression intermediary, compile.Location location)
    {
        super(location);
        _src = src;
        _intermediary = intermediary;
    }

    mixin compile.BranchAcceptor!(_src, _intermediary);
    mixin helper.Accessor!(_src, _intermediary);
}