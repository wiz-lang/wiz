(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};

    wiz.VersionText = '0.1 (js)';

    function scannerTest(scanner) {
        do {
            var token = scanner.next();
            var text = scanner.consumeLastText();
            wiz.compile.log(token + ' ' + wiz.parse.getVerboseTokenName(token, text));
        } while(token != wiz.parse.Token.EndOfFile);
    }

    function run(source, filename, platform) {
        var scanner = wiz.parse.Scanner(wiz.fs.MemoryFile(source), filename);
        var parser = wiz.parse.Parser(scanner);
        var program;
        switch(platform) {
            case "gb":
                program = wiz.compile.Program(wiz.cpu.gameboy.GameboyPlatform());
                break;
            case "6502":
                program = wiz.compile.Program(wiz.cpu.mos6502.Mos6502Platform());
                break;
            default:
                throw new Error("unsupported platform " + platform);
        }

        wiz.compile.log(">> Building...");
        var block = parser.parse();

        wiz.compile.build(program, block);
      
        wiz.compile.log(">> Writing ROM...");
        var buffer = program.save();

        //wiz.compile.log(">> Wrote to '" + output + "'.");
        wiz.compile.notice("Done.");
        return buffer;
    }

    wiz.build = function(source, filename, platform) {
        wiz.compile.clearErrors();
        wiz.compile.clearLog();
        wiz.fs.reset();
        wiz.compile.notice("version " + wiz.VersionText);

        try {
            return run(source, filename, platform);
        } catch(e) {
            if(e instanceof wiz.compile.CompileExit) {
                // Exit request, handle silently.
            } else {
                wiz.compile.log(e.stack);
                throw e;
            }
        }
    }
})(this);