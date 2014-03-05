(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.maximumErrors = 64;
    var errors;
    var previousContinued;

    wiz.compile.clearErrors = function() {
        errors = 0;
        previousContinued = false;
    }


    wiz.compile.clearLog = function() {}

    wiz.compile.CompileExit = function() {
        this.name = 'wiz.compile.CompileExit';
        this.message = 'Compile exit requested.';
    };

    function severity(fatal, previousContinued) {
        if(fatal) {
            return "fatal";
        } else if(previousContinued) {
            return "note";
        } else {
            return "error";
        }
    }

    wiz.compile.error = function(message, location, fatal, continued) {
        wiz.compile.log(location.toString(location) + ": " + severity(fatal, previousContinued) + ": " + message);
        if(!continued) {
            errors++;
        }
        previousContinued = continued;
        if(fatal || errors >= wiz.compile.maximumErrors) {
            wiz.compile.abort();
        }
    };

    wiz.compile.verify = function() {
        if(errors > 0) {
            wiz.compile.abort();
        }
    };

    wiz.compile.log = function(message) {
        console.log(message);
    };

    wiz.compile.notice = function(message) {
        wiz.compile.log("* wiz: " + message);
    };

    wiz.compile.abort = function() {
        wiz.compile.notice("failed with " + errors + " error(s)");
        throw new wiz.compile.CompileExit();
    };
})(this);