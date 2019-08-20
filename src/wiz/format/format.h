#ifndef WIZ_FORMAT_FORMAT_H
#define WIZ_FORMAT_FORMAT_H

#include <cstdint>
#include <unordered_map>

#include <wiz/compiler/bank.h>
#include <wiz/utility/report.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/string_pool.h>

namespace wiz {
    class Config;

    struct FormatContext {
        FormatContext(Report* report,
            StringPool* stringPool,
            const Config* config,
            StringView outputName,
            ArrayView<const Bank*> banks)
        : report(report),
        stringPool(stringPool),
        config(config),
        outputName(outputName) {}

        Report* report;
        StringPool* stringPool;
        const Config* config;
        StringView outputName;
        ArrayView<const Bank*> banks;

        std::unordered_map<const Bank*, std::size_t> bankOffsets;
        std::vector<std::uint8_t> data;
    };

    class Format {
        public:
            virtual ~Format() {};

            virtual bool generate(FormatContext& context) = 0;
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
