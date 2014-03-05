(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.ast = typeof(wiz.ast) !== 'undefined' ? wiz.ast : {};

    wiz.ast.Expression = {};
    wiz.ast.Expression.Max = 65535;

    wiz.ast.Number = function(numberType, value, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Number';
        that.numberType = numberType;
        that.value = value;
        return that;
    }

    wiz.ast.Attribute = function(pieces, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Attribute';
        that.pieces = pieces;
        that.fullName = pieces.join('.');
        return that;
    }

    wiz.ast.String = function(value, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'String';
        that.value = value;
        return that;
    }

    wiz.ast.Pop = function(location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Pop';
        return that;
    }

    wiz.ast.Infix = function(infixTypes, operands, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Infix';
        that.nodeChildren = ['operands'];
        that.types = infixTypes;
        that.operands = operands;
        return that;
    }

    wiz.ast.Prefix = function(prefixType, operand, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Prefix';
        that.nodeChildren = ['operand'];
        that.type = prefixType;
        that.operand = operand;
        return that;
    }

    wiz.ast.Postfix = function(postfixType, operand, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Postfix';
        that.nodeChildren = ['operand'];
        that.type = postfixType;
        that.operand = operand;
        return that;
    }
})(this);