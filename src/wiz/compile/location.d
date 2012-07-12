module wiz.compile.location;

static import std.conv;

struct Location
{
    public:
        string file;
        uint line;

        this(string file)
        {
            this(file, 0);
        }

        this(string file, uint line)
        {
            this.file = file;
            this.line = line;
        }
        
        this(this)
        {
            file = file;
            line = line;
        }
        
        string toString()
        {
            return file ~ "(" ~ std.conv.to!string(line) ~ ")";
        }
}