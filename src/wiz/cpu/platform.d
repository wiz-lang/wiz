module wiz.cpu.platform;

import wiz.lib;

abstract class Platform
{
    abstract sym.Definition[string] builtins();
    abstract ubyte[] generateCommand(compile.Program program, ast.Command stmt);
    abstract ubyte[] generateJump(compile.Program program, ast.Jump stmt);
    abstract ubyte[] generateAssignment(compile.Program program, ast.Assignment stmt);
}