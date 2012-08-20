module wiz.cpu.platform;

import wiz.lib;

abstract class Platform
{
    alias sym.Definition[string] BuiltinTable;
    
    abstract BuiltinTable builtins();
    abstract ubyte[] generateCommand(compile.Program program, ast.Command stmt);
    abstract ubyte[] generateJump(compile.Program program, ast.Jump stmt);
    abstract ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt);
    abstract ubyte[] generateComparison(compile.Program program, ast.Comparison stmt);
}