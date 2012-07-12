module wiz.ast.embed;

import wiz.lib;
import wiz.ast.lib;

class Embed : Statement
{
    private:
        string filename;

    public:
        this(string filename, compile.Location location)
        {
            super(StatementType.EMBED, location);
            this.filename = filename;
        }
            
        void aggregate()
        {

        }
        
        void validate()
        {

        }
        
        void generate()
        {

        }
}