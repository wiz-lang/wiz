module wiz.ast.conditional;

import wiz.lib;
import wiz.ast.lib;

class Conditional : Statement
{
    private JumpCondition _trigger;
    private bool _far;
    private Statement _action;
    public Statement alternative;

    this(JumpCondition trigger, bool far, Statement action, compile.Location location)
    {
        super(location);
        _trigger = trigger;
        _far = far;
        _action = action;
    }

    mixin compile.BranchAcceptor!(_trigger, _action, alternative);
    mixin helper.Accessor!(_trigger, _far, _action);
}