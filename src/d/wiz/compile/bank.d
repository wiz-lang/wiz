module wiz.compile.bank;

import wiz.lib;
import wiz.compile.lib;

class Bank
{
    private:
        // Value used to pad unused bank space.
        enum ubyte PadValue = 0xFF;

        // The name of this bank.
        string name;
        // Until a ROM relocation occurs, this page is not initialized.
        bool initialized;
        // Origin point as an absolute memory address where this bank starts.
        ulong origin;
        // The position, in bytes, into the bank.
        ulong position;
        // Whether or not this bank needs physical storage in the ROM.
        bool physical;
        // The total size of this bank, used to prevent write overflows, and calculate space in the ROM.
        ulong capacity;
        // The ROM data held by this bank.
        ubyte[] data;
    
        void reserve(string description, ulong size, compile.Location location)
        {
            if(initialized)
            {
                if(position + size > capacity)
                {
                    error(
                        std.string.format(
                            "%s needs %s byte(s), which exceeds the remaining space in '%s : %s * %d' by %s byte(s)",
                            description, size, name, physical ? "rom" : "ram", capacity, position + size - capacity
                        ), location, true
                    );
                }
                else
                {
                    position += size;
                }
            }
            else
            {
                error(
                    std.string.format(
                        "%s found inside '%s : %s * %d', but the bank has no address yet.",
                        description, name, physical ? "rom" : "ram", capacity
                    ), location, true
                );
            }
        }

    public:
        this(string name, bool physical, ulong capacity)
        {
            this.name = name;
            this.physical = physical;
            this.capacity = capacity;
            
            initialized = false;
            position = 0;
            
            if(physical)
            {
                data = new ubyte[capacity];
                data[] = PadValue;
            }
        }

        void rewind()
        {
            position = 0;
            initialized = false;
        }

        void reserveVirtual(string description, ulong size, compile.Location location)
        {
            if(physical)
            {
                error(
                    std.string.format(
                        "%s is not allowed in '%s : rom * %d'",
                        description, name, capacity
                    ), location, true
                );
            }
            else
            {
                reserve(description, size, location);
            }
        }

        void reservePhysical(string description, ulong size, compile.Location location)
        {
            if(physical)
            {
                reserve(description, size, location);
            }
            else
            {
                error(
                    std.string.format(
                        "%s is not allowed in '%s : ram * %d'",
                        description, name, capacity
                    ), location, true
                );
            }
        }

        void writePhysical(ubyte[] items, compile.Location location)
        {
            if(position + items.length > capacity)
            {
                error("attempt to write outside of bank's reserved space.", location, true);
            }
            else
            {
                foreach(item; items)
                {
                    data[position++] = item & 0xFF;
                }
            }
        }

        ulong checkAddress(string description, compile.Location location)
        {
            if(initialized)
            {
                return origin + position;
            }
            else
            {
                error(
                    std.string.format(
                        "%s is not allowed in '%s : %s * %d' before knowing its start address.",
                        description, name, physical ? "rom" : "ram", capacity
                    ), location, true
                );
                return 0xFACEBEEF;
            }
        }
        
        void setAddress(string description, ulong dest, compile.Location location)
        {
            if(initialized)
            {
                if(dest < origin + position)
                {
                    error(
                        std.string.format(
                            "attempt to move backwards within '%s : %s * %d'. (location %s -> %s)",
                            name, physical ? "rom" : "ram", capacity, origin + position, dest
                        ), location, true
                    );
                }
                else
                {   
                    reserve(description, dest - (origin + position), location);
                }
            }
            else
            {
                if(dest + capacity > 65536)
                {
                    error(
                        std.string.format(
                            "'%s : %s * %d' with start location %s has an invalid upper bound %s, outside of addressable memory 0..65535.",
                            name, physical ? "rom" : "ram", capacity, dest, dest + capacity
                        ), location, true
                    );                
                }
                origin = dest;
                initialized = true;
            }
        }

        void dump(ref ubyte[] buffer)
        {
            if(physical)
            {
                buffer ~= data;
            }
        }
}