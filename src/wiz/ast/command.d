module wiz.ast.command;

import wiz.lib;
import wiz.ast.lib;

class Command : Statement
{
    private parse.Keyword _type;
    private Expression _argument;

    this(parse.Keyword type, ast.Expression argument, compile.Location location)
    {
        super(location);
        _type = type;
        _argument = argument;
    }

    mixin compile.BranchAcceptor!(_argument);
    mixin helper.Accessor!(_type, _argument);
}