module wiz.application;

static import std.file;
static import std.path;
static import std.stdio;
static import std.string;
static import std.array;
import wiz = wiz.lib;

private enum ArgumentState
{
    INPUT,
    OUTPUT
}

int run(string[] arguments)
{
    string[] args = arguments[1 .. arguments.length];
    
    ArgumentState state = ArgumentState.INPUT;
    string input;
    string output;
    
    wiz.notice("version " ~ wiz.VERSION_TEXT);
    
    foreach(i, arg; args)
    {
        if(arg[0] == '-')
        {
            switch(arg)
            {
                case "-o":
                    state = ArgumentState.OUTPUT;
                    break;
                case "-h":
                case "--help":
                    std.stdio.writeln("usage: " ~ wiz.PROGRAM_NAME ~ " [... arg]");
                    std.stdio.writeln("  where args can can be one of:");
                    std.stdio.writeln("    input_filename");
                    std.stdio.writeln("      (required) the name of the nel source file to compile");
                    std.stdio.writeln("    -o output_filename");
                    std.stdio.writeln("      the name of the .nes rom file to generate.");
                    std.stdio.writeln("      (defaults to $input_filename + '.nes').");
                    std.stdio.writeln("    -h, --help");
                    std.stdio.writeln("      this helpful mesage");
                    return 1;
                default:
                    wiz.notice(std.string.format("unknown command line option '%s'. ignoring...", arg));
                    break;
            }
        }
        else
        {
            switch(state)
            {
                default:
                case ArgumentState.INPUT:
                    
                    if(input != "")
                    {
                        wiz.notice(std.string.format("input file already set to '%s'. skipping '%s'", input, arg));
                    }
                    else
                    {
                        input = arg;
                    }
                    break;
                case ArgumentState.OUTPUT:
                    if(output != "")
                    {
                        wiz.notice(std.string.format("output file already set to '%s'. skipping '%s'", output, arg));
                    }
                    else
                    {
                        output = arg;
                    }
                    state = ArgumentState.INPUT;
                    break;
            }
        }
    }
    
    if(!input)
    {
        wiz.notice("no input file given. type `" ~ wiz.PROGRAM_NAME ~ " --help` to see program usage.");
        return 1;
    }
    // Assume a default file of <<input_filename>>.gb
    if(!output)
    {
        output = std.path.stripExtension(input) ~ ".gb";
    }
    
    if(std.file.exists(input))
    {
        if(std.file.isDir(input))
        {
            wiz.log("error: input '" ~ input ~ "' is a directory.");
            return 1;
        }
    }
    else
    {
        wiz.log("error: input file '" ~ input ~ "' does not exist.");
        return 1;
    }
    
    if(std.file.exists(output) && std.file.isDir(output))
    {
        wiz.log("error: output '" ~ output ~ "' is a directory.");
        return 1;
    }

    auto scanner = new wiz.parse.Scanner(std.stdio.File(input, "rb"), input);
    auto parser = new wiz.parse.Parser(scanner);
    auto program = new wiz.compile.Program(new wiz.cpu.GameboyPlatform());

    wiz.log(">> Building...");
    auto block = parser.parse();
    wiz.compile.build(program, block);
  
    try
    {
/*      std.stdio.File file = std.stdio.File(output, "wb");
        wiz.log(">> Saving ROM...");
        program.save(file);
        file.close();*/
    }
    catch(Exception e)
    {
        wiz.log("error: output '" ~ output ~ "' could not be written.");
        wiz.compile.abort();
    }

    wiz.log(">> Wrote to '" ~ output ~ "'.");
    wiz.notice("Done.");
    return 0;
}

int main(string[] arguments)
{
    try
    {
        return run(arguments);
    }
    catch(wiz.compile.CompileExit exit)
    {
        return 1;
    }
}