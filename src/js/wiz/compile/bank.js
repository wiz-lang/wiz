(function(globals) {
    'use strict';
    var wiz = globals.wiz = typeof(globals.wiz) !== 'undefined' ? globals.wiz : {};
    wiz.compile = typeof(wiz.compile) !== 'undefined' ? wiz.compile : {};

    wiz.compile.Bank = function(name, physical, capacity) {
        var that = {};

        var PadValue = 0xFF;

        // The name of this bank.
        that.name = name;
        // Until a ROM relocation occurs, this page is not initialized.
        that.initialized = false;
        // Origin point as an absolute memory address where this bank starts.
        that.origin = 0;
        // The position, in bytes, into the bank.
        that.position = 0;
        // Whether or not this bank needs physical storage in the ROM.
        that.physical = physical;
        // The total size of this bank, used to prevent write overflows, and calculate space in the ROM.
        that.capacity = capacity;
        // The ROM data held by this bank.
        that.data = new Uint8Array(0);

        if(that.physical) {
            that.data = new Uint8Array(capacity);
            (function() {
                for(var i = 0; i < that.capacity; i++) {
                    that.data[i] = PadValue;
                }
            })();
        }

        that.reserve = function(description, size, location) {
            if(that.initialized) {
                if(that.position + size > that.capacity) {
                    wiz.compile.error(
                        description + " needs " + size + " byte(s), which exceeds the remaining space in '"
                        + that.name + " :  " + (that.physical ? "rom" : "ram") + " * " + that.capacity
                        + "' by " + (that.position + size - that.capacity) + " byte(s)",
                        location, true
                    );
                }
                else {
                    that.position += size;
                }
            } else {
                wiz.compile.error(
                    description + " found inside '"
                    + that.name + " :  " + (that.physical ? "rom" : "ram") + " * " + that.capacity
                    + "', but the bank has no address yet.",
                    location, true
                );
            }
        }

        that.rewind = function() {
            that.position = 0;
            that.initialized = false;
        }
        
        that.reserveVirtual = function(description, size, location) {
            if(that.physical) {
                wiz.compile.error(
                    description + " is not allowed in '"
                    + name + " : rom * " + that.capacity + "'",
                    location, true
                );
            } else {
                that.reserve(description, size, location);
            }
        }

        that.reservePhysical = function(description, size, location) {
            if(that.physical) {
                that.reserve(description, size, location);
            } else {
                wiz.compile.error(
                    description + " is not allowed in '"
                    + name + " : ram * " + that.capacity + "'",
                    location, true
                );
            }
        }

        that.writePhysical = function(items, location) {
            if(that.position + items.length > that.capacity) {
                wiz.compile.error("attempt to write outside of bank's reserved space.", location, true);
            } else {
                for(var i = 0; i < items.length; i++) {
                    that.data[that.position++] = items[i] & 0xFF;
                }
            }
        }

        that.checkAddress = function(description, location) {
            if(that.initialized) {
                return that.origin + that.position;
            } else {
                wiz.compile.error(
                    description + " is not allowed in '"
                    + that.name + " :  " + (that.physical ? "rom" : "ram") + " * " + that.capacity
                    + "' before knowing its start address.",
                    location, true
                );
                return 0xFACEBEEF;
            }
        }

        function paddedHex(value, width) {
            var width = 4;
            value = value.toString(16);
            return address.length >= width ? value : new Array(4 - value.length + 1).join('0') + value;
        }

        that.setAddress = function(description, dest, location) {
            if(that.initialized) {
                if(that.dest < that.origin + that.position) {
                    wiz.compile.error(
                        "attempt to move backwards within '"
                        + that.name + " :  " + (that.physical ? "rom" : "ram") + " * " + that.capacity
                        + "'. (location 0x" + paddedHex(that.origin + that.position, 4) + " -> 0x" + paddedHex(dest, 4) + ")",
                        location, true
                    );
                } else {
                    that.reserve(description, dest - (that.origin + that.position), location);
                }
            } else {
                if(dest + that.capacity > 65536) {
                    wiz.compile.error(
                        "'"
                        + that.name + " :  " + (that.physical ? "rom" : "ram") + " * " + that.capacity
                        + "' with start location " + dest + " (0x" + paddedHex(dest, 4) + ") has an invalid upper bound "
                        + (dest + that.capacity) + " (0x" + paddedHex(dest + that.capacity, 4) + "), outside of addressable memory 0..65535.",
                        location, true
                    );              
                }
                that.origin = dest;
                that.initialized = true;
            }
        }


        that.dump = function(buffer) {
            if(that.physical) {
                var data = that.data;
                var len = that.data.length;
                for(var i = 0; i < len; i++) {
                    buffer.push(data[i]);
                }
            }
        }

        return that;
    };
})(this);