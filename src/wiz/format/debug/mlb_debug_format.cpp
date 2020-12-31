#include <algorithm>

#include <wiz/compiler/definition.h>
#include <wiz/compiler/symbol_table.h>
#include <wiz/format/output/output_format.h>
#include <wiz/format/debug/mlb_debug_format.h>
#include <wiz/utility/misc.h>
#include <wiz/utility/path.h>
#include <wiz/utility/text.h>
#include <wiz/utility/writer.h>
#include <wiz/utility/resource_manager.h>

namespace wiz {
    namespace {
        bool isLabelOutputRelative(const Definition* definition) {
            switch (definition->kind) {
                case DefinitionKind::Func:
                    return true;
                case DefinitionKind::Var: {
                    const auto address = definition->getAddress();

                    if (address->absolutePosition.hasValue()) {
                        if (address->absolutePosition.get() < 0x800) {
                            return false;
                        } else if (address->absolutePosition.get() >= 0x6000 && address->absolutePosition.get() < 0x8000) {
                            return false;
                        } else if ((definition->var.qualifiers & Qualifiers::Extern) != Qualifiers::None || !address->relativePosition.hasValue()) {
                            return false;
                        } else if ((definition->var.qualifiers & Qualifiers::Const) != Qualifiers::None) {
                            return true;
                        }
                    }
                    break;
                }
                default: break;
            }

            return false;
        }

        StringView getLabelTypes(const Definition* definition) {
            switch (definition->kind) {
                case DefinitionKind::Func:
                    return "P"_sv;
                case DefinitionKind::Var: {
                    const auto address = definition->getAddress();

                    if (address->absolutePosition.hasValue()) {
                        if (address->absolutePosition.get() < 0x800) {
                            return "R"_sv;
                        } else if (address->absolutePosition.get() >= 0x6000 && address->absolutePosition.get() < 0x8000) {
                            return "SW"_sv;
                        } else if ((definition->var.qualifiers & Qualifiers::Extern) != Qualifiers::None || !address->relativePosition.hasValue()) {
                            return "G"_sv;
                        } else if ((definition->var.qualifiers & Qualifiers::Const) != Qualifiers::None) {
                            return "P"_sv;
                        }
                    }
                    break;
                }
                default: break;
            }

            return ""_sv;
        }

        void dumpAddress(Writer* writer, const Definition* definition, DebugFormatContext& context) {
            const auto outputContext = context.outputContext;
            const auto address = definition->getAddress();

            if (address.hasValue()) {
                if (address->absolutePosition.hasValue()) {
                    // Ignore hardware registers / 'extern'.
                    // If these are mapper-related, definitions for different mappers might alias each other.
                    // Also, mapper-related registers might alias code/data in the ROM area of address space.
                    const auto labelTypes = getLabelTypes(definition);
                    if (labelTypes == "G"_sv) {
                        return;
                    }

                    const auto outputRelative = isLabelOutputRelative(definition);

                    Optional<std::size_t> addressValue;
                    if (outputRelative) {
                        const auto offset = outputContext->getOutputOffset(address.get());
                        if (offset.hasValue()) {
                            addressValue = offset.get() - 16;
                        }
                    } else {
                        addressValue = address->absolutePosition.get();
                    }

                    if (!addressValue.hasValue()) {
                        return;
                    }

                    const auto addressString = toHexString(addressValue.get());

                    std::string fullName;
                    if (definition->parentScope != nullptr) {
                        const auto prefix = definition->parentScope->getFullName();
                        if (!prefix.empty()) {
                            fullName = prefix;
                            fullName += '.';
                        }
                    }
                    fullName += text::replaceAll(definition->name.toString(), "$", "__");

                    for (const auto& labelType : labelTypes) {
                        writer->write(StringView(&labelType, 1));
                        writer->write(":"_sv);
                        writer->write(StringView(addressString));
                        writer->write(":"_sv);
                        writer->writeLine(StringView(fullName));
                    }
                }
            }
        }

    }

    MlbDebugFormat::MlbDebugFormat() {}
    MlbDebugFormat::~MlbDebugFormat() {}

    bool MlbDebugFormat::generate(DebugFormatContext& context) {
        const auto debugName = path::stripExtension(context.outputName).toString() + ".mlb";

        if (auto writer = context.resourceManager->openWriter(StringView(debugName))) {
            for (auto& definition : context.definitions) {
                dumpAddress(writer.get(), definition, context);
            }
        }       

        return true;
    }
}