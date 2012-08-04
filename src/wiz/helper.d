module wiz.helper;

import std.stdio;

mixin template Accessor(alias attr, Rest...)
{
    mixin("@property auto " ~ attr.stringof[1 .. $] ~ "() { return " ~ attr.stringof ~ "; }");
    static if(Rest.length > 0)
    {
        mixin wiz.helper.Accessor!(Rest);
    }
}

void staticInit(alias dest)(lazy typeof(dest) init)
{
    static bool initialized;
    if(!initialized)
    {
        dest = init();
        initialized = true;
    }
}