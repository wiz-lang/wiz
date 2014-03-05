module wiz.ast.loop;

import wiz.lib;
import wiz.ast.lib;

class Loop : Statement
{
    private Block _block;
    private bool _far;

    this(Block block, bool far, compile.Location location)
    {
        super(location);
        _block = block;
        _far = far;
    }

    void expand()
    {
        bool tailConditional = false;
        if(block.statements.length > 0)
        {
            // 'while' or 'until' as last statement of loop?
            if(auto tail = cast(Jump) block.statements[block.statements.length - 1])
            {
                switch(tail.type)
                {
                    case parse.Keyword.While:
                        // Tail 'while cond' -> 'continue when cond'.
                        tailConditional = true;
                        auto jump = new Jump(parse.Keyword.Continue, far || tail.far,
                            tail.condition, tail.location
                        );
                        block.statements[block.statements.length - 1] = jump;
                        break;
                    case parse.Keyword.Until:
                        // Tail 'until cond' -> 'continue when ~cond'.
                        tailConditional = true;
                        auto jump = new Jump(parse.Keyword.Continue, far || tail.far,
                            new JumpCondition(true, tail.condition), tail.location
                        );
                        block.statements[block.statements.length - 1] = jump;
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
        code ~= _block;
        if(!tailConditional)
        {
            // Remove unconditional jump,
            code ~= new Jump(parse.Keyword.Goto, far,
                new Attribute(["$loop"], location),
                null, location
            );
        }
        code ~= new LabelDecl("$end", location);
        _block = new Block(code, location);
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block, _far);
}