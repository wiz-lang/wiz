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
        
    this()
    {
        bank = null;
        _environment = new Environment();

        /*
        Location location = Location("<builtin>");
        b.put(new RegisterDefinition("a", RegisterType.A, location));
        b.put(new RegisterDefinition("x", RegisterType.X, location));
        b.put(new RegisterDefinition("y", RegisterType.Y, location));
        b.put(new RegisterDefinition("s", RegisterType.S, location));
        b.put(new RegisterDefinition("p", RegisterType.P, location));

        PackageDefinition pkg = new PackageDefinition("builtin", new Environment(b), location);
        b.put(pkg);

        auto env = pkg.getTable();
        env.put(new RegisterDefinition("a", RegisterType.A, location));
        env.put(new RegisterDefinition("x", RegisterType.X, location));
        env.put(new RegisterDefinition("y", RegisterType.Y, location));
        env.put(new RegisterDefinition("s", RegisterType.S, location));
        env.put(new RegisterDefinition("p", RegisterType.P, location));*/
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
            _environment = null;
        }
    }
}