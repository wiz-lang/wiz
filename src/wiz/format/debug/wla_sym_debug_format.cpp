#include <algorithm>

#include <wiz/compiler/definition.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/format/output/output_format.h>
#include <wiz/format/debug/wla_sym_debug_format.h>
#include <wiz/utility/misc.h>
#include <wiz/utility/path.h>
#include <wiz/utility/text.h>
#include <wiz/utility/writer.h>
#include <wiz/utility/resource_manager.h>

// See: https://wla-dx.readthedocs.io/en/latest/symbols.html
// TODO: support [source files] and [addr-to-line mapping] functionality.
namespace wiz {
    namespace {
        bool isDebugLabelOutputRelative(const Definition* definition) {
            switch (definition->kind) {
                case DefinitionKind::Func:
                    return true;
                case DefinitionKind::Var: {
                    const auto address = definition->getAddress();

                    if (address->absolutePosition.hasValue()) {
                        if ((definition->var.qualifiers & Qualifiers::Const) != Qualifiers::None) {
                            return true;
                        } else {
                            return false;
                        }
                    }
                    break;
                }
                default: break;
            }

            return false;
        }

        bool isSnesRelatedFormat(DebugFormatContext& context) {
            return context.outputContext->formatName == "sfc"_sv
                || context.outputContext->formatName == "smc"_sv;
        }

        void dumpAddress(Writer* writer, const Definition* definition, DebugFormatContext& context) {
            const auto outputContext = context.outputContext;
            const auto address = definition->getAddress();

            if (address.hasValue()) {
                if (address->absolutePosition.hasValue()) {
                    const auto snes = isSnesRelatedFormat(context);
                    const auto outputRelative = !snes && isDebugLabelOutputRelative(definition);

                    // Ignore hardware registers / 'extern'.
                    // If these are mapper-related, definitions for different mappers might alias each other.
                    // Also, mapper-related registers might alias code/data in the ROM area of address space.
                    if ((definition->var.qualifiers & Qualifiers::Extern) != Qualifiers::None
                    || !address->relativePosition.hasValue()) {
                        return;
                    }

                    Optional<std::size_t> maybeValue;
                    if (outputRelative) {
                        const auto offset = outputContext->getOutputOffset(address.get());
                        if (offset.hasValue()) {
                            maybeValue = std::max<std::size_t>(offset.get() - outputContext->fileHeaderPrefixSize, 0);
                        }
                    } else {
                        maybeValue = address->absolutePosition.get();
                    }

                    if (!maybeValue.hasValue()) {
                        return;
                    }

                    const auto value = maybeValue.get();
                    const auto match = context.addressOwnership.find(value);
                    if (match != context.addressOwnership.end()) {
                        return;
                    } else {
                        context.addressOwnership[value] = definition;
                    }

                    // Bank index comes from:
                    // - the output-relative offset divided by debugBankSize (for output-relative definitions)
                    // - upper 8-bits of the absolute absolute (for absolute definitions).
                    // Meanwhile, the lower 16-bit part of the address comes from source-provided absolute address, so that
                    // 00:0000, 01:1000, 02:1000, etc give expected results on GB.
                    const auto bankIndex = outputRelative ? value / outputContext->debugBankSize : value >> 16U;
                    auto addressString = text::padLeft(toHexString(bankIndex & 0xFF), '0', 2);
                    addressString += ':';
                    addressString += text::padLeft(toHexString(address->absolutePosition.get() & 0xFFFF), '0', 4);

                    std::string fullName;
                    if ((definition->name.getLength() == 0 || definition->name[0] != '$')
                    && definition->parentScope != nullptr) {
                        const auto prefix = definition->parentScope->getFullName();
                        if (!prefix.empty()) {
                            fullName = prefix;
                            fullName += '.';
                        }
                    }
                    fullName += definition->name.toString();
                    
                    fullName = text::replaceAll(fullName, "$", "__");
                    fullName = text::replaceAll(fullName, "%", "__");

                    writer->write(StringView(addressString));
                    writer->write(" "_sv);
                    writer->writeLine(StringView(fullName));
                }
            }
        }
    }

    WlaSymDebugFormat::WlaSymDebugFormat() {}
    WlaSymDebugFormat::~WlaSymDebugFormat() {}

    bool WlaSymDebugFormat::generate(DebugFormatContext& context) {
        const auto debugName = path::stripExtension(context.outputName).toString() + ".sym";

        if (auto writer = context.resourceManager->openWriter(StringView(debugName))) {
            writer->writeLine("[labels]"_sv);
            for (const auto& definition : context.definitions) {
                dumpAddress(writer.get(), definition, context);
            }
        }       

        return true;
    }
}