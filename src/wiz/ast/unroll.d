module wiz.ast.unroll;

import wiz.lib;
import wiz.ast.lib;

class Unroll : Statement
{
    private Expression _repetitions;
    private Block _block;

    this(Expression repetitions, Block block, compile.Location location)
    {
        super(location);
        _repetitions = repetitions;
        _block = block;
    }

    void expand(uint times)
    {
        Statement[] code;
        foreach(i; 0 .. times)
        {
            code ~= _block;
        }
        _block = new Block(code, location);
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_repetitions, _block);
}