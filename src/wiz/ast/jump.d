module wiz.ast.jump;

import wiz.lib;
import wiz.ast.lib;

class Jump : Statement
{
    private bool _far;
    private parse.Keyword _type;
    private Expression _destination;
    private JumpCondition _condition;

    this(parse.Keyword type, bool far, compile.Location location)
    {
        super(location);
        _type = type;
    }

    this(parse.Keyword type, bool far, JumpCondition condition, compile.Location location)
    {
        this(type, far, location);
        _condition = condition;
    }

    this(parse.Keyword type, bool far, Expression destination, JumpCondition condition, compile.Location location)
    {
        this(type, far, condition, location);
        _far = far;
        _destination = destination;
    }

    mixin compile.BranchAcceptor!(_destination, _condition);
    mixin helper.Accessor!(_type, _far, _destination, _condition);
}

class JumpCondition : Node
{
    private bool _negated;
    private Attribute _attr;
    private parse.Branch _branch;

    this(bool negated, Attribute attr, compile.Location location)
    {
        super(location);
        _negated = negated;
        _attr = attr;
    }

    this(bool negated, parse.Branch branch, compile.Location location)
    {
        super(location);
        _negated = negated;
        _branch = branch;
    }

    mixin compile.BranchAcceptor!(_attr);
    mixin helper.Accessor!(_negated, _attr, _branch);
}