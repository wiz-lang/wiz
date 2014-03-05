(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.Environment = function(parent) {
        var that = {};
        that.parent = parent || null;
        that.dictionary = {};

        that.printKeys = function() {
            for(key in that.dictionary) {
                wiz.compile.log(key + ': ' + that.dictionary[key]); 
            }
        };
        
        that.put = function(name, def) {
            var match = that.get(name, true);
            if(match) {
                wiz.compile.error("redefinition of symbol '" + name + "'", def.decl.location, false, true);
                wiz.compile.error("(previously defined here)", match.decl.location);
            } else {
                that.dictionary[name] = def;
            }
        };

        that.get = function(name, shallow) {
            shallow = shallow || false;
            var match = that.dictionary[name] || null;
            if(!match) {
                if(shallow || !parent) {
                    return null;
                } else {
                    return that.parent.get(name, shallow);
                }
            } else {
                while(wiz.sym.check('AliasDef', match)) {
                    match = match.definition;
                }
                return match;
            }
        }

        return that;
    };
})(this);