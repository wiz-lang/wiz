module wiz.ast.conditional;

import wiz.lib;
import wiz.ast.lib;

class Conditional : Statement
{
    private JumpCondition _trigger;
    private Statement _action;
    public Statement alternative;

    this(JumpCondition trigger, Statement action, compile.Location location)
    {
        super(location);
        _trigger = trigger;
        _action = action;
    }

    mixin compile.BranchAcceptor!(_trigger, _action, alternative);
    mixin helper.Accessor!(_trigger, _action);
}