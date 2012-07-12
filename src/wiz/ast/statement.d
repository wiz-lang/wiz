module wiz.ast.statement;

import wiz.lib;
import wiz.ast.lib;

enum StatementType
{
    BANK,
    DATA,
    JUMP,
    LOOP,
    BLOCK,
    LABEL,
    EMBED,
    HEADER,
    COMMAND,
    CONSTANT,
    VARIABLE,
    ASSIGNMENT,
    COMPARISON,
    RELOCATION,
    CONDITIONAL,
    ENUMERATION,
};

abstract class Statement : Node
{
    private:
        StatementType statementType;
        
    public:
        this(StatementType statementType, compile.Location location)
        {
            super(location);
            this.statementType = statementType;
        }
        
        StatementType getStatementType()
        {
            return statementType;
        }
        
        abstract void aggregate();
        abstract void validate();
        abstract void generate();
}