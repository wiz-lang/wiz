(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.Location = function(file, line) {
        var that = {};
        that.file = file;
        that.line = line ? line : 0;

        that.copy = function() {
            return wiz.compile.Location(that.file, that.line);
        };
        
        that.toString = function() {
            return file + "(" + line + ")";
        };
        return that;
    };
})(this);