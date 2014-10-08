module wiz.compile.program;

import wiz.lib;
import wiz.compile.lib;

class NodeScope
{
    private Environment[] environments;
    private size_t index;

    this(Environment environment)
    {
        environments = [environment];
        index = environments.length;
    }

    void rewind()
    {
        index = 0;
    }

    void add(Environment environment)
    {
        environments ~= environment;
        index = environments.length;
    }

    Environment next()
    {
        if(index < environments.length)
        {
            return environments[index++];
        }
        else
        {
            return null;
        }
    }
}

class Program
{
    private Environment _environment;

    private Bank bank;
    private Bank[] banks;

    private NodeScope[ast.Node] scopes;
    private Environment[] environments;
    private ast.Node[] inlines;
    private bool[ast.Node] inlined;
    bool finalized;

    // An label and size that may be associated with a global address in the program, used by debug symbols.
    // A debugger will expect that each address can have exactly one label, so this is reflected here.
    // This also won't verify for conflicts when two symbols alias or have overlapping regions.
    string[size_t] addressNames;
    size_t[size_t] addressSizes;

    cpu.Platform platform;
        
    this(cpu.Platform platform)
    {
        this.bank = null;
        this.platform = platform;
        clearEnvironment();
    }

    mixin helper.Accessor!(_environment);

    void rewind()
    {
        foreach(i, s; scopes)
        {
            s.rewind();
        }
        foreach(i, b; banks)
        {
            b.rewind();
        }
        bank = null;
    }

    void registerAddress(size_t address, string name, size_t size, compile.Location location)
    {
        if(name && name[0] == '$')
        {
            name = std.string.format(".%s.%04X", name[1 .. name.length], address);
        }
        addressNames[address] = name;
        addressSizes[address] = size;
    }

    void addBank(Bank b)
    {
        banks ~= b;
    }

    Bank checkBank(string description, compile.Location location)
    {
        if(bank is null)
        {
            error(description ~ " is not allowed before an 'in' statement.", location, true);
            return null;
        }
        else
        {
            return bank;
        }
    }

    void switchBank(Bank b)
    {
        bank = b;
    }

    void clearEnvironment()
    {
        auto builtins = platform.builtins();
        auto env = new Environment();
        auto pkg = new sym.PackageDef(new ast.BuiltinDecl(), new Environment(env));
        
        foreach(name, builtin; builtins)
        {
            env.put(name, builtin);
        }
        env.put("builtin", pkg);

        env = pkg.environment;
        foreach(name, builtin; builtins)
        {
            env.put(name, builtin);
        }

        _environment = env;
        scopes = null;
        environments = [env];
    }

    void createNodeEnvironment(ast.Node node, Environment environment)
    {
        NodeScope match = scopes.get(node, null);
        if(match)
        {
            match.add(environment);
        }
        else
        {
            scopes[node] = new NodeScope(environment);
        }
    }

    Environment nextNodeEnvironment(ast.Node node)
    {
        NodeScope match = scopes.get(node, null);
        if(match)
        {
            return match.next();
        }
        else
        {
            return null;
        }
    }

    void enterEnvironment(Environment e)
    {
        environments ~= e;
        _environment = e;
    }

    void leaveEnvironment()
    {
        std.array.popBack(environments);
        if(environments.length > 0)
        {
            _environment = std.array.back(environments);
        }
        else
        {
            std.stdio.writefln("stack underflow", _environment);
            _environment = null;
        }
    }

    void enterInline(string context, ast.Node node)
    {
        auto match = inlined.get(node, false);
        if(match)
        {
            error("recursive cycle detected in " ~ context ~ ":", node.location, false, true);
            foreach(i, item; inlines)
            {
                log(std.string.format("%s - stack entry #%d", item.location, i));
            }
            error("infinite recursion is unrecoverable", node.location, true);
            abort();
        }

        inlined[node] = true;
        inlines ~= node;
    }

    void leaveInline()
    {
        inlined[std.array.back(inlines)] = false;
        std.array.popBack(inlines);
    }

    ubyte[] save()
    {
        ubyte[] buffer;
        foreach(bank; banks)
        {
            bank.dump(buffer);
        }
        platform.patch(buffer);
        return buffer;
    }

    string[string] exportNameLists()
    {
        string[string] namelists;
        foreach(bank; banks)
        {
            bank.exportNameLists(namelists);
        }
        string namelist = namelists.get("ram", "");
        foreach(address, name; addressNames)
        {
            if(address < 0x8000)
            {
                auto size = addressSizes.get(address, 0);
                if(size > 1)
                {
                    namelist ~= std.string.format("$%04X/%X#%s#\n", address, size, name);
                }
                else
                {
                    namelist ~= std.string.format("$%04X#%s#\n", address, name);
                }
            }
        }
        namelists["ram"] = namelist;
        return namelists;
    }
}