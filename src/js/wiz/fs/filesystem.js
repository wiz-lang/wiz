(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.fs = typeof(wiz.fs) !== 'undefined' ? wiz.fs : {};

    wiz.fs.AbstractFile = function() {
        var that = {};
        that.calculateSize = function() { throw new Error('not implemented'); }
        that.readLines = function() { throw new Error('not implemented'); }
        that.readBinary = function() { throw new Error('not implemented'); }
        return that;
    }

    wiz.fs.MemoryFile = function(source) {
        var that = wiz.fs.AbstractFile();
        that.source = source;

        that.calculateSize = function() {
            return that.source.length;
        }

        that.readLines = function() {
            var lines = that.source.split(/\r?\n/);
            for(var i = 0; i < lines.length; i++) {
                lines[i] = lines[i] + '\n';
            }
            return lines;
        }
        return that;
    }

    wiz.fs.open = function(filename) {
        throw new Error('not implemented');
    }

    wiz.fs.reset = function() {
        // Free any open file handles and start fresh.
    }
})(this);