module wiz.def.label;

import wiz.lib;
import wiz.def.lib;

class Label : Definition
{
    private:
        ast.Label _label;

    public:
        this(string name, ast.Label label, compile.Location location)
        {
            super(name, location);
            this._label = label;
        }

        @property ast.Label label()
        {
            return _label;
        }
}