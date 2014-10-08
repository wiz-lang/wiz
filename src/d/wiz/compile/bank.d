module wiz.compile.bank;

import wiz.lib;
import wiz.compile.lib;

class Bank
{
    private:
        // Value used to pad unused bank space.
        enum ubyte PadValue = 0xFF;

        // The index of this bank, used by debug symbols.
        int index;
        // The name of this bank.
        string name;
        // Until a ROM relocation occurs, this page is not initialized.
        bool initialized;
        // Origin point as an absolute memory address where this bank starts.
        size_t origin;
        // The position, in bytes, into the bank.
        size_t position;
        // Whether or not this bank needs physical storage in the ROM.
        bool physical;
        // The total size of this bank, used to prevent write overflows, and calculate space in the ROM.
        size_t capacity;
        // The ROM data held by this bank.
        ubyte[] data;

        // An label and size that may be associated with each address in this bank, used by debug symbols.
        // A debugger will expect that each address can have exactly one label, so this is reflected here.
        // This also won't verify for conflicts when two symbols alias or have overlapping regions.
        string[size_t] addressNames;
        size_t[size_t] addressSizes;
    
        void reserve(string description, size_t size, compile.Location location)
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
        this(size_t index, string name, bool physical, size_t capacity)
        {
            this.index = index;
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

        void reserveVirtual(string description, size_t size, compile.Location location)
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

        void reservePhysical(string description, size_t size, compile.Location location)
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

        size_t registerAddress(string name, size_t size, string description, compile.Location location)
        {
            auto address = checkAddress(description, location);
            if(name && name[0] == '$')
            {
                name = std.string.format(".%s.%04X", name[1 .. name.length], address);
            }
            addressNames[address] = name;
            addressSizes[address] = size;
            return address;
        }

        size_t checkAddress(string description, compile.Location location)
        {
            if(initialized)
            {
                auto address = origin + position;
                return address;
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
        
        void setAddress(string description, size_t dest, compile.Location location)
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

        void exportNameLists(ref string[string] namelists)
        {
            string key;
            if(!physical)
            {
                key = "ram";
            }
            else
            {
                if(!index)
                {
                    return;
                }
                key = std.string.format("%X", index - 1);
            }

            string namelist = namelists.get(key, "");
            foreach(address, name; addressNames)
            {
                auto size = addressSizes.get(address, 0);
                if(size > 1)
                {
                    namelist ~= std.string.format("$%04X/%X#%s#\n", address, size, name);
                }
                else
                {
                    namelist ~= std.string.format("$%04X#%s#\n", address, name);
                }
            }
            namelists[key] = namelist;
        }
}