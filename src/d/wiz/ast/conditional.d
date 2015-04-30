module wiz.ast.conditional;

import wiz.lib;
import wiz.ast.lib;

class Conditional : Statement
{
    private JumpCondition trigger;
    private bool far;
    private Statement prelude;
    private Statement action;
    public Statement alternative;

    private Block _block;
    private bool expanded;

    this(JumpCondition trigger, bool far, Statement prelude, Statement action, compile.Location location)
    {
        super(location);
        this.trigger = trigger;
        this.far = far;
        this.prelude = prelude;
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
        if(trigger && trigger.expr)
        {
            size_t value;
            bool known;
            if(compile.foldExpression(program, trigger.expr, false, value, known) && known)
            {
                if(value && !trigger.negated)
                {
                    _block = new Block([
                        // prelude
                        prelude,
                        // action
                        action
                    ], location);
                }
                else if(alternative)
                {
                    _block = new Block([
                        // prelude
                        prelude,
                        // action
                        alternative
                    ], location);
                }
                return true;
            }
        }


        // Run-time branching.
        Statement[] statements;
        if(alternative is null)
        {
            _block = new Block([
                // prelude
                prelude,
                // goto $endif when ~trigger
                new Jump(parse.Keyword.Goto, far,
                    new Attribute(["$endif"], location),
                    new JumpCondition(true, trigger), location
                ),
                // action
                action,
                // def $endif:
                new LabelDecl("$endif", location)
            ], location);
        }
        else
        {
            _block = new Block([
                // prelude
                prelude,
                // goto $else when ~trigger
                new Jump(parse.Keyword.Goto, far,
                    new Attribute(["$else"], location),
                    new JumpCondition(true, trigger), location
                ),
                // action
                action,
                // goto $endif
                new Jump(parse.Keyword.Goto, far,
                    new Attribute(["$endif"], location),
                    null, location
                ),
                // def $else:
                new LabelDecl("$else", location),
                //   alternative
                alternative,
                // def $endif:
                new LabelDecl("$endif", location)
            ], location);
        }

        return true;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block);
}