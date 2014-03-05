(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.cpu = typeof(wiz.cpu) !== 'undefined' ? wiz.cpu : {};
    wiz.cpu.gameboy = typeof(wiz.cpu.gameboy) !== 'undefined' ? wiz.cpu.gameboy : {};

    var at = 0;
    wiz.cpu.gameboy.ArgumentType = {
        None: '$at' + (at++),
        Immediate: '$at' + (at++),
        Indirection: '$at' + (at++),
        IndirectionInc: '$at' + (at++),
        IndirectionDec: '$at' + (at++),
        PositiveIndex: '$at' + (at++),
        NegativeIndex: '$at' + (at++),
        BitIndex: '$at' + (at++),
        Not: '$at' + (at++),
        Negated: '$at' + (at++),
        Swap: '$at' + (at++),
        Pop: '$at' + (at++),
        A: '$at' + (at++),
        B: '$at' + (at++),
        C: '$at' + (at++),
        D: '$at' + (at++),
        E: '$at' + (at++),
        F: '$at' + (at++),
        H: '$at' + (at++),
        L: '$at' + (at++),
        AF: '$at' + (at++),
        BC: '$at' + (at++),
        DE: '$at' + (at++),
        HL: '$at' + (at++),
        SP: '$at' + (at++),
        Carry: '$at' + (at++),
        Zero: '$at' + (at++),
        Interrupt: '$at' + (at++),
    }

    var ArgumentType = wiz.cpu.gameboy.ArgumentType;

    function wrap(s, quoted) {
        return quoted ? "'" + s + "'" : s;
    }

    // Argument(type[, immediate : wiz.ast.Expression][, base : Argument])
    wiz.cpu.gameboy.Argument = function(type, immediate, base) {
        var that = {};
        that.type = type;
        if(!base && immediate && typeof(immediate.nodeType) === 'undefined') {
            base = immediate;
            immediate = null;
        }
        that.immediate = immediate || null;
        that.base = base || null;

        that.toString = function(quoted) {
            switch(that.type) {
                case ArgumentType.None: return "???";
                case ArgumentType.Immediate: return "immediate";
                case ArgumentType.Indirection: return wrap("[" + base.toString(false) + "]", quoted);
                case ArgumentType.IndirectionInc: return wrap("[" + base.toString(false) + "++]", quoted);
                case ArgumentType.IndirectionDec: return wrap("[" + base.toString(false) + "--]", quoted);
                case ArgumentType.PositiveIndex: return wrap("[index:" + base.toString(false) + "]", quoted);
                case ArgumentType.NegativeIndex: return wrap("[-index:" + base.toString(false) + "]", quoted);
                case ArgumentType.BitIndex: return wrap(base.toString(false) + "@bit", quoted);
                case ArgumentType.Not: return wrap("~" + base.toString(false), quoted);
                case ArgumentType.Negated: return wrap("-" + base.toString(false), quoted);
                case ArgumentType.Swap: return wrap("<>" + base.toString(false), quoted);
                case ArgumentType.Pop: return wrap("pop", quoted);
                case ArgumentType.A: return wrap("a", quoted);
                case ArgumentType.B: return wrap("b", quoted);
                case ArgumentType.C: return wrap("c", quoted);
                case ArgumentType.D: return wrap("d", quoted);
                case ArgumentType.E: return wrap("e", quoted);
                case ArgumentType.F: return wrap("f", quoted);
                case ArgumentType.H: return wrap("h", quoted);
                case ArgumentType.L: return wrap("l", quoted);
                case ArgumentType.AF: return wrap("af", quoted);
                case ArgumentType.BC: return wrap("bc", quoted);
                case ArgumentType.DE: return wrap("de", quoted);
                case ArgumentType.HL: return wrap("hl", quoted);
                case ArgumentType.SP: return wrap("sp", quoted);
                case ArgumentType.Carry: return wrap("carry", quoted);
                case ArgumentType.Zero: return wrap("zero", quoted);
                case ArgumentType.Interrupt: return wrap("interrupt", quoted);
            }
        }

        that.getRegisterIndex = function() {
            switch(type) {
                case ArgumentType.B: return 0x0; break;
                case ArgumentType.C: return 0x1; break;
                case ArgumentType.D: return 0x2; break;
                case ArgumentType.E: return 0x3; break;
                case ArgumentType.H: return 0x4; break;
                case ArgumentType.L: return 0x5; break;
                case ArgumentType.Indirection:
                    if(base.type == ArgumentType.HL) {
                        return 0x6;
                    }
                    break;
                case ArgumentType.A: return 0x7; break;
                default:
            }
            throw new Error("argument is not register type");
        }

        that.getPairIndex = function() {
            switch(type) {
                case ArgumentType.BC: return 0x0; break;
                case ArgumentType.DE: return 0x1; break;
                case ArgumentType.HL: return 0x2; break;
                case ArgumentType.SP: return 0x3; break;
                case ArgumentType.AF: return 0x3; break;
                default:
            }
            throw new Error("argument is not pair type");
        }


        that.getPairLowIndex = function() {
            switch(type) {
                case ArgumentType.BC: return 0x1; break;
                case ArgumentType.DE: return 0x3; break;
                case ArgumentType.HL: return 0x5; break;
                default:
            }
            throw new Error("argument is not pair type");
        }

        that.getPairHighIndex = function() {
            switch(type) {
                case ArgumentType.BC: return 0x0; break;
                case ArgumentType.DE: return 0x2; break;
                case ArgumentType.HL: return 0x4; break;
                default:
            }
            throw new Error("argument is not pair type");
        }

        that.getFlagIndex = function(negated) {
            switch(type) {
                case ArgumentType.Zero: return negated ? 0x0 : 0x1; break;
                case ArgumentType.Carry: return negated ? 0x2 : 0x3; break;
                default: throw new Error("argument is not flag type");
            }
        }
        return that;
    }

    var Argument = wiz.cpu.gameboy.Argument;
    var ArgumentType = wiz.cpu.gameboy.ArgumentType;

    wiz.cpu.gameboy.Builtin = function(argumentType) {
        var that = wiz.sym.Definition(wiz.ast.BuiltinDecl());
        that.definitionType = 'Builtin';
        that.type = argumentType;
        return that;
    }

    wiz.cpu.gameboy.buildIndirection = function(program, root) {
        if(wiz.ast.check('Attribute', root)) {
            var def = wiz.compile.resolveAttribute(program, root);
            if(!def) {
                return null;
            }
            if(wiz.sym.check('Builtin', def)) {
                return Argument(ArgumentType.Indirection, Argument(def.type));
            }
        } else if(wiz.ast.check('Prefix', root)) {
            if(root.type == wiz.parse.Prefix.Indirection) {
                wiz.compile.error("double-indirection is not supported on the gameboy", root.location);
                return null;
            }
        }  else if(wiz.ast.check('Postfix', root)) {
            var argumentType;
            switch(root.type) {
                case wiz.parse.Postfix.Inc: argumentType = ArgumentType.IndirectionInc; break;
                case wiz.parse.Postfix.Dec: argumentType = ArgumentType.IndirectionDec; break;
            }
            var attr = root.operand;
            if(wiz.ast.check('Attribute', attr)) {
                var def = wiz.compile.resolveAttribute(program, attr);
                if(!def) {
                    return null;
                }
                if(wiz.sym.check('Builtin', def)) {
                    if(def.type == ArgumentType.HL) {
                        return Argument(argumentType, Argument(def.type));
                    }
                }
            }
            wiz.compile.error(
                "operator " + wiz.parse.getSimpleTokenName(root.type)
                + " on indirected operand is not supported (only '[hl"
                + wiz.parse.getSimpleTokenName(root.type) + "]' is valid)",
                root.location
            );
            return null;
        } else if(wiz.ast.check('Infix', root)) {
            var registerLeft;
            var registerRight;

            if(root.types[0] == wiz.parse.Infix.Colon) {
                var attr = root.operands[1];
                if(wiz.ast.check('Attribute', attr)) {
                    var def = wiz.compile.resolveAttribute(program, attr);
                    if(!def) {
                        return null;
                    }
                    if(wiz.sym.check('Builtin', def)) {
                        if(wiz.ast.check('Prefix', root.operands[0])) {
                            if(prefix.type == wiz.parse.Infix.Sub) {
                                return Argument(ArgumentType.NegativeIndex, root.operands[0].operand, Argument(def.type));
                            }
                        }
                        return Argument(ArgumentType.PositiveIndex, root.operands[0], Argument(def.type));
                    }
                }
                wiz.compile.error(
                    "index operator ':' must have register as a right-hand term",
                    root.location
                );
                return null;
            }
        } else if(wiz.ast.check('Pop', root)) {
            wiz.compile.error("'pop' is not allowed inside of indirection", root.location);
            return null;
        }
        return Argument(ArgumentType.Indirection, Argument(ArgumentType.Immediate, root));
    }

    wiz.cpu.gameboy.buildArgument = function(program, root) {
        var v;
        if(wiz.ast.check('Attribute', root)) {
            var def = wiz.compile.resolveAttribute(program, root);
            if(!def) {
                return null;
            }
            if(wiz.sym.check('Builtin', def)) {
                return Argument(def.type);
            }
        } else if(wiz.ast.check('Prefix', root)) {
            if(root.type == wiz.parse.Token.LBracket) {
                return wiz.cpu.gameboy.buildIndirection(program, root.operand);
            }
            if(root.type == wiz.parse.Token.Swap) {
                return Argument(ArgumentType.Swap, wiz.cpu.gameboy.buildArgument(program, root.operand));
            }
            if(root.type == wiz.parse.Token.Not) {
                return Argument(ArgumentType.Not, wiz.cpu.gameboy.buildArgument(program, root.operand));
            }
            if(root.type == wiz.parse.Token.Sub) {
                return Argument(ArgumentType.Negated, wiz.cpu.gameboy.buildArgument(program, root.operand));
            }
        } else if(wiz.ast.check('Infix', root)) {
            if(root.types[0] == wiz.parse.Token.At) {
                var attr = root.operands[0];
                if(wiz.ast.check('Attribute', attr)) {
                    var def = wiz.compile.resolveAttribute(program, attr);
                    if(!def) {
                        return null;
                    }
                    if(wiz.sym.check('Builtin', def)) {
                        return new Argument(ArgumentType.BitIndex, root.operands[1], new Argument(def.type));
                    }
                }
            }
        } else if(wiz.ast.check('Pop', root)) {
            return new Argument(ArgumentType.Pop);
        }
        return new Argument(ArgumentType.Immediate, root);
    }
})(this);