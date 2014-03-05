(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.ast = typeof(wiz.ast) !== 'undefined' ? wiz.ast : {};

    wiz.ast.Assignment = function(dest, intermediary, src, postfixType, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Assignment';
        that.nodeChildren = ['dest', 'intermediary', 'src'];
        that.dest = dest;
        that.intermediary = intermediary;
        that.src = src;
        that.postfix = postfixType;
        return that;
    }

    wiz.ast.BankDecl = function(names, type, size, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'BankDecl';
        that.nodeChildren = ['size'];
        for(var i = 0, len = names.length; i < len; i++) {
            names[i] = "bank " + names[i];
        }
        that.names = names;
        that.type = type;
        that.size = size;
        return that;
    }

    wiz.ast.Block = function(name, statements, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Block';
        that.nodeChildren = ['statements'];
        that.name = name;
        that.statements = statements;
        return that;
    }

    wiz.ast.BuiltinDecl = function(name) {
        var that = wiz.ast.Node(wiz.compile.Location("<builtin>"));
        that.nodeType = 'BuiltinDecl';
        that.name = name;
        return that;
    }

    wiz.ast.Comparison = function(left, right, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Comparison';
        that.nodeChildren = ['left', 'right'];
        that.left = left;
        that.right = right;
        return that;
    }

    wiz.ast.Conditional = function(trigger, far, prelude, action, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Conditional';
        that.nodeChildren = ['action', 'alternative', 'block'];
        that.trigger = trigger;
        that.far = far;
        that.prelude = prelude;
        that.action = action;
        that.alternative = null;

        that.block = null;
        that.expand = function() {
            var location = that.location;
            var trigger = that.trigger;
            var far = that.far;
            var prelude = that.prelude;
            var action = that.action;
            var alternative = that.alternative;

            if(!alternative) {
                that.block = wiz.ast.Block(null, [
                    // prelude
                    prelude,
                    // goto $end when ~trigger
                    wiz.ast.Jump(wiz.parse.Keyword.Goto, far,
                        wiz.ast.Attribute(["$end"], location),
                        wiz.ast.JumpCondition(true, trigger, location), location
                    ),
                    // action
                    action,
                    // def $end:
                    wiz.ast.LabelDecl("$end", location)
                ], location);
            } else {
                that.block = wiz.ast.Block(null, [
                    // prelude
                    prelude,
                    // goto $else when ~trigger
                    wiz.ast.Jump(wiz.parse.Keyword.Goto, far,
                        wiz.ast.Attribute(["$else"], location),
                        wiz.ast.JumpCondition(true, trigger, location), location
                    ),
                    // action
                    action,
                    // goto $end
                    wiz.ast.Jump(wiz.parse.Keyword.Goto, far,
                        wiz.ast.Attribute(["$end"], location),
                        null, location
                    ),
                    // def $else:
                    wiz.ast.LabelDecl("$else", location),
                    //   alternative
                    alternative,
                    // def $end:
                    wiz.ast.LabelDecl("$end", location)
                ], location);
            }

            // Now that we've exanded, remove original statements
            that.action = null;
            that.alternative = null;
        }

        return that;
    }

    wiz.ast.Data = function(storage, items, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Data';
        that.nodeChildren = ['storage', 'items'];
        that.storage = storage;
        that.items = items;
        return that;
    }

    wiz.ast.Embed = function(filename, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Embed';
        that.filename = filename;
        that.data = [];
        return that;
    }

    wiz.ast.FuncDecl = function(type, name, block, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'FuncDecl';
        that.nodeChildren = ['block', 'statements'];        
        that.inlined = false;
        that.type = type;
        that.name = name;
        that.inner = block;
        that.block = block;
        that.statements = [];

        that.expand = function() {
            if(!that.inlined) {
                // def name:
                that.statements.push(wiz.ast.LabelDecl(that.name, that.location));
                //     block
                that.statements.push(that.block);
                //     return
                switch(type) {
                    case wiz.parse.Keyword.Func:
                        that.statements.push(
                            wiz.ast.Jump(wiz.parse.Keyword.Return, false, null, null, that.location)
                        );
                        break;
                    case wiz.parse.Keyword.Task:
                        that.statements.push(
                            wiz.ast.Jump(wiz.parse.Keyword.Resume, false, null, null, that.location)
                        );
                        break;
                    default:
                        throw new Error('unhandled function type');
                }
            }
            that.block = null;
        }

        return that;
    }

    wiz.ast.Jump = function(type, far, destination, condition, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Jump';
        that.nodeChildren = ['destination', 'condition', 'inlining'];
        that.type = type;
        that.far = far;
        that.destination = destination;
        that.condition = condition;
        that.inlining = null;

        that.expand = function(block) {
            switch(type) {
                case wiz.parse.Keyword.Continue:
                    that.type = wiz.parse.Keyword.Goto;
                    that.destination = wiz.ast.Attribute(["$loop"], that.location);
                    break;
                case wiz.parse.Keyword.While:
                    that.type = wiz.parse.Keyword.Goto;
                    that.destination = wiz.ast.Attribute(["$end"], that.location);
                    that.condition = wiz.ast.JumpCondition(true, that.condition, that.condition.location);
                    break;
                case wiz.parse.Keyword.Until:
                case wiz.parse.Keyword.Break:
                    that.type = wiz.parse.Keyword.Goto;
                    that.destination = wiz.ast.Attribute(["$end"], that.location);
                    break;
                case wiz.parse.Keyword.Inline:
                    that.inlining = block;
                    break;
                default:
                    throw new Error('unhandled jump type');
            }
        }

        return that;
    }

    wiz.ast.JumpCondition = function(negated, condition, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'JumpCondition';
        that.nodeChildren = ['attr'];
        that.attr = null;
        that.branch = null;
        if(wiz.ast.check('JumpCondition', condition)) {
            that.negated = negated ? !condition._negated : condition.negated;
            that.attr = condition.attr;
            that.branch = condition.branch;
        } else if(wiz.ast.check('Attribute', condition)) {
            that.negated = negated;
            that.attr = condition;
        } else if(condition && wiz.parse.Branch[condition]) {
            that.negated = negated;
            that.branch = condition;
        } else {
            throw new Error("condition must be wiz.ast.JumpCondition, wiz.ast.Attribute, or a wiz.parse.Branch");
        }
        return that;
    }

    wiz.ast.LabelDecl = function(name, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'LabelDecl';
        that.name = name;
        return that;
    }

    wiz.ast.LetDecl = function(name, value, offset, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'LetDecl';
        that.nodeChildren = ['value'];
        that.name = name;
        that.value = value;
        that.offset = offset;
        return that;
    }

    wiz.ast.Loop = function(block, far, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Loop';
        that.nodeChildren = ['block'];
        that.block = block;
        that.far = far;

        that.expand = function() {
            var block = that.block;
            var far = that.far;
            var tailConditional = false;
            var location = that.location;
            if(block.statements.length > 0) {
                // 'while' or 'until' as last statement of loop?
                var tail = wiz.ast.check('Jump', block.statements[block.statements.length - 1]);
                if(tail) {
                    switch(tail.type) {
                        case wiz.parse.Keyword.While:
                            // Tail 'while cond' -> 'continue when cond'.
                            tailConditional = true;
                            var jump = wiz.ast.Jump(wiz.parse.Keyword.Continue, far || tail.far, null,
                                tail.condition, tail.location
                            );
                            block.statements[block.statements.length - 1] = jump;
                            break;
                        case wiz.parse.Keyword.Until:
                            // Tail 'until cond' -> 'continue when ~cond'.
                            tailConditional = true;
                            var jump = wiz.ast.Jump(wiz.parse.Keyword.Continue, far || tail.far, null,
                                wiz.ast.JumpCondition(true, tail.condition, tail.location), tail.location
                            );
                            block.statements[block.statements.length - 1] = jump;
                            break;
                        default:
                    }
                }
            }

            // def $loop:
            //   block
            //   goto $loop // (this line is removed if there is a while/until as last statement.
            // def $end:
            var code = [];
            code.push(wiz.ast.LabelDecl("$loop", location));
            code.push(block);
            if(!tailConditional) {
                // Remove unconditional jump,
                code.push(wiz.ast.Jump(wiz.parse.Keyword.Goto, far,
                    wiz.ast.Attribute(["$loop"], location),
                    null, location
                ));
            }
            code.push(wiz.ast.LabelDecl("$end", location));
            that.block = wiz.ast.Block(null, code, location);
        }
        return that;
    }

    wiz.ast.Push = function(src, intermediary, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Push';
        that.nodeChildren = ['src', 'intermediary'];
        that.src = src;
        that.intermediary = intermediary;
        return that;
    }

    wiz.ast.Relocation = function(name, dest, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Relocation';
        that.nodeChildren = ['dest'];
        that.name = name;
        that.mangledName = 'bank ' + name;
        that.dest = dest;
        return that;
    }

    wiz.ast.Storage = function(type, size, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Storage';
        that.nodeChildren = ['size'];
        that.type = type;
        that.size = size;
        return that;
    }

    wiz.ast.Unroll = function(repetitions, block, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'Unroll';
        that.nodeChildren = ['block'];
        that.repetitions = repetitions;
        that.block = block;

        that.expand = function(times) {
            var code = [];
            for(var i = 0; i < times; i++) {
                code.push(wiz.ast.Block(null, that.block.statements, that.location));
            }
            that.block = wiz.ast.Block(null, code, that.location);
        }
        
        return that;
    }

    wiz.ast.VarDecl = function(names, storage, location) {
        var that = wiz.ast.Node(location);
        that.nodeType = 'VarDecl';
        that.nodeChildren = ['storage'];
        that.names = names;
        that.storage = storage;
        return that;
    }
})(this);