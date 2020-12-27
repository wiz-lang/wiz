#ifndef WIZ_COMPILER_INSTRUCTION_H
#define WIZ_COMPILER_INSTRUCTION_H

#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>

#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/report.h>

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

    enum class InstructionOperandKind {
        BitIndex,
        Binary,
        Boolean,
        Dereference,
        Index,
        Integer,
        Register,
        Unary,
    };

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

        InstructionOperand(
            BitIndex bitIndex)
        : kind(InstructionOperandKind::BitIndex),
        bitIndex(std::move(bitIndex)) {}

        InstructionOperand(
            Binary binary)
        : kind(InstructionOperandKind::Binary),
        binary(std::move(binary)) {}

        InstructionOperand(
            Boolean boolean)
        : kind(InstructionOperandKind::Boolean),
        boolean(std::move(boolean)) {}

        InstructionOperand(
            Dereference dereference)
        : kind(InstructionOperandKind::Dereference),
        dereference(std::move(dereference)) {}

        InstructionOperand(
            Index index)
        : kind(InstructionOperandKind::Index),
        index(std::move(index)) {}

        InstructionOperand(
            Integer integer)
        : kind(InstructionOperandKind::Integer),
        integer(std::move(integer)) {}

        InstructionOperand(
            Register register_)
        : kind(InstructionOperandKind::Register),
        register_(std::move(register_)) {}

        InstructionOperand(
            Unary unary)
        : kind(InstructionOperandKind::Unary),
        unary(std::move(unary)) {}

        ~InstructionOperand();

        template <typename T> const T* tryGet() const;

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

        InstructionOperandKind kind;
        union {
            BitIndex bitIndex;
            Binary binary;
            Boolean boolean;
            Dereference dereference;
            Index index;
            Integer integer;
            Register register_;
            Unary unary;
        };
    };

    template <> WIZ_FORCE_INLINE const InstructionOperand::BitIndex* InstructionOperand::tryGet<InstructionOperand::BitIndex>() const {
        return kind == InstructionOperandKind::BitIndex ? &bitIndex : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Binary* InstructionOperand::tryGet<InstructionOperand::Binary>() const {
        return kind == InstructionOperandKind::Binary ? &binary : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Boolean* InstructionOperand::tryGet<InstructionOperand::Boolean>() const {
        return kind == InstructionOperandKind::Boolean ? &boolean : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Dereference* InstructionOperand::tryGet<InstructionOperand::Dereference>() const {
        return kind == InstructionOperandKind::Dereference ? &dereference : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Index* InstructionOperand::tryGet<InstructionOperand::Index>() const {
        return kind == InstructionOperandKind::Index ? &index : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Integer* InstructionOperand::tryGet<InstructionOperand::Integer>() const {
        return kind == InstructionOperandKind::Integer ? &integer : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Register* InstructionOperand::tryGet<InstructionOperand::Register>() const {
        return kind == InstructionOperandKind::Register ? &register_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperand::Unary* InstructionOperand::tryGet<InstructionOperand::Unary>() const {
        return kind == InstructionOperandKind::Unary ? &unary : nullptr;
    }

    enum class InstructionOperandPatternKind {
        BitIndex,
        Boolean,
        Capture,
        Dereference,
        Index,
        IntegerAtLeast,
        IntegerRange,
        Register,
        Unary,
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
        
        InstructionOperandPattern(
            BitIndex bitIndex)
        : kind(InstructionOperandPatternKind::BitIndex),
        bitIndex(std::move(bitIndex)) {}

        InstructionOperandPattern(
            Boolean boolean)
        : kind(InstructionOperandPatternKind::Boolean),
        boolean(std::move(boolean)) {}

        InstructionOperandPattern(
            Capture capture)
        : kind(InstructionOperandPatternKind::Capture),
        capture(std::move(capture)) {}

        InstructionOperandPattern(
            Dereference dereference)
        : kind(InstructionOperandPatternKind::Dereference),
        dereference(std::move(dereference)) {}

        InstructionOperandPattern(
            Index index)
        : kind(InstructionOperandPatternKind::Index),
        index(std::move(index)) {}

        InstructionOperandPattern(
            IntegerAtLeast integerAtLeast)
        : kind(InstructionOperandPatternKind::IntegerAtLeast),
        integerAtLeast(std::move(integerAtLeast)) {}

        InstructionOperandPattern(
            IntegerRange integerRange)
        : kind(InstructionOperandPatternKind::IntegerRange),
        integerRange(std::move(integerRange)) {}

        InstructionOperandPattern(
            Register register_)
        : kind(InstructionOperandPatternKind::Register),
        register_(std::move(register_)) {}

        InstructionOperandPattern(
            Unary unary)
        : kind(InstructionOperandPatternKind::Unary),
        unary(std::move(unary)) {}

        ~InstructionOperandPattern();

        template <typename T> const T* tryGet() const;

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

        InstructionOperandPatternKind kind;
        union {
            BitIndex bitIndex;
            Boolean boolean;
            Capture capture;
            Dereference dereference;
            Index index;
            IntegerAtLeast integerAtLeast;
            IntegerRange integerRange;
            Register register_;
            Unary unary;
        };
    };

    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::BitIndex* InstructionOperandPattern::tryGet<InstructionOperandPattern::BitIndex>() const {
        return kind == InstructionOperandPatternKind::BitIndex ? &bitIndex : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Boolean* InstructionOperandPattern::tryGet<InstructionOperandPattern::Boolean>() const {
        return kind == InstructionOperandPatternKind::Boolean ? &boolean : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Capture* InstructionOperandPattern::tryGet<InstructionOperandPattern::Capture>() const {
        return kind == InstructionOperandPatternKind::Capture ? &capture : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Dereference* InstructionOperandPattern::tryGet<InstructionOperandPattern::Dereference>() const {
        return kind == InstructionOperandPatternKind::Dereference ? &dereference : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Index* InstructionOperandPattern::tryGet<InstructionOperandPattern::Index>() const {
        return kind == InstructionOperandPatternKind::Index ? &index : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::IntegerAtLeast* InstructionOperandPattern::tryGet<InstructionOperandPattern::IntegerAtLeast>() const {
        return kind == InstructionOperandPatternKind::IntegerAtLeast ? &integerAtLeast : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::IntegerRange* InstructionOperandPattern::tryGet<InstructionOperandPattern::IntegerRange>() const {
        return kind == InstructionOperandPatternKind::IntegerRange ? &integerRange : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Register* InstructionOperandPattern::tryGet<InstructionOperandPattern::Register>() const {
        return kind == InstructionOperandPatternKind::Register ? &register_ : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionOperandPattern::Unary* InstructionOperandPattern::tryGet<InstructionOperandPattern::Unary>() const {
        return kind == InstructionOperandPatternKind::Unary ? &unary : nullptr;
    }

    struct InstructionOperandRoot {
        InstructionOperandRoot()
        : expression(nullptr),
        operand(nullptr) {}

        InstructionOperandRoot(
            const Expression* expression,
            FwdUniquePtr<const InstructionOperand> operand)
        : expression(expression),
        operand(std::move(operand)) {}

        InstructionOperandRoot(InstructionOperandRoot&&) noexcept = default;
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

    enum class InstructionTypeKind {
        BranchKind,
        UnaryOperatorKind,
        BinaryOperatorKind,
        VoidIntrinsic,
        LoadIntrinsic,
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

        InstructionType(
            BranchKind branchKind)
        : kind(InstructionTypeKind::BranchKind),
        branchKind(branchKind) {}

        InstructionType(
            UnaryOperatorKind unaryOperatorKind)
        : kind(InstructionTypeKind::UnaryOperatorKind),
        unaryOperatorKind(unaryOperatorKind) {}

        InstructionType(
            BinaryOperatorKind binaryOperatorKind)
        : kind(InstructionTypeKind::BinaryOperatorKind),
        binaryOperatorKind(binaryOperatorKind) {}

        InstructionType(
            VoidIntrinsic voidIntrinsic)
        : kind(InstructionTypeKind::VoidIntrinsic),
        voidIntrinsic(voidIntrinsic) {}

        InstructionType(
            LoadIntrinsic loadIntrinsic)
        : kind(InstructionTypeKind::LoadIntrinsic),
        loadIntrinsic(loadIntrinsic) {}

        template <typename T> const T* tryGet() const;

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

        InstructionTypeKind kind;
        union {
            BranchKind branchKind;
            UnaryOperatorKind unaryOperatorKind;
            BinaryOperatorKind binaryOperatorKind;
            VoidIntrinsic voidIntrinsic;
            LoadIntrinsic loadIntrinsic;
        };
    };

    template <> WIZ_FORCE_INLINE const BranchKind* InstructionType::tryGet<BranchKind>() const {
        return kind == InstructionTypeKind::BranchKind ? &branchKind : nullptr;
    }
    template <> WIZ_FORCE_INLINE const UnaryOperatorKind* InstructionType::tryGet<UnaryOperatorKind>() const {
        return kind == InstructionTypeKind::UnaryOperatorKind ? &unaryOperatorKind : nullptr;
    }
    template <> WIZ_FORCE_INLINE const BinaryOperatorKind* InstructionType::tryGet<BinaryOperatorKind>() const {
        return kind == InstructionTypeKind::BinaryOperatorKind ? &binaryOperatorKind : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionType::VoidIntrinsic* InstructionType::tryGet<InstructionType::VoidIntrinsic>() const {
        return kind == InstructionTypeKind::VoidIntrinsic ? &voidIntrinsic : nullptr;
    }
    template <> WIZ_FORCE_INLINE const InstructionType::LoadIntrinsic* InstructionType::tryGet<InstructionType::LoadIntrinsic>() const {
        return kind == InstructionTypeKind::LoadIntrinsic ? &loadIntrinsic : nullptr;
    }

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