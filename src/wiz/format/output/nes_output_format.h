#ifndef WIZ_FORMAT_OUTPUT_NES_OUTPUT_FORMAT_H
#define WIZ_FORMAT_OUTPUT_NES_OUTPUT_FORMAT_H

#include <wiz/format/output/output_format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class NesOutputFormat : public OutputFormat {
        public:
            NesOutputFormat();
            ~NesOutputFormat() override;

            bool generate(OutputFormatContext& context) override;
    };
}

#endif