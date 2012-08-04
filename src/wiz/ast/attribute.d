module wiz.ast.attribute;

import wiz.lib;
import wiz.ast.lib;

class Attribute : Expression
{
    private string[] _pieces;

    this(string[] pieces, compile.Location location)
    {
        super(location);
        _pieces = pieces;
    }
    
    @property string fullName()
    {
        return std.string.join(pieces, ".");
    }

    mixin compile.LeafAcceptor;
    mixin helper.Accessor!(_pieces);
}