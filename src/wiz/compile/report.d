module wiz.compile.report;

static import std.stdio;
static import std.string;
static import std.conv;
static import std.c.stdlib;

import wiz.lib;

public uint maximumErrors = 64;
private uint errors;
private bool previousContinued;

class CompileExit : Exception
{
    this()
    {
        super("CompileExit");
    }
}

string severity(bool fatal, bool previousContinued)
{
    if(fatal)
    {
        return "fatal";
    }
    else if(previousContinued)
    {
        return "note";
    }
    else
    {
        return "error";
    }
}

void error(string message, compile.Location location, bool fatal = false, bool continued = false)
{
    log(std.conv.to!string(location) ~ ": " ~ severity(fatal, previousContinued) ~ ": " ~ message);
    if(!continued)
    {
        errors++;
    }
    previousContinued = continued;
    if(fatal || errors >= maximumErrors)
    {
        abort();
    }
}

void verify()
{
    if(errors > 0)
    {
        abort();
    }
}

void notice(string message)
{
    std.stdio.writefln("* %s: %s", PROGRAM_NAME, message);
}

void log(string message)
{
    std.stdio.writeln(message);
}

void abort()
{
    notice(std.string.format("failed with %d error(s).", errors));
    throw new CompileExit();
}