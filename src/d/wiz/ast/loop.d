module wiz.ast.loop;

import wiz.lib;
import wiz.ast.lib;

class Loop : Statement
{
    private Block loop;
    private Block _block;
    private bool _far;
    private bool expanded;

    this(Block loop, bool far, compile.Location location)
    {
        super(location);
        this.loop = loop;
        _far = far;
    }

    bool expand()
    {
        if(expanded)
        {
            return false;
        }

        expanded = true;
        bool tailConditional = false;
        if(loop.statements.length > 0)
        {
            // 'while' or 'until' as last statement of loop?
            if(auto tail = cast(Jump) loop.statements[loop.statements.length - 1])
            {
                switch(tail.type)
                {
                    case parse.Keyword.While:
                        // Tail 'while cond' -> 'continue when cond'.
                        tailConditional = true;
                        auto jump = new Jump(parse.Keyword.Continue, far || tail.far,
                            tail.condition, tail.location
                        );
                        loop.statements[loop.statements.length - 1] = jump;
                        break;
                    case parse.Keyword.Until:
                        // Tail 'until cond' -> 'continue when ~cond'.
                        tailConditional = true;
                        auto jump = new Jump(parse.Keyword.Continue, far || tail.far,
                            new JumpCondition(true, tail.condition), tail.location
                        );
                        loop.statements[loop.statements.length - 1] = jump;
                        break;
                    default:
                }
            }
        }

        // def $loop:
        //   block
        //   goto $loop // (this line is removed if there is a while/until as last statement.
        // def $end:
        Statement[] code;
        code ~= new LabelDecl("$loop", location),
        code ~= loop;
        if(!tailConditional)
        {
            // Unconditional jump,
            code ~= new Jump(parse.Keyword.Goto, far,
                new Attribute(["$loop"], location),
                null, location
            );
        }
        code ~= new LabelDecl("$endloop", location);
        _block = new Block(code, location);

        return true;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block, _far);
}