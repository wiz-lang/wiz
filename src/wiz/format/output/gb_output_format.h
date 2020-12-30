#ifndef WIZ_FORMAT_OUTPUT_GB_OUTPUT_FORMAT_H
#define WIZ_FORMAT_OUTPUT_GB_OUTPUT_FORMAT_H

#include <wiz/format/output/output_format.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    class GameBoyOutputFormat : public OutputFormat {
        public:
            GameBoyOutputFormat();
            ~GameBoyOutputFormat() override;

            bool generate(OutputFormatContext& context) override;
    };
}

#endif