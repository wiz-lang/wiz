module wiz.ast.block;

static import std.stdio;
static import std.string;

import wiz.lib;
import wiz.ast.lib;

class Block : Statement
{
    private string _name;
    private Statement[] _statements;
    //private compile.Environment environment;

    this(Statement[] statements, compile.Location location)
    {
        super(location);
        _statements = statements;
    }
    
    this(string name, Statement[] statements, compile.Location location)
    {
        this(statements, location);
        _name = name;
    }

    mixin compile.BranchAcceptor!(_statements);
    mixin helper.Accessor!(_name, _statements);

    /*
    void aggregate()
    {
        compile.Environment parent = compile.environment;
        
        // Package?
        if(name.length > 0 && parent !is null)
        {
            // If there was already a package defined by that name declared
            // in this scope (and not a parent), then reuse that table.
            auto b = parent.tryGet!Block(name, false);

            // No previous table existed. Update scope.
            if(b is null)
            {
                environment = new compile.Environment(parent);
                // Add this table to the parent scope.
                parent.put(name, this);                    
            }
            // Reuse existing table.
            else
            {
                environment = b.environment;
            }
        }
        else
        {
            environment = new compile.Environment(parent);
        }

        compile.enterEnv(environment);
        foreach(statement; statements)
        {
            //statement.aggregate();
        }
        compile.exitEnv();
    }*/
}
