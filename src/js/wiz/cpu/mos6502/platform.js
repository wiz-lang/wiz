(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.cpu = typeof(wiz.cpu) !== 'undefined' ? wiz.cpu : {};
    wiz.cpu.mos6502 = typeof(wiz.cpu.mos6502) !== 'undefined' ? wiz.cpu.mos6502 : {};

    wiz.cpu.mos6502.Mos6502Platform = function() {
        var that = wiz.cpu.AbstractPlatform();
        // TODO
        return that;
    }
})(this);