module wiz.parse.scanner;

static import std.stdio;
static import std.string;

import wiz.lib;
import wiz.parse.lib;

private enum State
{
    Start,
    String,
    StringEscape,
    LeadingZero,
    IntDigits,
    HexDigits,
    BinDigits,
    Identifier,
    Slash,
    SlashSlashComment,
    SlashStarComment,
    SlashStarCommentStar,
    Tilde,
    Plus,
    Minus,
    Equals,
    Less,
    LessLess,
    LessLessLess,
    Greater,
    GreaterGreater,
    GreaterGreaterGreater,
}

class Scanner
{
    private char terminator;
    private size_t position;
    private uint commentLine;
    private char[] buffer;
    private string text;
    private string lastText;
    private State state;
    private std.stdio.File file;
    private compile.Location location;
    
    this(std.stdio.File file, string filename)
    {
        this.file = file;
        location = compile.Location(filename);
    }
    
    string getLastText()
    {
        string lastText = this.lastText;
        this.lastText = "";
        return lastText;
    }
        
    compile.Location getLocation()
    {
        return location;
    }
     
    private void flushText()
    {
        state = State.Start;
        lastText = text;
        text = "";
    }

    private void error(string message)
    {
        compile.error(message, location);
    }

    Token next()
    {
        while(true)
        {
            while(position < buffer.length)
            {
                char c = buffer[position];
                switch(state)
                {
                    case State.Start:
                        switch(c)
                        {
                            case '0':
                                state = State.LeadingZero;
                                text ~= c;
                                break;
                            case '1': .. case '9':
                                state = State.IntDigits;
                                text ~= c;
                                break;
                            case '_':
                            case 'a': .. case 'z':
                            case 'A': .. case 'Z':
                                state = State.Identifier;
                                text ~= c;
                                break;
                            case '\'', '\"':
                                terminator = c;
                                state = State.String;
                                break;
                            case ' ', '\t', '\r', '\n': break;
                            case ':': position++; return Token.Colon;
                            case ',': position++; return Token.Comma;
                            case '.': position++; return Token.Dot;
                            case '(': position++; return Token.LParen;
                            case ')': position++; return Token.RParen;
                            case '[': position++; return Token.LBracket;
                            case ']': position++; return Token.RBracket;
                            case '{': position++; return Token.LBrace;
                            case '}': position++; return Token.RBrace;
                            case ';': position++; return Token.Semi;
                            case '#': position++; return Token.Hash;
                            case '|': position++; return Token.Or;
                            case '*': position++; return Token.Mul;
                            case '%': position++; return Token.Mod;
                            case '@': position++; return Token.At;
                            case '&': position++; return Token.And;
                            case '^': position++; return Token.Xor;
                            case '!': position++; return Token.Exclaim;
                            case '~': state = State.Tilde; break;
                            case '+': state = State.Plus; break;
                            case '-': state = State.Minus; break;
                            case '/': state = State.Slash; break;
                            case '=': state = State.Equals; break;
                            case '<': state = State.Less; break;
                            case '>': state = State.Greater; break;
                            default: error(std.string.format("unrecognized character %s found.", c));
                        }
                        break;
                    case State.Identifier:
                        switch(c)
                        {
                            case '0': .. case '9':
                            case 'a': .. case 'z':
                            case 'A': .. case 'Z':
                            case '_':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.Identifier;
                        }
                        break;
                    case State.String:
                        if(c == terminator)
                        {
                            position++;
                            flushText();
                            return Token.String;
                        }
                        else switch(c)
                        {
                            case '\\':
                                state = State.StringEscape;
                                break;
                            default:
                                text ~= c;
                                break;
                        }
                        break;
                    case State.StringEscape:
                        state = State.String;
                        switch(c)
                        {
                            case '\"', '\'', '\\':
                                text ~= c;
                                break;
                            case 't': text ~= '\t'; break;
                            case 'r': text ~= '\r'; break;
                            case 'n': text ~= '\n'; break;
                            case 'f': text ~= '\f'; break;
                            case 'b': text ~= '\b'; break;
                            case 'a': text ~= '\a'; break;
                            case '0': text ~= '\0'; break;
                            default:
                                error(std.string.format("invalid escape sequence \\%s in string literal", c));
                                break;
                        }
                        break;
                    case State.LeadingZero:
                        switch(c)
                        {
                            case '0': .. case '9':
                                state = State.IntDigits;
                                text ~= c;
                                break;
                            case 'x': state = State.HexDigits; text ~= c; break;
                            case 'b': state = State.BinDigits; text ~= c; break;
                            default:
                                flushText();
                                return Token.Integer;
                        }                            
                        break;
                    case State.IntDigits:
                        switch(c)
                        {
                            case '0': .. case '9':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.Integer;
                        }
                        break;
                    case State.HexDigits:
                        switch(c)
                        {
                            case '0': .. case '9':
                            case 'a': .. case 'f':
                            case 'A': .. case 'F':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.Hexadecimal;
                        }
                        break;
                    case State.BinDigits:
                        switch(c)
                        {
                            case '0': .. case '1':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.Binary;
                        }
                        break;
                    case State.Tilde:
                        state = State.Start;
                        switch(c)
                        {
                            case '=': position++; return Token.NotEqual;
                            default: return Token.Not;
                        }
                        break;
                    case State.Plus:
                        state = State.Start;
                        switch(c)
                        {
                            case '#': position++; return Token.AddC;
                            case '+': position++; return Token.Inc;
                            default: return Token.Add;
                        }
                        break;
                    case State.Minus:
                        state = State.Start;
                        switch(c)
                        {
                            case '#': position++; return Token.SubC;
                            case '-': position++; return Token.Dec;
                            default: return Token.Sub;
                        }
                        break;
                    case State.Slash:
                        state = State.Start;
                        switch(c)
                        {
                            case '/':
                                state = State.SlashSlashComment;
                                break;
                            case '*':
                                state = State.SlashStarComment;
                                commentLine = location.line;
                                break;
                            default:
                                return Token.Div;
                        }
                        break;
                    case State.SlashSlashComment: break;
                    case State.SlashStarComment:
                        switch(c)
                        {
                            case '*': state = State.SlashStarCommentStar;
                            default: break;
                        }
                        break;
                    case State.SlashStarCommentStar:
                        switch(c)
                        {
                            case '/': state = State.Start; break;
                            default: state = State.SlashStarComment; break;
                        }
                        break;
                    case State.Equals:
                        state = State.Start;
                        switch(c)
                        {
                            case '=': position++; return Token.Equal;
                            default: return Token.Set;
                        }
                        break;
                    case State.Less:
                        state = State.Start;
                        switch(c)
                        {
                            case '<': state = State.LessLess; break;
                            case '>': position++; return Token.Swap;
                            case '=': position++; return Token.LessEqual;
                            default: return Token.Less;
                        }
                        break;
                    case State.LessLess:
                        state = State.Start;
                        switch(c)
                        {
                            case '<': state = State.LessLessLess; break;
                            case '-': position++; return Token.ArithShiftL;
                            default: return Token.ShiftL;
                        }
                        break;
                    case State.LessLessLess:
                        state = State.Start;
                        switch(c)
                        {
                            case '#': position++; return Token.RotateLC;
                            default: return Token.RotateL;
                        }
                        break;
                    case State.Greater:
                        state = State.Start;
                        switch(c)
                        {
                            case '>': state = State.GreaterGreater; break;
                            case '=': position++; return Token.GreaterEqual;
                            default: return Token.Greater;
                        }
                        break;
                    case State.GreaterGreater:
                        state = State.Start;
                        switch(c)
                        {
                            case '>': state = State.GreaterGreaterGreater; break;
                            case '-': position++; return Token.ArithShiftR;
                            default: return Token.ShiftR;
                        }
                        break;
                    case State.GreaterGreaterGreater:
                        state = State.Start;
                        switch(c)
                        {
                            case '#': position++; return Token.RotateRC;
                            default: return Token.RotateR;
                        }
                        break;
                    default:
                        error(std.string.format("unhandled state! (%s)", state));
                        assert(false);
                }
                position++;
            }
            
            if(file.isOpen() && file.readln(buffer))
            {   
                // Special handling in states for end-of-line.
                switch(state)
                {
                    case State.SlashSlashComment:
                        state = State.Start;
                        break;
                    case State.String:
                        state = State.Start;
                        error(std.string.format("expected closing quote %s, but got end-of-line", terminator));
                        break;
                    case State.StringEscape:
                        state = State.Start;
                        error("expected string escape sequence, but got end-of-line");
                        break;
                    default:
                        break;
                }
                position = 0;
                location.line++;
            }
            else
            {
                // End-of-file.
                switch(state)
                {
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
                        state = State.Start;
                        error(std.string.format("expected closing quote %s, but got end-of-file", terminator));
                        break;
                    case State.StringEscape:
                        state = State.Start;
                        error("expected string escape sequence, but got end-of-file");
                        break;
                    case State.SlashStarComment:
                        error(std.string.format("expected */ to close /* comment starting on line %d, but got end-of-file", commentLine));
                        break;
                    default:
                        break;
                }
                return Token.EndOfFile;
            }
        }
        assert(false);
    }
}