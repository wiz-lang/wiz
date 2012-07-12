module wiz.compile.report;

static import std.stdio;
static import std.string;
static import std.conv;
static import std.c.stdlib;

import wiz.lib;

public uint maximumErrors = 64;
private uint errors;

void error(string message, compile.Location location, bool fatal = false)
{
    log(std.conv.to!string(location) ~ ": " ~ (fatal ? "fatal" : "error") ~ ": " ~ message);
    errors++;
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
    std.c.stdlib.exit(1);
}