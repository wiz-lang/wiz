#include <wiz/ast/expression.h>
#include <wiz/compiler/config.h>
#include <wiz/format/sms_format.h>

namespace wiz {
    namespace {
        const std::size_t HeaderSize = 16;
        const StringView HeaderSignature("TMR SEGA  ");
    }

    SmsFormat::SmsFormat(SystemType systemType)
    : systemType(systemType) {}

    SmsFormat::~SmsFormat() {}

    bool SmsFormat::generate(Report* report, StringView outputName, const Config& config, ArrayView<UniquePtr<Bank>> banks, std::vector<std::uint8_t>& data) {
        static_cast<void>(outputName);
        
        // http://www.smspower.org/Development/ROMHeader

        for (const auto& bank : banks) {
            const auto bankData = bank->getData();
            data.reserve(data.size() + bankData.size());
            data.insert(data.end(), bankData.begin(), bankData.end());
        }

        std::size_t headerAddress = 0x1FF0;
        std::uint8_t checksumRomSizeSetting = 0xA;

        if (data.size() < 0x2000) {
            data.resize(0x2000, 0xFF);
        } else if (data.size() > 0x2000 && data.size() <= 0x4000) {
            headerAddress = 0x3FF0;
            checksumRomSizeSetting = 0xB;

            data.resize(0x4000, 0xFF);
        } else if (data.size() > 0x4000) {
            headerAddress = 0x7FF0;
            checksumRomSizeSetting = 0xC;

            if (data.size() < 0x8000) {
                data.resize(0x8000, 0xFF);
            }
        }

        memset(&data[headerAddress], 0, 0x10);
        memcpy(&data[headerAddress], HeaderSignature.getData(), HeaderSignature.getLength());

        if (const auto productCode = config.checkInteger(report, "product_code"_sv, false)) {
            if (Int128(0) <= productCode->second && productCode->second <= Int128(159999)) {
                std::uint32_t value = static_cast<std::uint32_t>(productCode->second);

                data[headerAddress + 0xC] |= (value % 10);
                value /= 10;
                data[headerAddress + 0xC] |= (value % 10) << 4;
                value /= 10;
                data[headerAddress + 0xD] |= (value % 10);
                value /= 10;
                data[headerAddress + 0xD] |= (value % 10) << 4;
                value /= 10;
                data[headerAddress + 0xE] |= (value & 0xF) << 4;
            } else {
                report->error("`product_code` of " + productCode->second.toString() + " is invalid (must be between 0 and 159999)", productCode->first->location);
            }
        }
        if (const auto version = config.checkInteger(report, "version"_sv, false)) {
            if (Int128(0) <= version->second && version->second <= Int128(0xF)) {
                data[headerAddress + 0xE] |= static_cast<std::uint8_t>(version->second);
            } else {
                report->error("`version` of " + version->second.toString() + " is invalid (must be between 0 and 15)", version->first->location);
            }
        }

        data[headerAddress + 0xF] = 0x40 | checksumRomSizeSetting;
        if (const auto region = config.checkString(report, "region"_sv, false)) {
            const auto value = region->second;

            std::uint8_t setting = systemType == SystemType::MasterSystem ? 0x04 : 0x07;
            if (value == "japan"_sv) {
                setting = systemType == SystemType::MasterSystem ? 0x03 : 0x05;
            } else if (value == "export"_sv) {
                setting = systemType == SystemType::MasterSystem ? 0x04 : 0x06;
            } else if (value == "international"_sv) {
                setting = systemType == SystemType::MasterSystem ? 0x04 : 0x07;
            } else {
                report->error("`region` of " + region->second.toString() + " is invalid (must be \"japan\", \"export\", or \"international\")", region->first->location);
            }

            data[headerAddress + 0xF] &= ~0xF0;
            data[headerAddress + 0xF] |= setting << 4;
        }

        std::uint16_t checksum = 0;
        for (std::size_t i = 0, size = data.size(); i != size; ++i) {
            if (i < headerAddress || i >= headerAddress + HeaderSize) {
                checksum += data[i];
            }
        }
        data[headerAddress + 0xA] = (checksum >> 8) & 0xFF;
        data[headerAddress + 0xB] = checksum & 0xFF;

        return true;
    }
}