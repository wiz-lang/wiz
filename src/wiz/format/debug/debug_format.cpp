#include <wiz/format/debug/debug_format.h>
#include <wiz/format/debug/mlb_debug_format.h>
#include <wiz/format/debug/rgbds_sym_debug_format.h>

namespace wiz {
    DebugFormatCollection::DebugFormatCollection() {
        add("mlb"_sv, std::make_unique<MlbDebugFormat>());
        add("rgbds"_sv, std::make_unique<RgbdsSymDebugFormat>());
    }

    DebugFormatCollection::~DebugFormatCollection() {}

    std::size_t DebugFormatCollection::getFormatNameCount() const {
        return formatNames.size();
    }

    StringView DebugFormatCollection::getFormatName(std::size_t index) const {
        return formatNames[index];
    }

    void DebugFormatCollection::add(StringView name, std::unique_ptr<DebugFormat> format) {
        const auto ptr = format.get();
        formats.push_back(std::move(format));
        formatNames.push_back(name);
        formatsByName[name] = ptr;
    }

    DebugFormat* DebugFormatCollection::find(StringView name) const {
        const auto match = formatsByName.find(name);
        if (match != formatsByName.end()) {
            return match->second;
        }
        return nullptr;
    }
}

