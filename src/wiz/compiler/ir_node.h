#ifndef WIZ_COMPILER_IR_NODE_H
#define WIZ_COMPILER_IR_NODE_H

#include <cstdint>
#include <vector>
#include <type_traits>

#include <wiz/ast/expression.h>
#include <wiz/compiler/instruction.h>
#include <wiz/utility/macros.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    struct Definition;
    class SymbolTable;

    enum class IrNodeKind {
        PushRelocation,
        PopRelocation,
        Label,
        Code,
        Var,
    };

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

        IrNode(
            PushRelocation pushRelocation,
            SourceLocation sourceLocation)
        : kind(IrNodeKind::PushRelocation),
        pushRelocation(std::move(pushRelocation)),
        location(sourceLocation) {}

        IrNode(
            PopRelocation popRelocation,
            SourceLocation sourceLocation)
        : kind(IrNodeKind::PopRelocation),
        popRelocation(std::move(popRelocation)),
        location(sourceLocation) {}

        IrNode(
            Label label,
            SourceLocation sourceLocation)
        : kind(IrNodeKind::Label),
        label(std::move(label)),
        location(sourceLocation) {}

        IrNode(
            Code code,
            SourceLocation sourceLocation)
        : kind(IrNodeKind::Code),
        code(std::move(code)),
        location(sourceLocation) {}

        IrNode(
            Var var,
            SourceLocation sourceLocation)
        : kind(IrNodeKind::Var),
        var(std::move(var)),
        location(sourceLocation) {}

        ~IrNode();

        template <typename T> const T* tryGet() const;

        IrNodeKind kind;

        union {
            PushRelocation pushRelocation;
            PopRelocation popRelocation;
            Label label;
            Code code;
            Var var;
        };

        SourceLocation location;
    };

    template <> WIZ_FORCE_INLINE const IrNode::PushRelocation* IrNode::tryGet<IrNode::PushRelocation>() const {
        return kind == IrNodeKind::PushRelocation ? &pushRelocation : nullptr;
    }
    template <> WIZ_FORCE_INLINE const IrNode::PopRelocation* IrNode::tryGet<IrNode::PopRelocation>() const {
        return kind == IrNodeKind::PopRelocation ? &popRelocation : nullptr;
    }
    template <> WIZ_FORCE_INLINE const IrNode::Label* IrNode::tryGet<IrNode::Label>() const {
        return kind == IrNodeKind::Label ? &label : nullptr;
    }
    template <> WIZ_FORCE_INLINE const IrNode::Code* IrNode::tryGet<IrNode::Code>() const {
        return kind == IrNodeKind::Code ? &code : nullptr;
    }
    template <> WIZ_FORCE_INLINE const IrNode::Var* IrNode::tryGet<IrNode::Var>() const {
        return kind == IrNodeKind::Var ? &var : nullptr;
    }

}

#endif