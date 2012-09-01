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
            // 'while' or 'until' as last statement of loop? Remove unconditional jump.
            if(auto tail = cast(Jump) block.statements[block.statements.length - 1])
            {
                tailConditional = tail.type == parse.Keyword.While || tail.type == parse.Keyword.Until;
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