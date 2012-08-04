module wiz.compile.environment;

import wiz.lib;
import wiz.compile.lib;

static import std.array;

class Environment
{
    private Environment parent;
    private sym.Definition[string] dictionary;

    this(Environment parent = null)
    {
        this.parent = parent;
    }
    
    void printKeys()
    {
        foreach(key, value; dictionary)
        {
            std.stdio.writeln(key ~ ": " ~ value.toString());
        }
    }
    
    void put(string name, sym.Definition def)
    {
        auto match = get!(sym.Definition)(name, true);
        if(match)
        {
            error("redefinition of symbol '" ~ name ~ "'", def.decl.location, false, true);
            error("(previously defined here)", match.decl.location);
        }
        else
        {
            dictionary[name] = def;
        }
    }

    T get(T)(string name, bool shallow = false)
    {
        sym.Definition match = dictionary.get(name, null);
        if(match is null)
        {
            if(shallow || parent is null)
            {
                return null;
            }
            else
            {
                return parent.get!(T)(name, shallow);
            }
        }
        else
        {
            return cast(T) match;
        }
    }
}


