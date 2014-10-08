module wiz.application;

static import std.file;
static import std.path;
static import std.stdio;
static import std.string;
static import std.array;
import wiz = wiz.lib;

private enum ArgumentState
{
    Input,
    Output
}

int run(string[] arguments)
{
    string[] args = arguments[1 .. arguments.length];
    
    ArgumentState state = ArgumentState.Input;
    string input;
    string output;
    string platform;
    bool useNamelists;
    
    wiz.notice("version " ~ wiz.VersionText);
    
    foreach(i, arg; args)
    {
        if(arg[0] == '-')
        {
            switch(arg)
            {
                case "-o":
                    state = ArgumentState.Output;
                    break;
                case "-h":
                case "--help":
                    std.stdio.writeln("usage: " ~ wiz.ProgramName ~ " [... arg]");
                    std.stdio.writeln("  where args can can be one of:");
                    std.stdio.writeln("    input_filename");
                    std.stdio.writeln("      the name of the source file to compile");
                    std.stdio.writeln("    -o output_filename");
                    std.stdio.writeln("      the name of the rom file to generate.");
                    std.stdio.writeln("    -gb");
                    std.stdio.writeln("      generate GBZ80 code (Game Boy, Game Boy Color)");
                    std.stdio.writeln("    -6502");
                    std.stdio.writeln("      generate 6502 code (NES, C64, Atari, Apple, etc)");
                    std.stdio.writeln("    -h, --help");
                    std.stdio.writeln("      this helpful mesage");
                    return 1;
                case "-gb":
                    if(platform)
                    {
                        wiz.notice(std.string.format("platform already set to '%s'. skipping '%s'", platform, arg));
                    }
                    else
                    {
                        platform = arg;
                    }
                    break;
                case "-6502":
                    if(platform)
                    {
                        wiz.notice(std.string.format("platform already set to '%s'. skipping '%s'", platform, arg));
                    }
                    else
                    {
                        platform = arg;
                    }
                    break;
                case "-nl":
                    useNamelists = true;
                    break;
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
                case ArgumentState.Input:
                    
                    if(input)
                    {
                        wiz.notice(std.string.format("input file already set to '%s'. skipping '%s'", input, arg));
                    }
                    else
                    {
                        input = arg;
                    }
                    break;
                case ArgumentState.Output:
                    if(output)
                    {
                        wiz.notice(std.string.format("output file already set to '%s'. skipping '%s'", output, arg));
                    }
                    else
                    {
                        output = arg;
                    }
                    state = ArgumentState.Input;
                    break;
            }
        }
    }
    
    if(!input)
    {
        wiz.notice("no input file given. type `" ~ wiz.ProgramName ~ " --help` to see program usage.");
        return 1;
    }
    if(!output)
    {
        wiz.notice("no output file given. type `" ~ wiz.ProgramName ~ " --help` to see program usage.");
        return 1;
    }
    if(!platform)
    {
        wiz.notice("no platform given. type `" ~ wiz.ProgramName ~ " --help` to see program usage.");
        return 1;
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
    wiz.compile.Program program;
    switch(platform)
    {
        case "-gb":
            program = new wiz.compile.Program(new wiz.cpu.gameboy.GameboyPlatform());
            break;
        case "-6502":
            program = new wiz.compile.Program(new wiz.cpu.mos6502.Mos6502Platform());
            break;
        default:
            assert(0);
    }

    wiz.log(">> Building...");
    auto block = parser.parse();
    wiz.compile.build(program, block);
  
    try
    {
        auto file = std.stdio.File(output, "wb");
        wiz.log(">> Writing ROM...");
        file.rawWrite(program.save());
        file.close();
    }
    catch(Exception e)
    {
        wiz.log("error: output '" ~ output ~ "' could not be written.");
        wiz.compile.abort();
    }
    wiz.log(">> Wrote to '" ~ output ~ "'.");

    if(useNamelists)
    {
        auto namelists = program.exportNameLists();
        wiz.log(">> Writing namelists...");
        foreach(name, namelist; namelists)
        {
            auto fn = output ~ "." ~ name ~ ".nl";
            try
            {
                auto file = std.stdio.File(fn, "w");
                wiz.log("   '" ~ fn ~ "'...");
                file.write(namelist);
                file.close();
            }
            catch(Exception e)
            {
                wiz.log("error: namelist '" ~ fn ~ "' could not be written.");
                wiz.compile.abort();
            }
        }
        wiz.log(">> Wrote all namelists.");
    }

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