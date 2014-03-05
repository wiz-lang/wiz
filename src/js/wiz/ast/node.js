(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.ast = typeof(wiz.ast) !== 'undefined' ? wiz.ast : {};

    wiz.ast.check = function(nodeType, node) {
        if(typeof(node) === 'object' && node && node.nodeType == nodeType) {
            return node;
        }
        return null;
    }

    var nodeId = 0;
    wiz.ast.Node = function(location) {
        var that = {};
        that.nodeId = '$node' + (nodeId++) + '$';
        that.nodeType = 'Node';
        that.nodeChildren = null;
        that.location = location.copy();
        return that;
    }
})(this);