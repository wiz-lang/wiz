module wiz.compile.visitor;

static import std.stdio;
static import std.traits;

class Visitor
{
    enum Pass
    {
        BEFORE,
        AFTER
    }

    bool preVisit(Object o)
    {
        return visit(pre, o);
    }

    bool postVisit(Object o)
    {
        return visit(post, o);
    }

    Visitor bind(C)(C callback)
    {
        static assert(std.traits.isCallable!C, "type '" ~ C.stringof ~ "' is not callable");

        alias std.traits.ReturnType!C Ret;
        alias std.traits.ParameterTypeTuple!C Par;

        enum ValidUnaryFunc = Par.length == 1
            && is(Ret == void);
        enum ValidBinaryFunc = Par.length == 2
            && is(Par[0] == Pass)
            && (is(Ret == void) || is(Ret == bool));
        static assert(ValidUnaryFunc || ValidBinaryFunc,
            "signature of callable '" ~ Ret.stringof ~ Par.stringof
            ~ "' is neither 'void(T)', 'bool(T)', "
            ~ "'void(Visitor.Pass, T)', nor 'bool(Visitor.Pass, T)'"
        );

        static if(Par.length == 1)
        {
            static if(is(Ret == void))
            {
                post[Par[0].classinfo] = (Object o) { callback(cast(Par[0]) o); return true; };
                return this;
            }
            else static if(is(Ret == bool))
            {
                post[Par[0].classinfo] = (Object o) { return callback(cast(Par[0]) o); };
                return this;
            }
        }
        else static if(Par.length == 2)
        {
            static if(is(Ret == void))
            {
                pre[Par[1].classinfo] = (Object o) { callback(Pass.BEFORE, cast(Par[1]) o); return true; };
                post[Par[1].classinfo] = (Object o) { callback(Pass.AFTER, cast(Par[1]) o); return true; };
                return this;
            }
            else static if(is(Ret == bool))
            {
                pre[Par[1].classinfo] = (Object o) { return callback(Pass.BEFORE, cast(Par[1]) o); };
                post[Par[1].classinfo] = (Object o) { return callback(Pass.AFTER, cast(Par[1]) o); };
                return this;
            }
        }
    }

    Visitor bind(C, R...)(C callback, R r)
    {
        bind(callback);
        static if(r.length > 0)
        {
            bind(r);
        }
        return this;
    }

    private bool visit(bool delegate(Object)[TypeInfo_Class] table, Object o)
    {
        auto t = o.classinfo;
        auto f = (t in table);
        while(f is null && t.base !is null)
        {
            t = t.base;
            f = (t in table);
        }
        if(f is null)
        {
            return true;
        }
        return (*f)(o);
    }

    private bool delegate(Object)[TypeInfo_Class] pre;
    private bool delegate(Object)[TypeInfo_Class] post;
}

auto visitor(R...)(R r)
{
    return (new Visitor()).bind(r);
}

void traverse(T, R...)(T node, R r)
{
    node.accept(visitor(r));
}

mixin template AbstractAcceptor()
{
    abstract void accept(wiz.compile.visitor.Visitor v);
}

mixin template LeafAcceptor()
{
    void accept(wiz.compile.visitor.Visitor v)
    {
        v.preVisit(this);
        v.postVisit(this);
    }
}

private void branchAccept(T, alias Attr, Rest...)(T self, Visitor v)
{
    auto attr = self.getVisitorMemberByName!(__traits(identifier, Attr));

    static if(std.traits.isIterable!(typeof(attr)))
    {
        foreach(item; attr)
        {
            if(item !is null)
            {
                item.accept(v);
            }
        }
    }
    else
    {
        if(attr !is null)
        {
            attr.accept(v);
        }
    }
    static if(Rest.length > 0)
    {
        branchAccept!(T, Rest)(self, v);
    }
}

mixin template BranchAcceptor(alias Attr, Rest...)
{
    auto getVisitorMemberByName(string field)()
    {
        return __traits(getMember, this, field);
    }

    void accept(wiz.compile.visitor.Visitor v)
    {
        if(v.preVisit(this))
        {
            wiz.compile.visitor.branchAccept!(typeof(this), Attr, Rest)(this, v);
        }
        v.postVisit(this);
    }
}