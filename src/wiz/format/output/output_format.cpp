#include <wiz/format/output/output_format.h>
#include <wiz/format/output/binary_output_format.h>
#include <wiz/format/output/gb_output_format.h>
#include <wiz/format/output/nes_output_format.h>
#include <wiz/format/output/sms_output_format.h>
#include <wiz/format/output/snes_output_format.h>

namespace wiz {
    Optional<std::size_t> OutputFormatContext::getOutputOffset(Address address) const {
        const auto bankOffset = bankOffsets.find(address.bank);
        return bankOffset != bankOffsets.end() && address.relativePosition.hasValue()
            ? address.relativePosition.get() + bankOffset->second
            : Optional<std::size_t>();
    }



    OutputFormatCollection::OutputFormatCollection() {
        add("bin"_sv, std::make_unique<BinaryOutputFormat>());
        add("gb"_sv, std::make_unique<GameBoyOutputFormat>());
        add("nes"_sv, std::make_unique<NesOutputFormat>());
        add("sms"_sv, std::make_unique<SmsOutputFormat>(SmsOutputFormat::SystemType::MasterSystem));
        add("gg"_sv, std::make_unique<SmsOutputFormat>(SmsOutputFormat::SystemType::GameGear));
        add("sfc"_sv, std::make_unique<SnesOutputFormat>());
        add("smc"_sv, std::make_unique<SnesSmcOutputFormat>());
    }

    OutputFormatCollection::~OutputFormatCollection() {}

    void OutputFormatCollection::add(StringView name, std::unique_ptr<OutputFormat> format) {
        formats[name] = std::move(format);
    }

    OutputFormat* OutputFormatCollection::find(StringView name) const {
        const auto match = formats.find(name);
        if (match != formats.end()) {
            return match->second.get();
        }
        return nullptr;
    }
}

