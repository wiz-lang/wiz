module wiz.compile.environment;

import wiz.lib;
import wiz.compile.lib;

static import std.array;
//static import std.digest.md;
static import std.base64;

class Environment
{
    private static size_t blockIndex = 0;
    static string generateBlockName()
    {
        return std.string.format("%%%X%%", blockIndex++);
    }

    private string name;
    private sym.Definition[string] dictionary;
    Environment parent;

    this()
    {
        this.name = "";
        this.parent = null;
    }

    this(string name, Environment parent)
    {
        this.name = name;
        this.parent = parent;
    }

    string getFullName()
    {
        if(parent is null)
        {
            return name;
        }
        else
        {
            if(name.length && name[0] == '%')
            {
                return name;
            }
            string parentName = parent.getFullName();
            return (parentName.length ? parentName ~ "." : "") ~ name;
        }
    }
    
    void printKeys()
    {
        foreach(key, value; dictionary)
        {
            std.stdio.writeln(key ~ ": " ~ value.toString());
        }
    }

    bool remove(string name)
    {
        return dictionary.remove(name);
    }
    
    void put(string name, sym.Definition def)
    {
        auto match = get(name, true);
        if(match && match.decl != def.decl)
        {
            error("redefinition of symbol '" ~ name ~ "'", def.decl.location, false, true);
            error("(previously defined here)", match.decl.location);
        }
        else
        {
            dictionary[name] = def;
        }
    }

    sym.Definition get(string name, bool shallow = false)
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
                return parent.get(name, shallow);
            }
        }
        else
        {
            while(cast(sym.AliasDef) match)
            {
                match = (cast(sym.AliasDef) match).definition;
            }
            return match;
        }
    }
}


