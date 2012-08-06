module wiz.lib;
public import ast = wiz.ast.lib;
public import cpu = wiz.cpu.lib;
public import sym = wiz.sym.lib;
public import parse = wiz.parse.lib;
public import compile = wiz.compile.lib;
public import helper = wiz.helper;

alias compile.log log;
alias compile.error error;
alias compile.notice notice;
alias helper.staticInit staticInit;

enum PROGRAM_NAME = "wiz";
enum VERSION_TEXT = "0.1";
