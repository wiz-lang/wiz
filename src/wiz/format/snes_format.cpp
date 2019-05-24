#include <algorithm>

#include <wiz/ast/expression.h>
#include <wiz/compiler/config.h>
#include <wiz/format/snes_format.h>
#include <wiz/utility/misc.h>

namespace wiz {
    namespace {
        const std::size_t SnesHeaderSize = 0x30;
        const std::size_t SnesTitleMaxLength = 21;
        const std::size_t MinRomSize = 128 * 1024;
        //const std::size_t RomBankSize = 32 * 1024;
        const std::size_t MaxTotalRomSize = 8 * 1024U * 1024U;

        enum class MapMode {
            LoRom,
            HiRom,
            LoRomSa1,
            LoRomSdd1,
            ExHiRom,
            ExHiRomSpc7110,
        };

        struct MapModeInfo {
            MapModeInfo(
                std::uint8_t value,
                std::size_t headerOffset)
            : value(value),
            headerOffset(headerOffset) {}

            std::uint8_t value;
            std::size_t headerOffset;
        };

        std::unordered_map<StringView, MapModeInfo> mapModes {
            {"lorom"_sv, MapModeInfo(0x20, 0x7F00)},
            {"hirom"_sv, MapModeInfo(0x21, 0xFF00)},
            {"sa1"_sv, MapModeInfo(0x23, 0x7F00)},
            {"sdd1"_sv, MapModeInfo(0x22, 0x7F00)},
            {"exhirom"_sv, MapModeInfo(0x25, 0x40FF00)},
            {"spc7110"_sv, MapModeInfo(0x2A, 0x40FF00)}
        };

        std::unordered_map<StringView, std::uint8_t> expansionSettings {
            {"none"_sv, 0x00},
            {"dsp"_sv, 0x03},
            {"super-fx"_sv, 0x23},
            {"obc1"_sv, 0x23},
            {"sa1"_sv, 0x33},
            {"other"_sv, 0xE3},
            {"custom"_sv, 0xF3}
        };

        std::unordered_map<StringView, std::uint8_t> regionSettings {
            {"ntsc"_sv, 0x01},
            {"pal"_sv, 0x02},
            {"japanese"_sv, 0x00},
            {"american"_sv, 0x01},
            {"european"_sv, 0x02},
            {"scandinavian"_sv, 0x03},
            {"french"_sv, 0x06},
            {"dutch"_sv, 0x07},
            {"spanish"_sv, 0x08},
            {"german"_sv, 0x09},
            {"italian"_sv, 0x0A},
            {"chinese"_sv, 0x0B},
            {"korean"_sv, 0x0D},
            {"canadian"_sv, 0x0F},
            {"brazilian"_sv, 0x10},
            {"australian"_sv, 0x11},
        };

        const std::size_t SmcHeaderSize = 0x200;
        const std::size_t SmcRomBlockSize = 8192;
    }

    SnesFormat::SnesFormat() {}
    SnesFormat::~SnesFormat() {}

    bool SnesFormat::generate(Report* report, StringView outputName, const Config& config, const std::vector<std::unique_ptr<Bank>>& banks, std::vector<std::uint8_t>& data) {
        static_cast<void>(outputName);

        // http://old.smwiki.net/wiki/Internal_ROM_Header
        // https://github.com/gilligan/snesdev/blob/master/docs/fullsnes.txt
        // https://en.wikibooks.org/wiki/Super_NES_Programming/SNES_memory_map#The_SNES_header

        for (const auto& bank : banks) {
            bank->exportRom(data);
        }
        
        std::uint8_t mapModeSetting = 0x20;
        std::size_t headerAddress = 0x7F00;
        if (const auto mapModeName = config.checkString(report, "map_mode"_sv, false)) {
            const auto match = mapModes.find(mapModeName->second);
            if (match != mapModes.end()) {
                const auto& mapModeInfo = match->second;
                mapModeSetting = mapModeInfo.value;
                headerAddress = mapModeInfo.headerOffset;
            } else {
                report->error("`map_mode` of \"" + mapModeName->second.toString() + "\" is not supported", mapModeName->first->location);
            }
        }
        if (const auto fastrom = config.checkBoolean(report, "fastrom"_sv, false)) {
            if (fastrom->second) {
                mapModeSetting |= 0x10;
            }
        }

        std::size_t minRomSize = std::max(headerAddress + 0x100, MinRomSize);
        if (data.size() < minRomSize) {
            data.resize(minRomSize, 0xFF);
        }

        memset(&data[headerAddress + 0xB0], 0, SnesHeaderSize);
        memset(&data[headerAddress + 0xC0], ' ', SnesTitleMaxLength);
        data[headerAddress + 0xD6] = mapModeSetting;
        data[headerAddress + 0xDA] = 0x33;
        data[headerAddress + 0xDC] = 0xFF;
        data[headerAddress + 0xDD] = 0xFF;

        if (const auto makerCode = config.checkFixedString(report, "maker_code"_sv, 2, false)) {
            memcpy(&data[headerAddress + 0xB0], makerCode->second.getData(), makerCode->second.getLength());
        }
        if (const auto gameCode = config.checkFixedString(report, "game_code"_sv, 4, false)) {
            memcpy(&data[headerAddress + 0xB2], gameCode->second.getData(), gameCode->second.getLength());
        }
        if (const auto expansionRamSize = config.checkInteger(report, "expansion_ram_size"_sv, false)) {
            const auto value = static_cast<std::size_t>(expansionRamSize->second);
            if (value != 0) {
                auto logValue = log2(value);
                if (value < 4096) {
                    report->error("`expansion_ram_size` of \"" + std::to_string(value) + "\" is not supported (must be at least 4096 bytes)", expansionRamSize->first->location);
                } else if (value > (static_cast<std::size_t>(1) << logValue)) {
                    report->error("`expansion_ram_size` of \"" + std::to_string(value) + "\" is not supported (must be a power-of-two)", expansionRamSize->first->location);
                } else {
                    data[headerAddress + 0xBD] = static_cast<std::uint8_t>(log2(value) - log2(4096));
                }
            }
        }
        if (const auto specialVersion = config.checkInteger(report, "special_version"_sv, false)) {
            data[headerAddress + 0xBE] = static_cast<std::uint8_t>(specialVersion->second);
        }
        if (const auto cartSubType = config.checkInteger(report, "cart_subtype"_sv, false)) {
            data[headerAddress + 0xBF] = static_cast<std::uint8_t>(cartSubType->second);
        }
        if (const auto title = config.checkFixedString(report, "title"_sv, SnesTitleMaxLength, false)) {
            memcpy(&data[headerAddress + 0xC0], title->second.getData(), title->second.getLength());
        }

        {
            std::uint8_t cartTypeLower = 0x00;
            std::uint8_t cartTypeUpper = 0x00;

            if (const auto expansion = config.checkString(report, "expansion_type"_sv, false)) {
                const auto match = expansionSettings.find(expansion->second);
                if (match != expansionSettings.end()) {
                    cartTypeLower = match->second & 0x0F;
                    cartTypeUpper = match->second & 0xF0;
                } else {
                    report->error("`expansion_type` of \"" + expansion->second.toString() + "\" is not supported", expansion->first->location);
                }
            }

            if (const auto ramSize = config.checkInteger(report, "ram_size"_sv, false)) {
                const auto value = static_cast<std::size_t>(ramSize->second);
                if (value != 0) {
                    auto logValue = log2(value);
                    if (value < 4096) {
                        report->error("`ram_size` of \"" + std::to_string(value) + "\" is not supported (must be at least 4096 bytes)", ramSize->first->location);
                    } else if (value > (static_cast<std::size_t>(1) << logValue)) {
                        report->error("`ram_size` of \"" + std::to_string(value) + "\" is not supported (must be a power-of-two)", ramSize->first->location);
                    } else {
                        data[headerAddress + 0xD8] = static_cast<std::uint8_t>(log2(value) - log2(4096));
                        if (cartTypeLower >= 0x03) {
                            cartTypeLower = 0x04;
                        } else {
                            cartTypeLower = 0x04;
                        }
                    }
                }
            }

            if (const auto battery = config.checkBoolean(report, "battery"_sv, false)) {
                if (battery->second) {
                    switch (cartTypeLower) {
                        case 0x00:
                        case 0x01:
                            cartTypeLower = 0x02;
                            break;
                        case 0x03:
                            cartTypeLower = 0x06;
                            break;
                        case 0x04:
                            cartTypeLower = 0x05;
                            break;
                        default: break;
                    }
                }
            }

            data[headerAddress + 0xD4] = cartTypeUpper | cartTypeLower;
        }

        {
            // Round size up to nearest power-of-two.
            auto logDataSize = log2(data.size());
            if (data.size() > (static_cast<std::size_t>(1) << logDataSize)) {
                ++logDataSize;
                data.resize(static_cast<std::size_t>(1) << logDataSize, 0xFF);
            }

            if (data.size() <= MaxTotalRomSize) {
                data[headerAddress + 0xD7] = static_cast<std::uint8_t>(logDataSize - log2(1024));
            } else {
                report->error("rom size of " + std::to_string(data.size()) + " bytes is too large (max is " + std::to_string(MaxTotalRomSize) + " bytes)", SourceLocation());
                return false;
            }
        }

        if (const auto region = config.checkString(report, "region"_sv, false)) {
            const auto match = regionSettings.find(region->second);
            if (match != regionSettings.end()) {
                data[headerAddress + 0xD9] = match->second;
            } else {
                report->error("`region` of \"" + region->second.toString() + "\" is not supported", region->first->location);
            }
        }
        if (const auto version = config.checkInteger(report, "rom_version"_sv, false)) {
            data[headerAddress + 0xDB] = static_cast<std::uint8_t>(version->second);
        }

        {
            const auto dataSize = data.size();
            const auto wholeSize = static_cast<std::size_t>(1) << log2(dataSize);

            std::uint16_t checksum = 0;
            for (std::size_t i = 0; i != wholeSize; ++i) {
                checksum += data[i];
            }

            const auto remainderSize = dataSize - wholeSize;
            if (remainderSize != 0) {
                const auto repeatSize = static_cast<std::size_t>(1) << log2(remainderSize);
                const auto repeatCount = repeatSize != 0 ? remainderSize / repeatSize : 0;

                std::uint16_t repeatChecksum = 0;
                for (std::size_t i = 0; i != repeatSize; ++i) {
                    repeatChecksum += data[wholeSize + i];
                }

                checksum += static_cast<std::uint16_t>(repeatChecksum * repeatCount);
            }

            data[headerAddress + 0xDC] = static_cast<std::uint8_t>(checksum) ^ 0xFF;
            data[headerAddress + 0xDD] = static_cast<std::uint8_t>(checksum >> 8) ^ 0xFF;
            data[headerAddress + 0xDE] = static_cast<std::uint8_t>(checksum);
            data[headerAddress + 0xDF] = static_cast<std::uint8_t>(checksum >> 8);
        }

        return true;
    }

    SnesSmcFormat::SnesSmcFormat() {}
    SnesSmcFormat::~SnesSmcFormat() {}

    bool SnesSmcFormat::generate(Report* report, StringView outputName, const Config& config, const std::vector<std::unique_ptr<Bank>>& banks, std::vector<std::uint8_t>& data) {
        // https://en.wikibooks.org/wiki/Super_NES_Programming/SNES_memory_map#The_SNES_header       

        SnesFormat snesFormat;
        if (!snesFormat.generate(report, outputName, config, banks, data)) {
            return false;
        }

        std::size_t romSize = data.size();
        std::size_t smcRomBlockCount = romSize / SmcRomBlockSize;

        data.insert(data.end(), SmcHeaderSize, 0);
                
        // I can't seem to find any open documentation on other data that is supposed to be in the SMC headers.
        // Let's just put the number of 8K blocks and be done, seems like some SMCs will do this.
        // From what I saw most emulators straight-up ignore this copier header and use the internal SNES header, so let's just do the bare minimum.
        data[0] = static_cast<std::uint8_t>(smcRomBlockCount);
        data[1] = static_cast<std::uint8_t>(smcRomBlockCount >> 8);

        return true;
    }
}