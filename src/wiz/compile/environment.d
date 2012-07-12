module wiz.compile.environment;

import wiz.lib;

static import std.array;

class Environment
{
    private:
        Environment parent;
        def.Definition[string] dictionary;

    public:
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
        
        void put(def.Definition d)
        {
            // Perform search without inheritance to only whine if the symbol was already declared in this scope.
            // This way functions can have locals that use the same name as somewhere in the parent, without problems.
            // (get always looks at current scope and works up, there is no ambiguity)
            def.Definition match = tryGet(d.name, false);
        
            if(match)
            {
                error("redefinition of symbol '" ~ d.name ~ "' (previously defined at " ~ match.location.toString() ~ ")", d.location);
            }
            else
            {
                dictionary[d.name] = d;
            }
        }
        
        def.Definition tryGet(string name, bool useInheritance = true)
        {
            def.Definition* match = name in dictionary;
            if(match is null)
            {
                if(useInheritance && parent !is null)
                {
                    return parent.tryGet(name, useInheritance);
                }
                return null;
            }
            else
            {
                return *match;
            }
        }
}

private Environment activeEnv;
private Environment builtinEnv;
private Environment[] environments;

void enterEnv(Environment env)
{
    environments ~= env;
    activeEnv = env;
}

void exitEnv()
{
    std.array.popBack(environments);
    if(environments.length > 0)
    {
        activeEnv = std.array.back(environments);
    }
    else
    {
        activeEnv = null;
    }
}

Environment getActiveEnv()
{
    return activeEnv;
}

Environment getBuiltinEnv()
{
    if(builtinEnv is null)
    {
        builtinEnv = new Environment();
        compile.Location builtinLocation = compile.Location("<builtin>");
        /*
        builtinEnv.put(new RegisterDefinition("a", RegisterType.A, builtinLocation));
        builtinEnv.put(new RegisterDefinition("x", RegisterType.X, builtinLocation));
        builtinEnv.put(new RegisterDefinition("y", RegisterType.Y, builtinLocation));
        builtinEnv.put(new RegisterDefinition("s", RegisterType.S, builtinLocation));
        builtinEnv.put(new RegisterDefinition("p", RegisterType.P, builtinLocation));

        PackageDefinition pkg = new PackageDefinition("builtin", new Environment(builtinEnv), builtinLocation);
        builtinEnv.put(pkg);

        auto env = pkg.getTable();
        env.put(new RegisterDefinition("a", RegisterType.A, builtinLocation));
        env.put(new RegisterDefinition("x", RegisterType.X, builtinLocation));
        env.put(new RegisterDefinition("y", RegisterType.Y, builtinLocation));
        env.put(new RegisterDefinition("s", RegisterType.S, builtinLocation));
        env.put(new RegisterDefinition("p", RegisterType.P, builtinLocation));*/
    }
    return builtinEnv;
}


