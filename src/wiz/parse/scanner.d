module wiz.parse.scanner;

static import std.stdio;
static import std.string;

import wiz.lib;
import wiz.parse.lib;

private enum State
{
    START,
    STRING,
    STRING_ESCAPE,
    LEADING_ZERO,
    INT_DIGITS,
    HEX_DIGITS,
    BIN_DIGITS,
    IDENTIFIER,
    SLASH,
    SLASH_SLASH_COMMENT,
    SLASH_STAR_COMMENT,
    SLASH_STAR_COMMENT_STAR,
    EXCLAIM,
    PLUS,
    MINUS,
    EQUALS,
    LT,
    LT_LT,
    LT_LT_LT,
    GT,
    GT_GT,
    GT_GT_GT,
}

class Scanner
{
    private char terminator;
    private uint position;
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
        state = State.START;
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
                    case State.START:
                        switch(c)
                        {
                            case '0':
                                state = State.LEADING_ZERO;
                                text ~= c;
                                break;
                            case '1': .. case '9':
                                state = State.INT_DIGITS;
                                text ~= c;
                                break;
                            case '_':
                            case 'a': .. case 'z':
                            case 'A': .. case 'Z':
                                state = State.IDENTIFIER;
                                text ~= c;
                                break;
                            case '\'', '\"':
                                terminator = c;
                                state = State.STRING;
                                break;
                            case ' ', '\t', '\r', '\n': break;
                            case ':': position++; return Token.COLON;
                            case ',': position++; return Token.COMMA;
                            case '.': position++; return Token.DOT;
                            case '(': position++; return Token.LPAREN;
                            case ')': position++; return Token.RPAREN;
                            case '[': position++; return Token.LBRACKET;
                            case ']': position++; return Token.RBRACKET;
                            case '{': position++; return Token.LBRACE;
                            case '}': position++; return Token.RBRACE;
                            case ';': position++; return Token.SEMI;
                            case '#': position++; return Token.HASH;
                            case '|': position++; return Token.OR;
                            case '*': position++; return Token.MUL;
                            case '%': position++; return Token.MOD;
                            case '@': position++; return Token.AT;
                            case '~': position++; return Token.NOT;
                            case '&': position++; return Token.AND;
                            case '^': position++; return Token.XOR;
                            case '!': state = State.EXCLAIM; break;
                            case '+': state = State.PLUS; break;
                            case '-': state = State.MINUS; break;
                            case '/': state = State.SLASH; break;
                            case '=': state = State.EQUALS; break;
                            case '<': state = State.LT; break;
                            case '>': state = State.GT; break;
                            default: error(std.string.format("unrecognized character %s found.", c));
                        }
                        break;
                    case State.IDENTIFIER:
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
                                return Token.IDENTIFIER;
                        }
                        break;
                    case State.STRING:
                        if(c == terminator)
                        {
                            position++;
                            flushText();
                            return Token.STRING;
                        }
                        else switch(c)
                        {
                            case '\\':
                                state = State.STRING_ESCAPE;
                                break;
                            default:
                                text ~= c;
                                break;
                        }
                        break;
                    case State.STRING_ESCAPE:
                        state = State.STRING;
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
                    case State.LEADING_ZERO:
                        switch(c)
                        {
                            case '0': .. case '9':
                                state = State.INT_DIGITS;
                                text ~= c;
                                break;
                            case 'x': state = State.HEX_DIGITS; text ~= c; break;
                            case 'b': state = State.BIN_DIGITS; text ~= c; break;
                            default:
                                flushText();
                                return Token.INTEGER;
                        }                            
                        break;
                    case State.INT_DIGITS:
                        switch(c)
                        {
                            case '0': .. case '9':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.INTEGER;
                        }
                        break;
                    case State.HEX_DIGITS:
                        switch(c)
                        {
                            case '0': .. case '9':
                            case 'a': .. case 'f':
                            case 'A': .. case 'F':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.HEXADECIMAL;
                        }
                        break;
                    case State.BIN_DIGITS:
                        switch(c)
                        {
                            case '0': .. case '1':
                                text ~= c;
                                break;
                            default:
                                flushText();
                                return Token.BINARY;
                        }
                        break;
                    case State.EXCLAIM:
                        state = State.START;
                        switch(c)
                        {
                            case '=': position++; return Token.NE;
                            default: return Token.EXCLAIM;
                        }
                        break;
                    case State.PLUS:
                        state = State.START;
                        switch(c)
                        {
                            case '#': position++; return Token.ADDC;
                            case '+': position++; return Token.INC;
                            default: return Token.ADD;
                        }
                        break;
                    case State.MINUS:
                        state = State.START;
                        switch(c)
                        {
                            case '#': position++; return Token.SUBC;
                            case '-': position++; return Token.DEC;
                            default: return Token.SUB;
                        }
                        break;
                    case State.SLASH:
                        state = State.START;
                        switch(c)
                        {
                            case '/':
                                state = State.SLASH_SLASH_COMMENT;
                                break;
                            case '*':
                                state = State.SLASH_STAR_COMMENT;
                                commentLine = location.line;
                                break;
                            default:
                                return Token.DIV;
                        }
                        break;
                    case State.SLASH_SLASH_COMMENT: break;
                    case State.SLASH_STAR_COMMENT:
                        switch(c)
                        {
                            case '*': state = State.SLASH_STAR_COMMENT_STAR;
                            default: break;
                        }
                        break;
                    case State.SLASH_STAR_COMMENT_STAR:
                        switch(c)
                        {
                            case '/': state = State.START; break;
                            default: state = State.SLASH_STAR_COMMENT; break;
                        }
                        break;
                    case State.EQUALS:
                        state = State.START;
                        switch(c)
                        {
                            case '=': position++; return Token.EQ;
                            default: return Token.SET;
                        }
                        break;
                    case State.LT:
                        state = State.START;
                        switch(c)
                        {
                            case '<': state = State.LT_LT; break;
                            case '>': position++; return Token.SWAP;
                            case '=': position++; return Token.LE;
                            default: return Token.LT;
                        }
                        break;
                    case State.LT_LT:
                        state = State.START;
                        switch(c)
                        {
                            case '<': state = State.LT_LT_LT; break;
                            case '-': position++; return Token.ASHL;
                            default: return Token.SHL;
                        }
                        break;
                    case State.LT_LT_LT:
                        state = State.START;
                        switch(c)
                        {
                            case '#': position++; return Token.ROLC;
                            default: return Token.ROL;
                        }
                        break;
                    case State.GT:
                        state = State.START;
                        switch(c)
                        {
                            case '>': state = State.GT_GT; break;
                            case '=': position++; return Token.GE;
                            default: return Token.GT;
                        }
                        break;
                    case State.GT_GT:
                        state = State.START;
                        switch(c)
                        {
                            case '>': state = State.GT_GT_GT; break;
                            case '-': position++; return Token.ASHR;
                            default: return Token.SHR;
                        }
                        break;
                    case State.GT_GT_GT:
                        state = State.START;
                        switch(c)
                        {
                            case '#': position++; return Token.RORC;
                            default: return Token.ROR;
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
                    case State.SLASH_SLASH_COMMENT:
                        state = State.START;
                        break;
                    case State.STRING:
                        state = State.START;
                        error(std.string.format("expected closing quote %s, but got end-of-line", terminator));
                        break;
                    case State.STRING_ESCAPE:
                        state = State.START;
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
                    case State.IDENTIFIER:
                        flushText();
                        return Token.IDENTIFIER;
                    case State.LEADING_ZERO:
                    case State.INT_DIGITS:
                        flushText();
                        return Token.INTEGER;
                    case State.HEX_DIGITS:
                        flushText();
                        return Token.HEXADECIMAL;
                    case State.BIN_DIGITS:
                        flushText();
                        return Token.BINARY;
                    case State.STRING:
                        state = State.START;
                        error(std.string.format("expected closing quote %s, but got end-of-file", terminator));
                        break;
                    case State.STRING_ESCAPE:
                        state = State.START;
                        error("expected string escape sequence, but got end-of-file");
                        break;
                    case State.SLASH_STAR_COMMENT:
                        error(std.string.format("expected */ to close /* comment starting on line %d, but got end-of-file", commentLine));
                        break;
                    default:
                        break;
                }
                return Token.EOF;
            }
        }
        assert(false);
    }
}