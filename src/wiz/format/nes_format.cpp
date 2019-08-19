#include <algorithm>

#include <wiz/ast/expression.h>
#include <wiz/compiler/config.h>
#include <wiz/format/nes_format.h>

namespace wiz {
    namespace {
        const std::size_t HeaderSize = 16;
        const std::size_t PrgRomBankSize = 16 * 1024U;
        const std::size_t ChrRomBankSize = 8 * 1024U;
        const std::size_t PrgRamBankSize = 8 * 1024U;

        const StringView HeaderSignature("NES\x1A");

        std::unordered_map<StringView, std::uint8_t> cartTypes {
            {"nrom"_sv, 0},
            {"sxrom"_sv, 1},
            {"mmc1"_sv, 1},
            {"uxrom"_sv, 2},
            {"cnrom"_sv, 3},
            {"txrom"_sv, 4},
            {"mmc3"_sv, 4},
            {"mmc6"_sv, 4},
            {"exrom"_sv, 5},
            {"mmc5"_sv, 5},
            {"axrom"_sv, 7},
            {"pxrom"_sv, 9},
            {"mmc2"_sv, 9},
            {"fxrom"_sv, 10},
            {"mmc4"_sv, 10},
            {"color-dreams"_sv, 11},
            {"cprom"_sv, 13},
            {"24c02"_sv, 16},
            {"ss8806"_sv, 18},
            {"n163"_sv, 19},
            {"vrc4a"_sv, 21},
            {"vrc4c"_sv, 21},
            {"vrc2a"_sv, 22},
            {"vrc2b"_sv, 23},
            {"vrc4e"_sv, 23},
            {"vrc6a"_sv, 24},
            {"vrc4b"_sv, 25},
            {"vrc4d"_sv, 25},
            {"vrc6b"_sv, 26},
            {"action-53"_sv, 28},
            {"unrom-512"_sv, 30},
            {"bnrom"_sv, 34},
            {"rambo1"_sv, 64},
            {"gxrom"_sv, 66},
            {"mxrom"_sv, 66},
            {"after-burner"_sv, 68},
            {"fme7"_sv, 69},
            {"sunsoft5b"_sv, 69},
            {"codemasters"_sv, 71},
            {"vrc3"_sv, 73},
            {"vrc1"_sv, 75},
            {"n109"_sv, 79},
            {"vrc7"_sv, 85},
            {"gtrom"_sv, 111},
            {"txsrom"_sv, 118},
            {"tqrom"_sv, 119},
            {"24c01"_sv, 159},
            {"dxrom"_sv, 206},
            {"n118"_sv, 206},
            {"n175"_sv, 210},
            {"n340"_sv, 210},
            {"action52"_sv, 228},
            {"codemasters-quattro"_sv, 232},
        };
    }

    NesFormat::NesFormat() {}
    NesFormat::~NesFormat() {}

    bool NesFormat::generate(Report* report, StringView outputName, const Config& config, ArrayView<const Bank*> banks, FormatOutput& output) {
        static_cast<void>(outputName);
        
        // https://wiki.nesdev.com/w/index.php/INES
        // https://wiki.nesdev.com/w/index.php/NES_2.0

        auto& data = output.data;

        data.insert(data.end(), HeaderSize, 0);

        for (const auto& bank : banks) {
            if (isBankKindStored(bank->getKind()) && bank->getKind() != BankKind::CharacterRom) {
                const auto bankData = bank->getData();

                output.bankOffsets[bank] = data.size();
                data.reserve(data.size() + bankData.size());
                data.insert(data.end(), bankData.begin(), bankData.end());
            }
        }

        std::size_t prgSize = data.size() - HeaderSize;
        std::size_t paddedPrgSize = (prgSize + PrgRomBankSize - 1) / PrgRomBankSize * PrgRomBankSize;

        if (prgSize < paddedPrgSize) {
            data.resize(paddedPrgSize + HeaderSize, 0xFF);
            prgSize = paddedPrgSize;
        }

        for (const auto& bank : banks) {
            if (isBankKindStored(bank->getKind()) && bank->getKind() == BankKind::CharacterRom) {
                const auto bankData = bank->getData();

                output.bankOffsets[bank] = data.size();
                data.reserve(data.size() + bankData.size());
                data.insert(data.end(), bankData.begin(), bankData.end());
            }
        }

        std::size_t chrSize = data.size() - prgSize - HeaderSize;
        std::size_t paddedChrSize = (chrSize + ChrRomBankSize - 1) / ChrRomBankSize * ChrRomBankSize;

        if (chrSize < paddedChrSize) {
            data.resize(paddedChrSize + prgSize + HeaderSize, 0xFF);
            chrSize = paddedChrSize;
        }

        memcpy(&data[0], HeaderSignature.getData(), HeaderSignature.getLength());
        data[4] = static_cast<std::uint8_t>(prgSize / PrgRomBankSize);
        data[5] = static_cast<std::uint8_t>(chrSize / ChrRomBankSize);

        std::uint8_t mapper = 0;
        if (const auto cartType = config.checkString(report, "cart_type"_sv, false)) {
            const auto match = cartTypes.find(cartType->second);
            if (match != cartTypes.end()) {
                mapper = match->second;
            } else {
                report->error("`cart_type` of \"" + cartType->second.toString() + "\" is not supported", cartType->first->location);
            }
        }
        if (const auto cartTypeID = config.checkInteger(report, "cart_type_id"_sv, false)) {
            mapper = static_cast<std::uint8_t>(cartTypeID->second);
        }

        data[6] = (mapper & 0xFF) << 4;
        data[7] = ((mapper & 0xFF) >> 4) << 4;        

        if (const auto verticalMirror = config.checkBoolean(report, "vertical_mirror"_sv, false)) {
            if (verticalMirror->second) {
                data[6] |= 0x01;
            }
        }
        if (const auto battery = config.checkBoolean(report, "battery"_sv, false)) {
            if (battery->second) {
                data[6] |= 0x02;
            }
        }
        if (const auto fourScreen = config.checkBoolean(report, "four_screen"_sv, false)) {
            if (fourScreen->second) {
                data[6] |= 0x08;
            }
        }
        if (const auto prgRamSize = config.checkInteger(report, "prg_ram_size"_sv, false)) {
            if (prgRamSize->second >= Int128(PrgRamBankSize * 255)) {
                report->error("`prg_ram_size` of " + prgRamSize->second.toString() + " is too big (must be no more than " + std::to_string(PrgRamBankSize * 255) + " bytes)", prgRamSize->first->location);
            } else {
                const auto value = static_cast<std::size_t>(prgRamSize->second);
                if (value % PrgRamBankSize != 0) {
                    report->error("`prg_ram_size` of " + std::to_string(value) + " is not supported (must be divisible by " + std::to_string(PrgRamBankSize) + " bytes)", prgRamSize->first->location);
                } else {
                    data[8] = static_cast<std::uint8_t>(value / PrgRamBankSize);
                }
            }
        }

        return true;
    }
}