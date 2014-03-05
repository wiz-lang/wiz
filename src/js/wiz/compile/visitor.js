(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.Visitor = function(callbacks) {
        var that = {};
        that.pre = {}
        that.post = {}
        for(var nodeType in callbacks) {
            var callback = callbacks[nodeType];

            if(typeof(callback) === 'function') {
                that.post[nodeType] = callback;
            } else {
                that.pre[nodeType] = callback.pre;
                that.post[nodeType] = callback.post;
            }
        }

        function visit(table, o) {
            if(typeof(o.nodeType) === 'undefined') {
                throw new Error('node missing node type');
            }

            var f = table[o.nodeType];
            if(!f) {
                return true;
            }
            return f(o);
        }

        that.preVisit = function(o) {
            return visit(that.pre, o);
        }

        that.postVisit = function(o) {
            return visit(that.post, o);
        }

        return that;
    };

    wiz.compile.traverse = function(node, callbacks) {
        wiz.compile.accept(node, wiz.compile.Visitor(callbacks));
    }

    wiz.compile.accept = function(node, visitor) {
        if(visitor.preVisit(node) !== false) {
            var children = node.nodeChildren;
            if(children) {
                for(var c = 0, clen = children.length; c < clen; c++) {
                    var child = node[children[c]];
                    if(child) {
                        if(typeof(child.length) !== 'undefined') {
                            for(var i = 0, ilen = child.length; i < ilen; i++) {
                                if(child[i]) {
                                    wiz.compile.accept(child[i], visitor);
                                }
                            }
                        } else {
                            wiz.compile.accept(child, visitor);
                        }
                    }
                }
            }
        }
        visitor.postVisit(node);
    }


})(this);