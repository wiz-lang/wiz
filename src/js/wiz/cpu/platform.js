(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.cpu = typeof(wiz.cpu) !== 'undefined' ? wiz.cpu : {};

    wiz.cpu.AbstractPlatform = function() {
        var that = {};
        that.builtins = function() { throw new Error('not implemented'); }
        that.generatePush = function(program, stmt) { throw new Error('not implemented'); }
        that.generateJump = function(program, stmt) { throw new Error('not implemented'); }
        that.generateComparison = function(program, stmt) { throw new Error('not implemented'); }
        that.generateAssignment = function(program, stmt) { throw new Error('not implemented'); }
        that.patch = function(buffer) { throw new Error('not implemented'); }
        return that;
    }
})(this);