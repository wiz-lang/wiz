module wiz.def.definition;

import wiz.lib;
import wiz.def.lib;

class Definition
{
    private:
        string _name;
        compile.Location _location;
        
    public:
        this(string name, compile.Location location)
        {
            this._name = name;
            this._location = location;
        }

        @property string name()
        {
            return _name;
        }
        
        @property compile.Location location()
        {
            return _location;
        }
}