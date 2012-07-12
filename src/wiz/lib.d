module wiz.lib;
public import ast = wiz.ast.lib;
public import def = wiz.def.lib;
public import parse = wiz.parse.lib;
public import compile = wiz.compile.lib;

alias compile.log log;
alias compile.error error;
alias compile.notice notice;

immutable string PROGRAM_NAME = "wiz";
immutable string VERSION_TEXT = "0.1";
