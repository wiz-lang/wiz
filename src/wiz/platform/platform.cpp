#include <cassert>
#include <wiz/platform/platform.h>
#include <wiz/platform/mos6502_platform.h>
#include <wiz/platform/z80_platform.h>
#include <wiz/platform/gb_platform.h>
#include <wiz/platform/wdc65816_platform.h>
#include <wiz/platform/spc700_platform.h>

namespace wiz {
    PlatformCollection::PlatformCollection() {
        addPlatform("6502"_sv, std::make_unique<Mos6502Platform>(Mos6502Platform::Revision::Base6502));
        addPlatform("65c02"_sv, std::make_unique<Mos6502Platform>(Mos6502Platform::Revision::Base65C02));
        addPlatform("rockwell65c02"_sv, std::make_unique<Mos6502Platform>(Mos6502Platform::Revision::Rockwell65C02));
        addPlatform("wdc65c02"_sv, std::make_unique<Mos6502Platform>(Mos6502Platform::Revision::Wdc65C02));
        addPlatform("huc6280"_sv, std::make_unique<Mos6502Platform>(Mos6502Platform::Revision::Huc6280));
        addPlatform("z80"_sv, std::make_unique<Z80Platform>());
        addPlatform("gb"_sv, std::make_unique<GameBoyPlatform>());
        addPlatform("wdc65816"_sv, std::make_unique<Wdc65816Platform>());
        addPlatform("spc700"_sv, std::make_unique<Spc700Platform>());

        addFileExtension("nes"_sv, "6502"_sv);
        addFileExtension("a26"_sv, "6502"_sv);
        addFileExtension("pce"_sv, "huc6280"_sv);
        addFileExtension("sms"_sv, "z80"_sv);
        addFileExtension("gg"_sv, "z80"_sv);
        addFileExtension("gb"_sv, "gb"_sv);
        addFileExtension("smc"_sv, "wdc65816"_sv);
        addFileExtension("sfc"_sv, "wdc65816"_sv);
    }

    PlatformCollection::~PlatformCollection() {}

    std::size_t PlatformCollection::getPlatformCount() const {
        return platforms.size();
    }

    Platform* PlatformCollection::getPlatform(std::size_t index) const {
        return platforms[index].get();
    }

    std::size_t PlatformCollection::getPlatformNameCount() const {
        return platformNames.size();
    }

    StringView PlatformCollection::getPlatformName(std::size_t index) const {
        return platformNames[index];
    }

    void PlatformCollection::addPlatform(StringView name, std::unique_ptr<Platform> platform) {
        const auto ptr = platform.get();
        platforms.push_back(std::move(platform));
        platformNames.push_back(name);
        platformsByName[name] = ptr;
    }

    void PlatformCollection::addFileExtension(StringView fileExtension, StringView platformName) {
        if (const auto platform = findByName(platformName)) {
            platformsByFileExtension[fileExtension] = platform;
        } else {
            assert(false);
        }
    }

    void PlatformCollection::addPlatformAlias(StringView aliasName, StringView originalName) {
        if (const auto platform = findByName(originalName)) {
            platformNames.push_back(aliasName);
            platformsByName[aliasName] = platform;
        } else {
            assert(false);
        }
    }

    Platform* PlatformCollection::findByName(StringView name) const {
        const auto match = platformsByName.find(name);
        if (match != platformsByName.end()) {
            return match->second;
        }
        return nullptr;
    }

    Platform* PlatformCollection::findByFileExtension(StringView extension) const {
        const auto match = platformsByFileExtension.find(extension);
        if (match != platformsByFileExtension.end()) {
            return match->second;
        }
        return nullptr;
    }

}

