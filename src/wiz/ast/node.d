module wiz.ast.node;

import wiz.lib;

class Node
{
    private:
        compile.Location _location;
        
    public:
        this(compile.Location location)
        {
            this._location = location;
        }
        
        @property compile.Location location()
        {
            return _location;
        }
}