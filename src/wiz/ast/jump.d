module wiz.ast.jump;

import wiz.lib;
import wiz.ast.lib;

class Jump : Statement
{
    private parse.Keyword _type;
    private Expression _destination;
    private JumpCondition _condition;

    this(parse.Keyword type, compile.Location location)
    {
        super(location);
        _type = type;
    }

    this(parse.Keyword type, JumpCondition condition, compile.Location location)
    {
        this(type, location);
        _condition = condition;
    }

    this(parse.Keyword type, Expression destination, JumpCondition condition, compile.Location location)
    {
        this(type, condition, location);
        _destination = destination;
    }

    mixin compile.BranchAcceptor!(_destination, _condition);
    mixin helper.Accessor!(_type, _destination, _condition);
}

class JumpCondition : Node
{
    private bool _negated;
    private string _flag;

    this(bool negated, string flag, compile.Location location)
    {
        super(location);
        _negated = negated;
        _flag = flag;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_negated, _flag);
}