#include <wiz/ast/expression.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/config.h>
#include <wiz/format/output/binary_output_format.h>

namespace wiz {
    BinaryOutputFormat::BinaryOutputFormat() {}
    BinaryOutputFormat::~BinaryOutputFormat() {}

    bool BinaryOutputFormat::generate(OutputFormatContext& context) {
        const auto report = context.report;
        const auto config = context.config;
        const auto& banks = context.banks;
        auto& data = context.data;

        const auto trim = config->checkBoolean(report, "trim"_sv, false);
        std::size_t trimmedBankIndex = SIZE_MAX;

        if (trim) {
            for (std::size_t i = banks.size() - 1; i < banks.size(); --i) {
                if (isBankKindStored(banks[i]->getKind())) {
                    trimmedBankIndex = i;
                    break;
                }
            }
        }

        for (std::size_t i = 0; i != banks.size(); ++i) {
            const auto& bank = banks[i];            
            const auto bankData = trimmedBankIndex == i ? bank->getUsedData() : bank->getData();

            context.bankOffsets[bank] = data.size();
            data.reserve(data.size() + bankData.size());
            data.insert(data.end(), bankData.begin(), bankData.end());
        }

        return true;
    }
}