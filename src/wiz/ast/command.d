module wiz.ast.command;

import wiz.lib;
import wiz.ast.lib;

class Command : Statement
{
    private parse.Keyword _command;
    private Expression _argument;

    this(parse.Keyword command, ast.Expression argument, compile.Location location)
    {
        super(location);
        _command = command;
        _argument = argument;
    }

    mixin compile.BranchAcceptor!(_argument);
    mixin helper.Accessor!(_command, _argument);
}