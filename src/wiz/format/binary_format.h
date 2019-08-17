#ifndef WIZ_FORMAT_BIN_FORMAT_H
#define WIZ_FORMAT_BIN_FORMAT_H

#include <wiz/format/format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class BinaryFormat : public Format {
        public:
            BinaryFormat();
            ~BinaryFormat() override;

            bool generate(Report* report, StringView outputName, const Config& config, ArrayView<UniquePtr<Bank>> banks, std::vector<std::uint8_t>& data) override;
    };
}

#endif