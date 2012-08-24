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

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_block, _far);
}