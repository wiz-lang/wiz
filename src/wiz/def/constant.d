module wiz.def.constant;

import wiz.lib;
import wiz.def.lib;

class Constant : Definition
{
    private:
        ast.Constant _constant;

    public:
        this(string name, ast.Constant constant, compile.Location location)
        {
            super(name, location);
            this._constant = constant;
        }

        @property ast.Constant constant()
        {
            return _constant;
        }
}