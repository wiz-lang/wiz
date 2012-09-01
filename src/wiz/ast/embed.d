module wiz.ast.embed;

import wiz.lib;
import wiz.ast.lib;

class Embed : Statement
{
    private string _filename;
    public ubyte[] data;

    this(string filename, compile.Location location)
    {
        super(location);
        _filename = filename;
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_filename);
}