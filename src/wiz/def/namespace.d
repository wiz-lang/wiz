module wiz.def.namespace;

import wiz.lib;
import wiz.def.lib;

class Namespace : Definition
{
    private:
        compile.Environment _environment;

    public:
        this(string name, compile.Environment environment, compile.Location location)
        {
            super(name, location);
            this._environment = environment;
        }

        @property compile.Environment environment()
        {
            return _environment;
        }
}