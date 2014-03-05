(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    function NodeScope(environment) {
        var that = {};
        that.environments = [environment];
        that.index = that.environments.length;

        that.rewind = function() {
            that.index = 0;
        }

        that.add = function(environment) {
            that.environments.push(environment);
            that.index = that.environments.length;
        }

        that.next = function() {
            if(that.index < that.environments.length) {
                return that.environments[that.index++];
            } else {
                return null;
            }
        }
        return that;
    }

    wiz.compile.Program = function(platform) {
        var that = {};
        that.bank = null;
        that.banks = [];

        that.scopes = {}; // ast.Node -> NodeScope;
        that.environment = null;
        that.environments = [];
        that.inlines = [];
        that.inlined = {} // ast.Node -> boolean
        that.finalized = false;
        that.platform = platform;

        that.rewind = function() {
            for(var k in that.scopes) {
                that.scopes[k].rewind();
            }
            for(var i = 0; i < that.banks.length; i++) {
                that.banks[i].rewind();
            }
            that.bank = null;
        }

        that.addBank = function(b) {
            that.banks.push(b);
        }

        that.checkBank = function(description, location) {
            if(!that.bank) {
                wiz.compile.error(description + " is not allowed before an 'in' statement.", location, true);
                return null;
            } else {
                return that.bank;
            }
        }

        that.switchBank = function(b) {
            that.bank = b;
        }

        that.clearEnvironment = function() {
            var builtins = platform.builtins();
            var env = wiz.compile.Environment();
            var pkg = wiz.sym.PackageDef(wiz.ast.BuiltinDecl(), wiz.compile.Environment(env));
            
            for(var name in builtins) {
                env.put(name, builtins[name]);
            }
            env.put("builtin", pkg);

            env = pkg.environment;
            for(var name in builtins) {
                env.put(name, builtins[name]);
            }

            that.environment = env;
            that.scopes = {};
            that.environments = [env];
        }

        that.createNodeEnvironment = function(node, environment) {
            var match = that.scopes[node.nodeId] || null;
            if(match) {
                match.add(environment);
            } else {
                that.scopes[node.nodeId] = NodeScope(environment);
            }
        }

        that.nextNodeEnvironment = function(node) {
            var match = that.scopes[node.nodeId] || null;
            if(match) {
                return match.next();
            } else {
                return null;
            }
        }

        that.enterEnvironment = function(e) {
            that.environments.push(e);
            that.environment = e;
        }

        that.leaveEnvironment = function() {
            that.environments.pop();
            if(that.environments.length > 0) {
                that.environment = that.environments[that.environments.length - 1];
            } else {
                throw new Error("stack underflow", that.environment);
            }
        }

        that.enterInline = function(context, node) {
            var match = that.inlined[node.nodeId] || false;
            if(match) {
                wiz.compile.error("recursive cycle detected in " + context + ":", node.location, false, true);
                for(var i = 0; i < that.inlines.length; i++) {
                    wiz.compile.log(that.inlines[i].location + " - stack entry #" + i);
                }
                wiz.compile.error("infinite recursion is unrecoverable", node.location, true);
                wiz.compile.abort();
            }

            that.inlined[node.nodeId] = true;
            that.inlines.push(node);
        }

        that.leaveInline = function() {
            that.inlined[that.inlines[that.inlines.length - 1].nodeId] = false;
            that.inlines.pop();
        }

        that.save = function() {
            var buffer = [];
            for(var i = 0; i < that.banks.length; i++) {
                that.banks[i].dump(buffer);
            }
            platform.patch(buffer);
            return new Uint8Array(buffer);
        }

        that.clearEnvironment();
        return that;
    }

})(this);