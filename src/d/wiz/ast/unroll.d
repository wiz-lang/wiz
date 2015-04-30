module wiz.ast.unroll;

import wiz.lib;
import wiz.ast.lib;

class Unroll : Statement
{
    private Expression _repetitions;
    private Block _block;

    private bool expanded;

    this(Expression repetitions, Block block, compile.Location location)
    {
        super(location);
        _repetitions = repetitions;
        _block = block;
    }

    bool expand(compile.Program program)
    {
        if(expanded)
        {
            return false;
        }

        expanded = true;

        size_t times;
        if(!compile.foldExpression(program, repetitions, true, times))
        {
            return true;
        }

        Statement[] code;
        foreach(i; 0 .. times)
        {
            code ~= new Block(_block.statements, location);
        }
        _block = new Block(code, location);

        return true;
    }

    mixin compile.BranchAcceptor!(_block);
    mixin helper.Accessor!(_repetitions, _block);
}