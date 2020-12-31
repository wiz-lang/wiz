#ifndef WIZ_FORMAT_DEBUG_MLB_DEBUG_FORMAT_H
#define WIZ_FORMAT_DEBUG_MLB_DEBUG_FORMAT_H

#include <wiz/format/debug/debug_format.h>

namespace wiz {
    class MlbDebugFormat : public DebugFormat {
        public:
            MlbDebugFormat();
            ~MlbDebugFormat() override;

            bool generate(DebugFormatContext& context) override;
    };
}

#endif
