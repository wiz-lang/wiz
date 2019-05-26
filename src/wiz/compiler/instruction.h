#ifndef WIZ_COMPILER_INSTRUCTION_H
#define WIZ_COMPILER_INSTRUCTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>

#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/variant.h>
#include <wiz/utility/overload.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/report.h>
#include <wiz/utility/bit_flags.h>

namespace wiz {
    struct Definition;
    struct Attribute;
    struct AttributeStack;
    struct Expression;

    enum class BranchKind;
    enum class UnaryOperatorKind;
    enum class BinaryOperatorKind;

    class Bank;
    class Report;

    struct InstructionOperand {
        struct BitIndex {
            BitIndex(
                FwdUniquePtr<const InstructionOperand> operand,
                FwdUniquePtr<const InstructionOperand> index)
            : operand(std::move(operand)),
            subscript(std::move(index)) {}

            FwdUniquePtr<const InstructionOperand> operand;
            FwdUniquePtr<const InstructionOperand> subscript;
        };

        struct Binary {
            Binary(
                BinaryOperatorKind kind,
                FwdUniquePtr<const InstructionOperand> left,
                FwdUniquePtr<const InstructionOperand> right)
            : kind(kind),
            left(std::move(left)),
            right(std::move(right)) {}

            BinaryOperatorKind kind;
            FwdUniquePtr<const InstructionOperand> left;
            FwdUniquePtr<const InstructionOperand> right;
        };

        struct Boolean {
            Boolean(
                bool value)
            : value(value),
            placeholder(false) {}

            Boolean(
                bool value,
                bool placeholder)
            : value(value),
            placeholder(placeholder) {}

            bool value;
            bool placeholder;
        };

        struct Dereference {
            Dereference(
                bool far,
                FwdUniquePtr<const InstructionOperand> operand,
                std::size_t size)
            : far(far),
            operand(std::move(operand)),
            size(size) {}

            bool far;
            FwdUniquePtr<const InstructionOperand> operand;
            std::size_t size;
        };

        struct Index {
            Index(
                bool far,
                FwdUniquePtr<const InstructionOperand> operand,
                FwdUniquePtr<const InstructionOperand> subscript,
                std::size_t subscriptScale,
                std::size_t size)
            : far(far),
            operand(std::move(operand)),
            subscript(std::move(subscript)),
            subscriptScale(subscriptScale),
            size(size) {}

            bool far;
            FwdUniquePtr<const InstructionOperand> operand;
            FwdUniquePtr<const InstructionOperand> subscript;
            std::size_t subscriptScale;
            std::size_t size;
        };

        struct Integer {
            Integer(
                Int128 value)
            : value(value),
            placeholder(false) {}

            Integer(
                Int128 value,
                bool placeholder)
            : value(value),
            placeholder(placeholder) {}

            Int128 value;
            bool placeholder;
        };

        struct Register {
            Register(
                const Definition* definition)
            : definition(definition) {}

            const Definition* definition;
        };

        struct Unary {
            Unary(
                UnaryOperatorKind kind,
                FwdUniquePtr<const InstructionOperand> operand)
            : kind(kind),
            operand(std::move(operand)) {}

            UnaryOperatorKind kind;
            FwdUniquePtr<const InstructionOperand> operand;
        };

        template <typename T>
        InstructionOperand(
            T&& variant)
        : variant(std::forward<T>(variant)) {}

        FwdUniquePtr<InstructionOperand> clone() const;
        int compare(const InstructionOperand& other) const;
        std::string toString() const;

        bool operator ==(const InstructionOperand& other) const {
            return compare(other) == 0;
        }

        bool operator !=(const InstructionOperand& other) const {
            return compare(other) != 0;
        }

        bool operator <(const InstructionOperand& other) const {
            return compare(other) < 0;
        }

        bool operator >(const InstructionOperand& other) const {
            return compare(other) > 0;
        }

        bool operator <=(const InstructionOperand& other) const {
            return compare(other) <= 0;
        }

        bool operator >=(const InstructionOperand& other) const {
            return compare(other) >= 0;
        }

        using VariantType = Variant<
            BitIndex,
            Binary,
            Boolean,
            Dereference,
            Index,
            Integer,
            Register,
            Unary
        >;

        VariantType variant;
    };

    struct InstructionOperandPattern {
        struct BitIndex {
            BitIndex(
                FwdUniquePtr<const InstructionOperandPattern> operandPattern,
                FwdUniquePtr<const InstructionOperandPattern> subscriptPattern)
            : operandPattern(std::move(operandPattern)),
            subscriptPattern(std::move(subscriptPattern)) {}

            FwdUniquePtr<const InstructionOperandPattern> operandPattern;
            FwdUniquePtr<const InstructionOperandPattern> subscriptPattern;
        };

        struct Boolean {
            Boolean(
                bool value)
            : value(value) {}

            bool value;
        };

        struct Capture {
            Capture(
                FwdUniquePtr<const InstructionOperandPattern> operandPattern)
            : operandPattern(std::move(operandPattern)) {}

            FwdUniquePtr<const InstructionOperandPattern> operandPattern;
        };

        struct Dereference {
            Dereference(
                bool far,
                FwdUniquePtr<const InstructionOperandPattern> operand,
                std::size_t size)
            : far(far),
            operandPattern(std::move(operand)),
            size(size) {}

            bool far;
            FwdUniquePtr<const InstructionOperandPattern> operandPattern;
            std::size_t size;
        };

        struct Index {
            Index(
                bool far,
                FwdUniquePtr<const InstructionOperandPattern> operandPattern,
                FwdUniquePtr<const InstructionOperandPattern> subscriptPattern,
                std::size_t subscriptScale,
                std::size_t size)
            : far(far),
            operandPattern(std::move(operandPattern)),
            subscriptPattern(std::move(subscriptPattern)),
            subscriptScale(subscriptScale),
            size(size) {}

            bool far;
            FwdUniquePtr<const InstructionOperandPattern> operandPattern;
            FwdUniquePtr<const InstructionOperandPattern> subscriptPattern;
            std::size_t subscriptScale;
            std::size_t size;
        };

        struct IntegerAtLeast {
            IntegerAtLeast(
                Int128 min)
            : min(min) {}

            Int128 min;
        };

        struct IntegerRange {
            IntegerRange(
                Int128 value)
            : min(value),
            max(value) {}

            IntegerRange(
                Int128 min,
                Int128 max)
            : min(min),
            max(max) {}

            Int128 min;
            Int128 max;
        };

        struct Register {
            Register(
                Definition* definition)
            : definition(definition) {}

            Definition* definition;
        };

        struct Unary {
            Unary(
                UnaryOperatorKind kind,
                FwdUniquePtr<const InstructionOperandPattern> operandPattern)
            : kind(kind),
            operandPattern(std::move(operandPattern)) {}

            UnaryOperatorKind kind;
            FwdUniquePtr<const InstructionOperandPattern> operandPattern;
        };
        
        template <typename T>
        InstructionOperandPattern(
            T&& variant)
        : variant(std::forward<T>(variant)) {}

        FwdUniquePtr<InstructionOperandPattern> clone() const;
        int compare(const InstructionOperandPattern& other) const;
        bool isSubsetOf(const InstructionOperandPattern& other) const;
        bool matches(const InstructionOperand& operand) const;
        bool extract(const InstructionOperand& operand, std::vector<const InstructionOperand*>& captureList) const; 
        std::string toString() const;

        bool operator ==(const InstructionOperandPattern& other) const {
            return compare(other) == 0;
        }

        bool operator !=(const InstructionOperandPattern& other) const {
            return compare(other) != 0;
        }

        bool operator <(const InstructionOperandPattern& other) const {
            return compare(other) < 0;
        }

        bool operator >(const InstructionOperandPattern& other) const {
            return compare(other) > 0;
        }

        bool operator <=(const InstructionOperandPattern& other) const {
            return compare(other) <= 0;
        }

        bool operator >=(const InstructionOperandPattern& other) const {
            return compare(other) >= 0;
        }

        using VariantType = Variant<
            BitIndex,
            Boolean,
            Capture,
            Dereference,
            Index,
            IntegerAtLeast,
            IntegerRange,
            Register,
            Unary
        >;
        VariantType variant;
    };

    struct InstructionOperandRoot {
        InstructionOperandRoot()
        : expression(nullptr),
        operand(nullptr) {}

        InstructionOperandRoot(
            const Expression* expression,
            FwdUniquePtr<const InstructionOperand> operand)
        : expression(expression),
        operand(std::move(operand)) {}

        InstructionOperandRoot(InstructionOperandRoot&&) = default;
        InstructionOperandRoot(const InstructionOperandRoot&) = delete;

        const Expression* expression = nullptr;
        FwdUniquePtr<const InstructionOperand> operand;
    };

    struct InstructionOptions {
        InstructionOptions(
            const std::vector<std::uint8_t>& opcode,
            const std::vector<std::size_t>& parameter,
            const std::vector<Definition*> affectedFlags)
        : opcode(opcode),
        parameter(parameter),
        affectedFlags(affectedFlags) {}

        std::vector<std::uint8_t> opcode;
        std::vector<std::size_t> parameter;
        std::vector<Definition*> affectedFlags;
    };

    using InstructionSizeFunc = std::size_t (*)(const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists);
    using InstructionWriteFunc = bool (*)(Report* report, const Bank* bank, std::vector<std::uint8_t>& buffer, const InstructionOptions& options, const std::vector<std::vector<const InstructionOperand*>>& captureLists, SourceLocation location);

    struct InstructionEncoding {
        InstructionEncoding(
            InstructionSizeFunc calculateSize,
            InstructionWriteFunc write)
        : calculateSize(calculateSize),
        write(write) {}
        
        InstructionSizeFunc calculateSize;
        InstructionWriteFunc write;
    };

    struct InstructionType {
        // Takes a list of source operands to match on.
        // Called like a void function.
        struct VoidIntrinsic {
            VoidIntrinsic(Definition* definition)
            : definition(definition) {}

            Definition* definition;
        };

        // Takes dest as first argument in signature (which is given by lvalue being assigned to), followed by source operands to match on.
        // Called like a normal function, but it is particular about what destination it is being returned into.
        struct LoadIntrinsic {
            LoadIntrinsic(Definition* definition)
            : definition(definition) {}

            Definition* definition;
        };

        InstructionType(const InstructionType&) = default;
        InstructionType(InstructionType&&) = default;

        template <typename T>
        InstructionType(
            T&& variant)
        : variant(std::forward<T>(variant)) {}

        int compare(const InstructionType& other) const;
        std::size_t hash() const;

        bool operator ==(const InstructionType& other) const {
            return compare(other) == 0;
        }

        bool operator !=(const InstructionType& other) const {
            return compare(other) != 0;
        }

        bool operator <(const InstructionType& other) const {
            return compare(other) < 0;
        }

        bool operator >(const InstructionType& other) const {
            return compare(other) > 0;
        }

        bool operator <=(const InstructionType& other) const {
            return compare(other) <= 0;
        }

        bool operator >=(const InstructionType& other) const {
            return compare(other) >= 0;
        }

        using VariantType = Variant<
            BranchKind,
            UnaryOperatorKind,
            BinaryOperatorKind,
            VoidIntrinsic,
            LoadIntrinsic
        >;

        VariantType variant;
    };

    struct InstructionSignature {
        InstructionSignature(
            const InstructionType& type,
            std::uint32_t requiredModeFlags,
            const std::vector<const InstructionOperandPattern*>& operandPatterns)
        : type(type),
        requiredModeFlags(requiredModeFlags),
        operandPatterns(operandPatterns) {}

        InstructionType type;
        std::uint32_t requiredModeFlags;
        std::vector<const InstructionOperandPattern*> operandPatterns;

        int compare(const InstructionSignature& other) const;
        bool isSubsetOf(const InstructionSignature& other) const;
        bool matches(std::uint32_t modeFlags, const std::vector<InstructionOperandRoot>& operandRoots) const;
        bool extract(const std::vector<InstructionOperandRoot>& operandRoots, std::vector<std::vector<const InstructionOperand*>>& captureLists) const;
    };

    struct Instruction {
        Instruction(
            const InstructionSignature& signature,
            const InstructionEncoding* encoding,
            const InstructionOptions& options)
        : signature(signature),
        encoding(encoding),
        options(options) {}

        InstructionSignature signature;
        const InstructionEncoding* encoding;
        InstructionOptions options;
    };
}

namespace std {
    template <> struct hash<wiz::InstructionType> {
        std::size_t operator()(const wiz::InstructionType& type) const {
            return type.hash();
        }
    };
}

#endif