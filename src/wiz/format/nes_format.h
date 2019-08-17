#ifndef WIZ_FORMAT_NES_FORMAT_H
#define WIZ_FORMAT_NES_FORMAT_H

#include <wiz/format/format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class NesFormat : public Format {
        public:
            NesFormat();
            ~NesFormat() override;

            bool generate(Report* report, StringView outputName, const Config& config, ArrayView<UniquePtr<Bank>> banks, std::vector<std::uint8_t>& data) override;
    };
}

#endif