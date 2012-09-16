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
    bool finalized;

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
}