#ifndef WIZ_FORMAT_OUTPUT_OUTPUT_FORMAT_H
#define WIZ_FORMAT_OUTPUT_OUTPUT_FORMAT_H

#include <cstdint>
#include <unordered_map>

#include <wiz/compiler/address.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class Bank;
    class Config;
    class Report;
    class StringPool;

    struct OutputFormatContext {
        OutputFormatContext(Report* report,
            StringPool* stringPool,
            const Config* config,
            StringView outputName,
            ArrayView<const Bank*> banks)
        : report(report),
        stringPool(stringPool),
        config(config),
        outputName(outputName),
        banks(banks) {}

        Optional<std::size_t> getOutputOffset(Address address) const;

        Report* report;
        StringPool* stringPool;
        const Config* config;
        StringView outputName;
        ArrayView<const Bank*> banks;

        // The size of the file header, non-zero only if it exists in its own section outside the main program ROM.
        // (Used to determine the 'true offset' of ROM banks, as debug symbols may need.) 
        std::size_t fileHeaderPrefixSize = 0;
        // The default debug bank size of the bank part of PRG debug symbols for the platform.
        // Some platforms simply use the absolute byte offset in the ROM file to determine their address,
        // others need to divide the offset to get a bank index.
        std::size_t debugBankSize = 65536;

        std::unordered_map<const Bank*, std::size_t> bankOffsets;
        std::vector<std::uint8_t> data;
    };

    class OutputFormat {
        public:
            virtual ~OutputFormat() {};

            virtual bool generate(OutputFormatContext& context) = 0;
    };

    class OutputFormatCollection {
        public:
            OutputFormatCollection();
            ~OutputFormatCollection();

            void add(StringView name, std::unique_ptr<OutputFormat> format);
            OutputFormat* find(StringView name) const;

        private:
            std::unordered_map<StringView, std::unique_ptr<OutputFormat>> formats;
    };
}

#endif
