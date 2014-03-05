(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.sym = typeof(wiz.sym) !== 'undefined' ? wiz.compile : {};

    wiz.sym.check = function(definitionType, def) {
        if(typeof(def) === 'object' && def && def.definitionType == definitionType) {
            return def;
        }
        return null;
    }

    wiz.sym.Definition = function(decl) {
        var that = {};
        that.definitionType = 'Definition';
        that.decl = decl;
        return that;
    };

    wiz.sym.AliasDef = function(decl, definition) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'AliasDef';
        that.definition = definition;
        return that;
    }

    wiz.sym.BankDef = function(decl, bank) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'BankDef';
        that.bank = bank;
        return that;
    }

    wiz.sym.ConstDef = function(decl, environment) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'ConstDef';
        that.environment = environment;
        return that;
    }

    wiz.sym.FuncDef = function(decl) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'FuncDef';
        return that;
    }

    wiz.sym.LabelDef = function(decl) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'LabelDef';
        that.hasAddress = false;
        that.address = 0;
        return that;
    }

    wiz.sym.PackageDef = function(decl, environment) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'PackageDef';
        that.environment = environment;
        return that;
    }

    wiz.sym.VarDef = function(decl) {
        var that = wiz.sym.Definition(decl);
        that.definitionType = 'VarDef';
        that.hasAddress = false;
        that.address = 0;
        return that;
    }
})(this);