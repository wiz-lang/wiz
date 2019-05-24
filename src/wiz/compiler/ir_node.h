#ifndef WIZ_COMPILER_IR_NODE_H
#define WIZ_COMPILER_IR_NODE_H

#include <cstdint>
#include <vector>
#include <type_traits>

#include <wiz/ast/expression.h>
#include <wiz/compiler/instruction.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    struct Definition;
    class SymbolTable;

    struct IrNode {
        struct PushRelocation {
            PushRelocation(
                Bank* bank,
                Optional<std::size_t> address)
            : bank(bank),
            address(address) {}

            Bank* bank;
            Optional<std::size_t> address;
        };

        struct PopRelocation {};

        struct Label {
            Label(
                Definition* definition)
            : definition(definition) {}

            Definition* definition;
        };

        struct Code {
            Code(
                const Instruction* instruction,
                std::vector<InstructionOperandRoot> operandRoots)
            : instruction(instruction),
            operandRoots(std::move(operandRoots)) {}

            const Instruction* instruction;
            std::vector<InstructionOperandRoot> operandRoots;
        };

        struct Var {
            Var(
                Definition* definition)
            : definition(definition) {}

            Definition* definition;
        };

        template <typename T>
        IrNode(
            T&& variant,
            SourceLocation sourceLocation)
        : variant(std::forward<T>(variant)),
        location(sourceLocation) {}

        using VariantType = Variant<
            PushRelocation,
            PopRelocation,
            Label,
            Code,
            Var
        >;

        VariantType variant;
        SourceLocation location;
    };
}

#endif