#ifndef WIZ_FORMAT_SNES_FORMAT_H
#define WIZ_FORMAT_SNES_FORMAT_H

#include <wiz/format/format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class SnesFormat : public Format {
        public:
            SnesFormat();
            ~SnesFormat() override;

            bool generate(Report* report, StringView outputName, const Config& config, ArrayView<const Bank*> banks, FormatOutput& output) override;
    };

    class SnesSmcFormat : public Format {
        public:
            SnesSmcFormat();
            ~SnesSmcFormat() override;

            bool generate(Report* report, StringView outputName, const Config& config, ArrayView<const Bank*> banks, FormatOutput& output) override;
    };
}

#endif