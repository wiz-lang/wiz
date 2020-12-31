#ifndef WIZ_FORMAT_DEBUG_RGBDS_SYM_DEBUG_FORMAT_H
#define WIZ_FORMAT_DEBUG_RGBDS_SYM_DEBUG_FORMAT_H

#include <wiz/format/debug/debug_format.h>

namespace wiz {
    class RgbdsSymDebugFormat : public DebugFormat {
        public:
            RgbdsSymDebugFormat();
            ~RgbdsSymDebugFormat() override;

            bool generate(DebugFormatContext& context) override;
    };
}

#endif
