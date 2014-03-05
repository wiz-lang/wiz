(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.parse = typeof(wiz.parse) !== 'undefined' ? wiz.parse : {};

    var st = 0;
    var State = {
        Start: 'st' + (st++),
        String: 'st' + (st++),
        StringEscape: 'st' + (st++),
        LeadingZero: 'st' + (st++),
        IntDigits: 'st' + (st++),
        HexDigits: 'st' + (st++),
        BinDigits: 'st' + (st++),
        Identifier: 'st' + (st++),
        Slash: 'st' + (st++),
        SlashSlashComment: 'st' + (st++),
        SlashStarComment: 'st' + (st++),
        SlashStarCommentStar: 'st' + (st++),
        Tilde: 'st' + (st++),
        Plus: 'st' + (st++),
        Minus: 'st' + (st++),
        Equals: 'st' + (st++),
        Less: 'st' + (st++),
        LessLess: 'st' + (st++),
        LessLessLess: 'st' + (st++),
        Greater: 'st' + (st++),
        GreaterGreater: 'st' + (st++),
        GreaterGreaterGreater: 'st' + (st++),
    };

    wiz.parse.Scanner = function(file, filename) {
        var Token = wiz.parse.Token;

        var that = {};
        that.terminator = '';
        that.position = 0;
        that.commentLine = 0;
        that.buffer = '';
        that.text = '';
        that.lastText = '';
        that.state = State.Start;
        that.source = file.readLines();
        that.sourceIndex = 0;
        that.location = wiz.compile.Location(filename);
        
        that.consumeLastText = function() {
            var lastText = that.lastText;
            that.lastText = "";
            return lastText;
        };
        
        that.getLocation = function() {
            return that.location.copy();
        };
     
        function flushText() {
            that.state = State.Start;
            that.lastText = that.text;
            that.text = "";
        }

        function error(message) {
            wiz.compile.error(message, that.location);
        }

        that.next = function() {
            while(true) {
                while(that.position < that.buffer.length) {
                    var c = that.buffer.charAt(that.position);
                    switch(that.state) {
                        case State.Start:
                            switch(c) {
                                case '0':
                                    that.state = State.LeadingZero;
                                    that.text += c;
                                    break;
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                    that.state = State.IntDigits;
                                    that.text += c;
                                    break;
                                case '_':
                                case 'a':
                                case 'b':
                                case 'c':
                                case 'd':
                                case 'e':
                                case 'f':
                                case 'g':
                                case 'h':
                                case 'i':
                                case 'j':
                                case 'k':
                                case 'l':
                                case 'm':
                                case 'n':
                                case 'o':
                                case 'p':
                                case 'q':
                                case 'r':
                                case 's':
                                case 't':
                                case 'u':
                                case 'v':
                                case 'w':
                                case 'x':
                                case 'y':
                                case 'z':
                                case 'A':
                                case 'B':
                                case 'C':
                                case 'D':
                                case 'E':
                                case 'F':
                                case 'G':
                                case 'H':
                                case 'I':
                                case 'J':
                                case 'K':
                                case 'L':
                                case 'M':
                                case 'N':
                                case 'O':
                                case 'P':
                                case 'Q':
                                case 'R':
                                case 'S':
                                case 'T':
                                case 'U':
                                case 'V':
                                case 'W':
                                case 'X':
                                case 'Y':
                                case 'Z':
                                    that.state = State.Identifier;
                                    that.text += c;
                                    break;
                                case '\'': case '\"':
                                    that.terminator = c;
                                    that.state = State.String;
                                    break;
                                case ' ': case '\t': case '\r': case '\n': break;
                                case ':': that.position++; return Token.Colon;
                                case ',': that.position++; return Token.Comma;
                                case '.': that.position++; return Token.Dot;
                                case '(': that.position++; return Token.LParen;
                                case ')': that.position++; return Token.RParen;
                                case '[': that.position++; return Token.LBracket;
                                case ']': that.position++; return Token.RBracket;
                                case '{': that.position++; return Token.LBrace;
                                case '}': that.position++; return Token.RBrace;
                                case ';': that.position++; return Token.Semi;
                                case '#': that.position++; return Token.Hash;
                                case '|': that.position++; return Token.Or;
                                case '*': that.position++; return Token.Mul;
                                case '%': that.position++; return Token.Mod;
                                case '@': that.position++; return Token.At;
                                case '&': that.position++; return Token.And;
                                case '^': that.position++; return Token.Xor;
                                case '!': that.position++; return Token.Exclaim;
                                case '~': that.state = State.Tilde; break;
                                case '+': that.state = State.Plus; break;
                                case '-': that.state = State.Minus; break;
                                case '/': that.state = State.Slash; break;
                                case '=': that.state = State.Equals; break;
                                case '<': that.state = State.Less; break;
                                case '>': that.state = State.Greater; break;
                                default: error("unrecognized character " + c + " found.");
                            }
                            break;
                        case State.Identifier:
                            switch(c) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                case 'a':
                                case 'b':
                                case 'c':
                                case 'd':
                                case 'e':
                                case 'f':
                                case 'g':
                                case 'h':
                                case 'i':
                                case 'j':
                                case 'k':
                                case 'l':
                                case 'm':
                                case 'n':
                                case 'o':
                                case 'p':
                                case 'q':
                                case 'r':
                                case 's':
                                case 't':
                                case 'u':
                                case 'v':
                                case 'w':
                                case 'x':
                                case 'y':
                                case 'z':
                                case 'A':
                                case 'B':
                                case 'C':
                                case 'D':
                                case 'E':
                                case 'F':
                                case 'G':
                                case 'H':
                                case 'I':
                                case 'J':
                                case 'K':
                                case 'L':
                                case 'M':
                                case 'N':
                                case 'O':
                                case 'P':
                                case 'Q':
                                case 'R':
                                case 'S':
                                case 'T':
                                case 'U':
                                case 'V':
                                case 'W':
                                case 'X':
                                case 'Y':
                                case 'Z':
                                case '_':
                                    that.text += c;
                                    break;
                                default:
                                    flushText();
                                    return Token.Identifier;
                            }
                            break;
                        case State.String:
                            if(c == that.terminator) {
                                that.position++;
                                flushText();
                                return Token.String;
                            } else switch(c) {
                                case '\\':
                                    that.state = State.StringEscape;
                                    break;
                                default:
                                    that.text += c;
                                    break;
                            }
                            break;
                        case State.StringEscape:
                            that.state = State.String;
                            switch(c) {
                                case '\"': case '\'': case '\\':
                                    that.text += c;
                                    break;
                                case 't': that.text += '\t'; break;
                                case 'r': that.text += '\r'; break;
                                case 'n': that.text += '\n'; break;
                                case 'f': that.text += '\f'; break;
                                case 'b': that.text += '\b'; break;
                                case 'a': that.text += String.fromCharCode(0x07); break;
                                case '0': that.text += '\0'; break;
                                default:
                                    error("invalid escape sequence \\" + c + " in string literal");
                                    break;
                            }
                            break;
                        case State.LeadingZero:
                            switch(c) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                    that.state = State.IntDigits;
                                    that.text += c;
                                    break;
                                case 'x': that.state = State.HexDigits; that.text += c; break;
                                case 'b': that.state = State.BinDigits; that.text += c; break;
                                default:
                                    flushText();
                                    return Token.Integer;
                            }
                            break;
                        case State.IntDigits:
                            switch(c) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                    that.text += c;
                                    break;
                                default:
                                    flushText();
                                    return Token.Integer;
                            }
                            break;
                        case State.HexDigits:
                            switch(c) {
                                case '0':
                                case '1':
                                case '2':
                                case '3':
                                case '4':
                                case '5':
                                case '6':
                                case '7':
                                case '8':
                                case '9':
                                case 'a':
                                case 'b':
                                case 'c':
                                case 'd':
                                case 'e':
                                case 'f':
                                case 'A':
                                case 'B':
                                case 'C':
                                case 'D':
                                case 'E':
                                case 'F':
                                    that.text += c;
                                    break;
                                default:
                                    flushText();
                                    return Token.Hexadecimal;
                            }
                            break;
                        case State.BinDigits:
                            switch(c) {
                                case '0': case '1':
                                    that.text += c;
                                    break;
                                default:
                                    flushText();
                                    return Token.Binary;
                            }
                            break;
                        case State.Tilde:
                            that.state = State.Start;
                            switch(c) {
                                case '=': that.position++; return Token.NotEqual;
                                default: return Token.Not;
                            }
                            break;
                        case State.Plus:
                            that.state = State.Start;
                            switch(c) {
                                case '#': that.position++; return Token.AddC;
                                case '+': that.position++; return Token.Inc;
                                default: return Token.Add;
                            }
                            break;
                        case State.Minus:
                            that.state = State.Start;
                            switch(c) {
                                case '#': that.position++; return Token.SubC;
                                case '-': that.position++; return Token.Dec;
                                default: return Token.Sub;
                            }
                            break;
                        case State.Slash:
                            that.state = State.Start;
                            switch(c) {
                                case '/':
                                    that.state = State.SlashSlashComment;
                                    break;
                                case '*':
                                    that.state = State.SlashStarComment;
                                    that.commentLine = that.location.line;
                                    break;
                                default:
                                    return Token.Div;
                            }
                            break;
                        case State.SlashSlashComment: break;
                        case State.SlashStarComment:
                            switch(c) {
                                case '*': that.state = State.SlashStarCommentStar; break;
                                default: break;
                            }
                            break;
                        case State.SlashStarCommentStar:
                            switch(c) {
                                case '/': that.state = State.Start; break;
                                default: that.state = State.SlashStarComment; break;
                            }
                            break;
                        case State.Equals:
                            that.state = State.Start;
                            switch(c) {
                                case '=': that.position++; return Token.Equal;
                                default: return Token.Set;
                            }
                            break;
                        case State.Less:
                            that.state = State.Start;
                            switch(c) {
                                case '<': that.state = State.LessLess; break;
                                case '>': that.position++; return Token.Swap;
                                case '=': that.position++; return Token.LessEqual;
                                default: return Token.Less;
                            }
                            break;
                        case State.LessLess:
                            that.state = State.Start;
                            switch(c) {
                                case '<': that.state = State.LessLessLess; break;
                                case '-': that.position++; return Token.ArithShiftL;
                                default: return Token.ShiftL;
                            }
                            break;
                        case State.LessLessLess:
                            that.state = State.Start;
                            switch(c) {
                                case '#': that.position++; return Token.RotateLC;
                                default: return Token.RotateL;
                            }
                            break;
                        case State.Greater:
                            that.state = State.Start;
                            switch(c) {
                                case '>': that.state = State.GreaterGreater; break;
                                case '<': that.position++; return Token.Swap;
                                case '=': that.position++; return Token.GreaterEqual;
                                default: return Token.Greater;
                            }
                            break;
                        case State.GreaterGreater:
                            that.state = State.Start;
                            switch(c) {
                                case '>': that.state = State.GreaterGreaterGreater; break;
                                case '-': that.position++; return Token.ArithShiftR;
                                default: return Token.ShiftR;
                            }
                            break;
                        case State.GreaterGreaterGreater:
                            that.state = State.Start;
                            switch(c) {
                                case '#': that.position++; return Token.RotateRC;
                                default: return Token.RotateR;
                            }
                            break;
                        default:
                            throw new Error('unhandled that.state ' + that.state);
                    }
                    that.position++;
                }
                
                if(that.sourceIndex < that.source.length) {
                    that.buffer = that.source[that.sourceIndex++];

                    // Special handling in that.states for end-of-line.
                    switch(that.state)
                    {
                        case State.SlashSlashComment:
                            that.state = State.Start;
                            break;
                        case State.String:
                            that.state = State.Start;
                            error("expected closing quote " + that.terminator + ", but got end-of-line");
                            break;
                        case State.StringEscape:
                            that.state = State.Start;
                            error("expected string escape sequence, but got end-of-line");
                            break;
                        default:
                            break;
                    }
                    that.position = 0;
                    that.location.line++;
                } else {
                    // End-of-file.
                    switch(that.state) {
                        case State.Identifier:
                            flushText();
                            return Token.Identifier;
                        case State.LeadingZero:
                        case State.IntDigits:
                            flushText();
                            return Token.Integer;
                        case State.HexDigits:
                            flushText();
                            return Token.Hexadecimal;
                        case State.BinDigits:
                            flushText();
                            return Token.Binary;
                        case State.String:
                            that.state = State.Start;
                            error("expected closing quote " + that.terminator + ", but got end-of-file");
                            break;
                        case State.StringEscape:
                            that.state = State.Start;
                            error("expected string escape sequence, but got end-of-file");
                            break;
                        case State.SlashStarComment:
                            error("expected */ to close /* comment starting on line " + that.commentLine + ", but got end-of-file");
                            break;
                        default:
                            break;
                    }
                    return Token.EndOfFile;
                }
            }
            throw new Error();
        };

        return that;
    };    
})(this);