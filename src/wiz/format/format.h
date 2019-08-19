#ifndef WIZ_FORMAT_FORMAT_H
#define WIZ_FORMAT_FORMAT_H

#include <cstdint>
#include <unordered_map>

#include <wiz/compiler/bank.h>
#include <wiz/utility/report.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class Config;

    struct FormatOutput {
        std::unordered_map<const Bank*, std::size_t> bankOffsets;
        std::vector<std::uint8_t> data;
    };

    class Format {
        public:
            virtual ~Format() {};

            virtual bool generate(Report* report, StringView outputName, const Config& configItems, ArrayView<const Bank*> banks, FormatOutput& output) = 0;
    };

    class FormatCollection {
        public:
            FormatCollection();
            ~FormatCollection();

            void add(StringView name, std::unique_ptr<Format> format);
            Format* find(StringView name) const;

        private:
            std::unordered_map<StringView, std::unique_ptr<Format>> formats;
    };
}

#endif
