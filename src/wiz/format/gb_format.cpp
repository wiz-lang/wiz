#include <algorithm>
#include <cctype>

#include <wiz/utility/path.h>
#include <wiz/utility/misc.h>
#include <wiz/ast/expression.h>
#include <wiz/compiler/config.h>
#include <wiz/format/gb_format.h>

namespace wiz {
    namespace {
        const std::size_t RomBankSize = 32 * 1024;
        const std::size_t MaxTotalRomSize = 8 * 1024U * 1024U;
        const std::uint8_t LogoBitmap[] = {
            0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
            0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
            0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E,
        };

        std::unordered_map<StringView, std::uint8_t> cartTypes {
            {"rom"_sv, 0x00},
            {"mbc1"_sv, 0x01},
            {"mbc1-ram"_sv, 0x02},
            {"mbc1-ram-battery"_sv, 0x03},
            {"mbc2"_sv, 0x05},
            {"mbc2-battery"_sv, 0x06},
            {"rom-ram"_sv, 0x08},
            {"rom-ram-battery"_sv, 0x09},
            {"mmm01"_sv, 0x0B},
            {"mmm01-ram"_sv, 0x0C},
            {"mmm01-ram-battery"_sv, 0x0D},
            {"mbc3-timer-battery"_sv, 0x0F},
            {"mbc3-timer-ram-battery"_sv, 0x10},
            {"mbc3"_sv, 0x11},
            {"mbc3-ram"_sv, 0x12},
            {"mbc3-ram-battery"_sv, 0x13},
            {"mbc4"_sv, 0x15},
            {"mbc4-ram"_sv, 0x16},
            {"mbc4-ram-battery"_sv, 0x17},
            {"mbc5"_sv, 0x19},
            {"mbc5-ram"_sv, 0x1A},
            {"mbc5-ram-battery"_sv, 0x1B},
            {"mbc5-rumble"_sv, 0x1C},
            {"mbc5-rumble-ram"_sv, 0x1D},
            {"mbc5-rumble-ram-battery"_sv, 0x1E},
            {"mmm01"_sv, 0x0B},
            {"camera"_sv, 0xFC},
            {"tama5"_sv, 0xFD},
            {"huc3"_sv, 0xFE},
            {"huc1"_sv, 0xFF},
        };
    }

    GameBoyFormat::GameBoyFormat() {}
    GameBoyFormat::~GameBoyFormat() {}

    bool GameBoyFormat::generate(Report* report, StringView outputName, const Config& config, const std::vector<std::unique_ptr<Bank>>& banks, std::vector<std::uint8_t>& data) {
        static_cast<void>(outputName);
        
        // http://problemkaputt.de/pandocs.htm#thecartridgeheader

        for (const auto& bank : banks) {
            bank->exportRom(data);
        }

        if (data.size() < RomBankSize) {
            data.resize(RomBankSize, 0xFF);
        }

        memset(&data[0x134], 0, 0x14D - 0x134);
        memcpy(&data[0x104], LogoBitmap, sizeof(LogoBitmap));
        data[0x14B] = 0x33;

        std::size_t titleMaxLength = config.has("manufacturer"_sv) ? 11 : 15;

        if (const auto title = config.checkFixedString(report, "title"_sv, titleMaxLength, false)) {
            memcpy(&data[0x134], title->second.getData(), title->second.getLength());
        } else {
            auto truncatedOutputName = path::stripExtension(outputName).sub(0, titleMaxLength).toString();
            std::transform(truncatedOutputName.begin(), truncatedOutputName.end(), truncatedOutputName.begin(), [](unsigned char c) { return std::toupper(c); });
            memcpy(&data[0x134], truncatedOutputName.data(), truncatedOutputName.length());
        }

        if (const auto manufacturer = config.checkFixedString(report, "manufacturer"_sv, 4, false)) {
            memcpy(&data[0x13F], manufacturer->second.getData(), manufacturer->second.getLength());
        }
        if (const auto gbcCompatible = config.checkBoolean(report, "gbc_compatible"_sv, false)) {
            if (gbcCompatible->second) {
                data[0x143] = 0x80;
            }
        }
        if (const auto gbcExclusive = config.checkBoolean(report, "gbc_exclusive"_sv, false)) {
            if (gbcExclusive->second) {
                data[0x143] = 0xC0;
            }
        }
        if (const auto licensee = config.checkFixedString(report, "licensee"_sv, 2, false)) {
            memcpy(&data[0x144], licensee->second.getData(), licensee->second.getLength());
        }
        if (const auto sgbCompatible = config.checkBoolean(report, "sgb_compatible"_sv, false)) {
            if (sgbCompatible->second) {
                data[0x146] = 0x03;
            }
        }
        if (const auto cartType = config.checkString(report, "cart_type"_sv, false)) {
            const auto match = cartTypes.find(cartType->second);
            if (match != cartTypes.end()) {
                data[0x147] = static_cast<std::uint8_t>(match->second);
            } else {
                report->error("`cart_type` of \"" + cartType->second.toString() + "\" is not supported", cartType->first->location);
            }
        }
        if (const auto cartTypeID = config.checkInteger(report, "cart_type_id"_sv, false)) {
            data[0x147] = static_cast<std::uint8_t>(cartTypeID->second);
        }
        if (const auto ramSize = config.checkInteger(report, "ram_size"_sv, false)) {
            const auto value = ramSize->second;
            
            std::uint8_t setting = 0x00;
            if (value > Int128(32 * 1024)) {
                report->error("`ram_size` of " + value.toString() + " is too large (max is 32 KiB)", ramSize->first->location);
            } else if (Int128(8 * 1024) < value && value <= Int128(32 * 1024)) {
                setting = 0x03;
            } else if (Int128(2 * 1024) < value && value <= Int128(8 * 1024)) {
                setting = 0x02;
            } else if (Int128(0) < value && value <= Int128(2 * 1024)) {
                setting = 0x01;
            }

            data[0x149] = setting;
        }
        if (const auto international = config.checkBoolean(report, "international"_sv, false)) {
            if (international->second) {
                data[0x14A] = 0x01;
            }
        }
        if (const auto oldLicensee = config.checkInteger(report, "old_licensee"_sv, false)) {
            data[0x14B] = static_cast<std::uint8_t>(oldLicensee->second);
        }
        if (const auto version = config.checkInteger(report, "version"_sv, false)) {
            data[0x14C] = static_cast<std::uint8_t>(version->second);
        }

        // Round size up to nearest power-of-two.
        auto logDataSize = log2(data.size());
        if (data.size() > (1U << logDataSize)) {
            ++logDataSize;
            data.resize(1U << logDataSize, 0xFF);
        }

        if (data.size() <= MaxTotalRomSize) {
            data[0x148] = static_cast<std::uint8_t>(logDataSize - log2(RomBankSize));
        } else {
            report->error("rom size of " + std::to_string(data.size()) + " bytes is too large (max is " + std::to_string(MaxTotalRomSize) + " bytes)", SourceLocation());
            return false;
        }

        std::uint8_t headerChecksum = 0;
        for(std::size_t i = 0x134; i != 0x14D; ++i) {
            headerChecksum = (headerChecksum - data[i] - 1) & 0xFF;
        }
        data[0x14D] = headerChecksum;

        std::uint16_t globalChecksum = 0;
        for (std::size_t i = 0, size = data.size(); i != size; ++i) {
            if (i != 0x14E && i != 0x14F) {
                globalChecksum += data[i];
            }
        }
        data[0x14E] = (globalChecksum >> 8) & 0xFF;
        data[0x14F] = globalChecksum & 0xFF;

        return true;
    }
}