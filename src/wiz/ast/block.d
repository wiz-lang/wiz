module wiz.ast.block;

static import std.stdio;
static import std.string;

import wiz.lib;
import wiz.ast.lib;

class Block : Statement
{
    private:
        string name;
        Statement[] statements;
        
        compile.Environment environment;
    public:
        this(Statement[] statements, compile.Location location)
        {
            super(StatementType.BLOCK, location);
            this.statements = statements;
        }
        
        this(string name, Statement[] statements, compile.Location location)
        {
            this(statements, location);
            this.name = name;
        }
        
        void aggregate()
        {
            compile.Environment parent = compile.getActiveEnv();
            
            // Package?
            if(name.length > 0 && parent !is null)
            {
                // If there was already a package defined by that name declared
                // in this scope (and not a parent), then reuse that table.
                def.Namespace ns = cast(def.Namespace) parent.tryGet(name, false);                
                // Reuse existing table.
                if(ns !is null)
                {
                    environment = ns.environment;
                }
                // No previous table existed. Update scope.
                // Add this table to the parent scope.
                else
                {
                    environment = new compile.Environment(parent);
                    parent.put(new def.Namespace(name, environment, location));
                }
            }
            else
            {
                environment = new compile.Environment(parent);
            }

            compile.enterEnv(environment);
            foreach(statement; statements)
            {
                statement.aggregate();
            }
            compile.exitEnv();
        }
        
        void validate()
        {
            compile.enterEnv(environment);
            foreach(statement; statements)
            {
                statement.validate();
            }
            compile.exitEnv();
        }
        
        void generate()
        {
            compile.enterEnv(environment);
            foreach(statement; statements)
            {
                statement.generate();
            }
            compile.exitEnv();
        }
}
