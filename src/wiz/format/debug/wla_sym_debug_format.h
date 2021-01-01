#ifndef WIZ_FORMAT_DEBUG_WLA_SYM_DEBUG_FORMAT_H
#define WIZ_FORMAT_DEBUG_WLA_SYM_DEBUG_FORMAT_H

#include <wiz/format/debug/debug_format.h>

namespace wiz {
    class WlaSymDebugFormat : public DebugFormat {
        public:
            WlaSymDebugFormat();
            ~WlaSymDebugFormat() override;

            bool generate(DebugFormatContext& context) override;
    };
}

#endif
