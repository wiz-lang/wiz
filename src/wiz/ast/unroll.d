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

    mixin compile.BranchAcceptor!(_repetitions);
    mixin helper.Accessor!(_repetitions, _block);
}