#include <wiz/ast/expression.h>
#include <wiz/compiler/config.h>
#include <wiz/format/binary_format.h>

namespace wiz {
    BinaryFormat::BinaryFormat() {}
    BinaryFormat::~BinaryFormat() {}

    bool BinaryFormat::generate(Report* report, StringView outputName, const Config& config, ArrayView<UniquePtr<Bank>> banks, std::vector<std::uint8_t>& data) {
        static_cast<void>(outputName);
        
        const auto trim = config.checkBoolean(report, "trim"_sv, false);
        std::size_t trimmedBankIndex = SIZE_MAX;

        if (trim) {
            for (std::size_t i = banks.size() - 1; i < banks.size(); --i) {
                if (isBankKindStored(banks[i]->getKind())) {
                    trimmedBankIndex = i;
                }
            }
        }

        for (std::size_t i = 0; i != banks.size(); ++i) {
            const auto& bank = banks[i];            
            const auto bankData = trimmedBankIndex == i ? bank->getUsedData() : bank->getData();
            data.reserve(data.size() + bankData.size());
            data.insert(data.end(), bankData.begin(), bankData.end());
        }

        return true;
    }
}