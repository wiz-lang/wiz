#include <wiz/format/format.h>
#include <wiz/format/binary_format.h>
#include <wiz/format/gb_format.h>
#include <wiz/format/nes_format.h>
#include <wiz/format/sms_format.h>
#include <wiz/format/snes_format.h>

namespace wiz {
    FormatCollection::FormatCollection() {
        add("bin"_sv, std::make_unique<BinaryFormat>());
        add("gb"_sv, std::make_unique<GameBoyFormat>());
        add("nes"_sv, std::make_unique<NesFormat>());
        add("sms"_sv, std::make_unique<SmsFormat>(SmsFormat::SystemType::MasterSystem));
        add("gg"_sv, std::make_unique<SmsFormat>(SmsFormat::SystemType::GameGear));
        add("sfc"_sv, std::make_unique<SnesFormat>());
        add("smc"_sv, std::make_unique<SnesSmcFormat>());
    }

    FormatCollection::~FormatCollection() {}

    void FormatCollection::add(StringView name, std::unique_ptr<Format> format) {
        formats[name] = std::move(format);
    }

    Format* FormatCollection::find(StringView name) const {
        const auto match = formats.find(name);
        if (match != formats.end()) {
            return match->second.get();
        }
        return nullptr;
    }
}

