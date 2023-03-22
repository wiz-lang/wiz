#ifndef WIZ_FORMAT_DEBUG_MLB_DEBUG_FORMAT_H
#define WIZ_FORMAT_DEBUG_MLB_DEBUG_FORMAT_H

#include <wiz/format/debug/debug_format.h>

namespace wiz {
    typedef std::pair<std::size_t, StringView> OwnershipKey;
    struct OwnershipKeyHash {
        std::size_t operator() (const OwnershipKey& pair) const {
            return std::hash<std::size_t>()(pair.first) ^ std::hash<StringView>()(pair.second);
        }
    };
    typedef std::unordered_map<OwnershipKey, const Definition*, OwnershipKeyHash> MlbAddressOwnership;

    class MlbDebugFormat : public DebugFormat {
        public:
            MlbDebugFormat();
            ~MlbDebugFormat() override;

            bool generate(DebugFormatContext& context) override;
    };
}

#endif
