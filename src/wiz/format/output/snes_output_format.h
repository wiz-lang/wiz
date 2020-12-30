#ifndef WIZ_FORMAT_OUTPUT_SNES_OUTPUT_FORMAT_H
#define WIZ_FORMAT_OUTPUT_SNES_OUTPUT_FORMAT_H

#include <wiz/format/output/output_format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class SnesOutputFormat : public OutputFormat {
        public:
            SnesOutputFormat();
            ~SnesOutputFormat() override;

            bool generate(OutputFormatContext& context) override;
    };

    class SnesSmcOutputFormat : public OutputFormat {
        public:
            SnesSmcOutputFormat();
            ~SnesSmcOutputFormat() override;

            bool generate(OutputFormatContext& context) override;
    };
}

#endif