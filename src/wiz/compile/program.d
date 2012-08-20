module wiz.compile.program;

import wiz.lib;
import wiz.compile.lib;

class Program
{
    private Environment _environment;

    private Bank bank;
    private Bank[] banks;
    private Environment[ast.Node] nodeEnvironments;
    private Environment[] environmentStack;

    cpu.Platform platform;
        
    this(cpu.Platform platform)
    {
        this.platform = platform;

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

        bank = null;
        _environment = env;
        environmentStack ~= env;
    }

    mixin helper.Accessor!(_environment);

    void rewind()
    {
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

    void enterEnvironment(ast.Node node, Environment e = null)
    {
        Environment match = nodeEnvironments.get(node, null);
        if(match is null)
        {
            if(e is null)
            {
                e = new Environment(_environment);
            }
            nodeEnvironments[node] = e;
        }
        else
        {
            e = match;
        }

        environmentStack ~= e;
        _environment = e;
    }

    void leaveEnvironment()
    {
        std.array.popBack(environmentStack);
        if(environmentStack.length > 0)
        {
            _environment = std.array.back(environmentStack);
        }
        else
        {
            std.stdio.writefln("stack underflow", _environment);
            _environment = null;
        }
    }
}