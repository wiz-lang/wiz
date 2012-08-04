module wiz.ast.loop;

import wiz.lib;
import wiz.ast.lib;

class Loop : Statement
{
    private Block _block;

    this(Block block, compile.Location location)
    {
        super(location);
        _block = block;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block);
}