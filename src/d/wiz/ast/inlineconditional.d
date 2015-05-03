module wiz.ast.inlineconditional;

import wiz.lib;
import wiz.ast.lib;

class InlineConditional : Statement
{
    private bool negated;
    private Expression trigger;
    private Statement action;
    public Statement alternative;

    private Block _block;
    private bool expanded;

    this(bool negated, Expression trigger, Statement action, compile.Location location)
    {
        super(location);
        this.negated = negated;
        this.trigger = trigger;
        this.action = action;
    }

    bool expand(compile.Program program)
    {
        if(expanded)
        {
            return false;
        }

        expanded = true;

        // Compile-time branching.
        if(trigger)
        {
            size_t value;
            bool known;
            if(compile.foldExpression(program, trigger, false, value, known) && known)
            {
                if(value && !negated)
                {
                    _block = new Block([action], location);
                }
                else if(alternative)
                {
                    _block = new Block([alternative], location);
                }
                return true;
            }
        }
        return true;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block);
}