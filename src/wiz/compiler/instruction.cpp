#include <cstddef>
#include <algorithm>

#include <wiz/ast/expression.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>

namespace wiz {
    template <>
    void FwdDeleter<InstructionOperand>::operator()(const InstructionOperand* ptr) {
        delete ptr;
    }

    InstructionOperand::~InstructionOperand() {
        switch (kind) {
            case InstructionOperandKind::BitIndex: bitIndex.~BitIndex(); break;
            case InstructionOperandKind::Binary: binary.~Binary(); break;
            case InstructionOperandKind::Boolean: boolean.~Boolean(); break;
            case InstructionOperandKind::Dereference: dereference.~Dereference(); break;
            case InstructionOperandKind::Index: index.~Index(); break;
            case InstructionOperandKind::Integer: integer.~Integer(); break;
            case InstructionOperandKind::Register: register_.~Register(); break;
            case InstructionOperandKind::Unary: unary.~Unary(); break;
        }
    }

    FwdUniquePtr<InstructionOperand> InstructionOperand::clone() const {
        switch (kind) {
            case InstructionOperandKind::BitIndex: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::BitIndex(
                    bitIndex.operand->clone(),
                    bitIndex.subscript->clone()));
            }
            case InstructionOperandKind::Binary: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Binary(
                    binary.kind,
                    binary.left->clone(),
                    binary.right->clone()));
            }
            case InstructionOperandKind::Boolean: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(
                    boolean.value,
                    boolean.placeholder));
            }
            case InstructionOperandKind::Dereference: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(
                    dereference.far,
                    dereference.operand->clone(),
                    dereference.size));
            }
            case InstructionOperandKind::Index: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                    index.far,
                    index.operand->clone(),
                    index.subscript->clone(),
                    index.subscriptScale,
                    index.size));
            }
            case InstructionOperandKind::Integer: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(
                    integer.value,
                    integer.placeholder));
            }
            case InstructionOperandKind::Register: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Register(
                    register_.definition));
            }
            case InstructionOperandKind::Unary: {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Unary(
                    unary.kind,
                    unary.operand->clone()));
            }
            default: std::abort(); return nullptr;
        }
    }

    int InstructionOperand::compare(const InstructionOperand& other) const {
        if (this == &other) {
            return 0;
        }

        if (kind != other.kind) {
            return kind < other.kind ? -1 : 1;
        }

        switch (kind) {
            case InstructionOperandKind::BitIndex: {
                const auto& otherBitIndex = other.bitIndex;
                int result = bitIndex.operand->compare(*otherBitIndex.operand);
                if (result != 0) {
                    return result;
                }
                return bitIndex.subscript->compare(*otherBitIndex.subscript);
            }
            case InstructionOperandKind::Binary: {
                const auto& otherBinary = other.binary;
                if (binary.kind != otherBinary.kind) {
                    return binary.kind < otherBinary.kind ? -1 : 1;
                }
                int result = binary.left->compare(*otherBinary.left);
                if (result != 0) {
                    return result;
                }
                return binary.right->compare(*otherBinary.right);
            }
            case InstructionOperandKind::Boolean: {
                const auto& otherBoolean = other.boolean;
                return (boolean.value ? 1 : 0) - (otherBoolean.value ? 1 : 0);
            }
            case InstructionOperandKind::Dereference: {
                const auto& otherDereference = other.dereference;
                if (dereference.far != otherDereference.far) {
                    return (dereference.far ? 1 : 0) - (otherDereference.far ? 1 : 0);
                }
                int result = dereference.operand->compare(*otherDereference.operand);
                if (result != 0) {
                    return result;
                }
                if (dereference.size != otherDereference.size) {
                    return dereference.size < otherDereference.size ? -1 : 1;
                }        
                return 0;
            }
            case InstructionOperandKind::Index: {
                const auto& otherIndex = other.index;
                if (index.far != otherIndex.far) {
                    return (index.far ? 1 : 0) - (otherIndex.far ? 1 : 0);
                }
                int result = index.operand->compare(*otherIndex.operand);
                if (result != 0) {
                    return result;
                }
                result = index.subscript->compare(*otherIndex.subscript);
                if (result != 0) {
                    return result;
                }
                if (index.subscriptScale != otherIndex.subscriptScale) {
                    return index.subscriptScale < otherIndex.subscriptScale ? -1 : 1;
                }
                if (index.size != otherIndex.size) {
                    return index.size < otherIndex.size ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandKind::Integer: {
                const auto& otherInteger = other.integer;
                if (integer.value != otherInteger.value) {
                    return integer.value < otherInteger.value ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandKind::Register: {
                const auto& otherRegister = other.register_;
                if (register_.definition != otherRegister.definition) {
                    return register_.definition < otherRegister.definition ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandKind::Unary: {
                const auto& otherUnary = other.unary;
                if (unary.kind != otherUnary.kind) {
                    return unary.kind < otherUnary.kind ? -1 : 1;
                }
                return unary.operand->compare(*otherUnary.operand);
            }
            default: std::abort(); return -1;
        }
    }

    std::string InstructionOperand::toString() const {
        switch (kind) {
            case InstructionOperandKind::BitIndex: {
                return bitIndex.operand->toString() + " $ " + bitIndex.subscript->toString();
            }
            case InstructionOperandKind::Binary: {
                return "(" + binary.left->toString()
                + " " + getBinaryOperatorSymbol(binary.kind).toString() + " "
                + binary.right->toString() + ")";
            }
            case InstructionOperandKind::Boolean: {
                return boolean.placeholder ? "{bool}" : std::string(boolean.value ? "true" : "false");
            }
            case InstructionOperandKind::Dereference: {
                return "*("
				+ dereference.operand->toString()
				+ " as "
				+ std::string(dereference.far ? "far " : "")
				+ "*u" + std::to_string(dereference.size * 8)
				+ ")";
            }
            case InstructionOperandKind::Index: {
                return "*(("
                + index.operand->toString()
                + " + " + index.subscript->toString()
                + (index.subscriptScale > 1 ? " * " + std::to_string(index.subscriptScale) : "")
                + ") as "
				+ std::string(index.far ? "far " : "")
				+ "*u" + std::to_string(index.size * 8)
				+ ")";
            }
            case InstructionOperandKind::Integer: {
                return integer.placeholder ? "{integer}" : integer.value.toString();
            }
            case InstructionOperandKind::Register: {
                return register_.definition->name.toString();
            }
            case InstructionOperandKind::Unary: {
                return isUnaryPostIncrementOperator(unary.kind)
                    ? unary.operand->toString() + getUnaryOperatorSymbol(unary.kind).toString()
                    : getUnaryOperatorSymbol(unary.kind).toString() + unary.operand->toString();
            }
            default: std::abort(); return "";
        }
    }




    template <>
    void FwdDeleter<InstructionOperandPattern>::operator()(const InstructionOperandPattern* ptr) {
        delete ptr;
    }

    InstructionOperandPattern::~InstructionOperandPattern() {
        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: bitIndex.~BitIndex(); break;
            case InstructionOperandPatternKind::Boolean: boolean.~Boolean(); break;
            case InstructionOperandPatternKind::Capture: capture.~Capture(); break;
            case InstructionOperandPatternKind::Dereference: dereference.~Dereference(); break;
            case InstructionOperandPatternKind::Index: index.~Index(); break;
            case InstructionOperandPatternKind::IntegerAtLeast: integerAtLeast.~IntegerAtLeast(); break;
            case InstructionOperandPatternKind::IntegerRange: integerRange.~IntegerRange(); break;
            case InstructionOperandPatternKind::Register: register_.~Register(); break;
            case InstructionOperandPatternKind::Unary: unary.~Unary(); break;
        }
    }

    FwdUniquePtr<InstructionOperandPattern> InstructionOperandPattern::clone() const {
        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::BitIndex(
                    bitIndex.operandPattern->clone(),
                    bitIndex.subscriptPattern->clone()));
            }
            case InstructionOperandPatternKind::Boolean: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Boolean(boolean.value));
            }
            case InstructionOperandPatternKind::Capture: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(capture.operandPattern->clone()));
            }
            case InstructionOperandPatternKind::Dereference: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    dereference.far,
                    dereference.operandPattern->clone(),
                    dereference.size));
            }
            case InstructionOperandPatternKind::Index: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    index.far,
                    index.operandPattern->clone(),
                    index.subscriptPattern->clone(),
                    index.subscriptScale,
                    index.size));
            }
            case InstructionOperandPatternKind::IntegerAtLeast: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerAtLeast(integerAtLeast.min));
            }
            case InstructionOperandPatternKind::IntegerRange: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerRange(integerRange.min, integerRange.max));
            }
            case InstructionOperandPatternKind::Register: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Register(register_.definition));
            }
            case InstructionOperandPatternKind::Unary: {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Unary(
                    unary.kind,
                    unary.operandPattern->clone()));
            }
            default: std::abort(); return nullptr;
        }
    }

    int InstructionOperandPattern::compare(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return 0;
        }

        if (kind != other.kind) {
            return kind < other.kind ? -1 : 1;
        }

        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                const auto& otherBitIndex = other.bitIndex;
                int result = bitIndex.operandPattern->compare(*otherBitIndex.operandPattern);
                if (result != 0) {
                    return result;
                }
                return bitIndex.subscriptPattern->compare(*otherBitIndex.subscriptPattern);
            }
            case InstructionOperandPatternKind::Boolean: {
                const auto& otherBoolean = other.boolean;
                return (otherBoolean.value ? 1 : 0) - (otherBoolean.value ? 1 : 0);
            }
            case InstructionOperandPatternKind::Capture: {
                const auto& otherCapture = other.capture;
                return capture.operandPattern->compare(*otherCapture.operandPattern);
            }
            case InstructionOperandPatternKind::Dereference: {
                const auto& otherDereference = other.dereference;
                if (dereference.far != otherDereference.far) {
                    return (dereference.far ? 1 : 0) - (otherDereference.far ? 1 : 0);
                }
                return dereference.operandPattern->compare(*otherDereference.operandPattern);
            }
            case InstructionOperandPatternKind::Index: {
                const auto& otherIndex = other.index;
                if (index.far != otherIndex.far) {
                    return (index.far ? 1 : 0) - (otherIndex.far ? 1 : 0);
                }
                int result = index.operandPattern->compare(*otherIndex.operandPattern);
                if (result != 0) {
                    return result;
                }
                result = index.subscriptPattern->compare(*otherIndex.subscriptPattern);
                if (result != 0) {
                    return result;
                }
                if (index.subscriptScale != otherIndex.subscriptScale) {
                    return index.subscriptScale < otherIndex.subscriptScale ? -1 : 1;
                }
                if (index.size != otherIndex.size) {
                    return index.size < otherIndex.size ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandPatternKind::IntegerAtLeast: {
                const auto& otherIntegerAtLeast = other.integerAtLeast;
                if (integerAtLeast.min != otherIntegerAtLeast.min) {
                    return integerAtLeast.min < otherIntegerAtLeast.min ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandPatternKind::IntegerRange: {
                const auto& otherIntegerRange = other.integerRange;
                if (integerRange.min != otherIntegerRange.min) {
                    return integerRange.min < otherIntegerRange.min ? -1 : 1;
                }
                if (integerRange.max != otherIntegerRange.max) {
                    return integerRange.max < otherIntegerRange.max ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandPatternKind::Register: {
                const auto& otherRegister = other.register_;
                if (register_.definition != otherRegister.definition) {
                    return register_.definition < otherRegister.definition ? -1 : 1;
                }
                return 0;
            }
            case InstructionOperandPatternKind::Unary: {
                const auto& otherUnary = other.unary;
                if (unary.kind != otherUnary.kind) {
                    return unary.kind < otherUnary.kind ? -1 : 1;
                }
                return unary.operandPattern->compare(*otherUnary.operandPattern);
            }
            default: std::abort(); return -1;
        }
    }

    bool InstructionOperandPattern::isSubsetOf(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return true;
        }

        if (const auto otherCapture = other.tryGet<Capture>()) {
            return isSubsetOf(*otherCapture->operandPattern);
        }

        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                if (const auto otherIndex = other.tryGet<BitIndex>()) {
                    return bitIndex.operandPattern->isSubsetOf(*otherIndex->operandPattern)
                    && bitIndex.subscriptPattern->isSubsetOf(*otherIndex->subscriptPattern);
                }
                return false;
            }
            case InstructionOperandPatternKind::Boolean: {
                if (const auto otherBoolean = other.tryGet<Boolean>()) {
                    return boolean.value == otherBoolean->value;
                }
                return false;
            }
            case InstructionOperandPatternKind::Capture: {
                return capture.operandPattern->isSubsetOf(other);
            }
            case InstructionOperandPatternKind::Dereference: {
                if (const auto otherDereference = other.tryGet<Dereference>()) {
                    return dereference.far == otherDereference->far
                    && dereference.size == otherDereference->size
                    && dereference.operandPattern->isSubsetOf(*otherDereference->operandPattern);
                }
                return false;
            }
            case InstructionOperandPatternKind::Index: {
                if (const auto otherIndex = other.tryGet<Index>()) {
                    return index.far == otherIndex->far
                    && index.size == otherIndex->size
                    && index.subscriptScale == otherIndex->subscriptScale
                    && index.operandPattern->isSubsetOf(*otherIndex->operandPattern)
                    && index.subscriptPattern->isSubsetOf(*otherIndex->subscriptPattern);
                }
                return false;
            }
            case InstructionOperandPatternKind::IntegerAtLeast: {
                if (const auto otherAtLeastPattern = other.tryGet<IntegerAtLeast>()) {
                    return integerAtLeast.min >= otherAtLeastPattern->min;
                }
                return false;
            }
            case InstructionOperandPatternKind::IntegerRange: {
                if (const auto otherRange = other.tryGet<IntegerRange>()) {
                    return otherRange->min <= integerRange.min && integerRange.max <= otherRange->max;
                }
                return false;
            }
            case InstructionOperandPatternKind::Register: {
                if (const auto otherRegister = other.tryGet<Register>()) {
                    return register_.definition == otherRegister->definition;
                }
                return false;
            }
            case InstructionOperandPatternKind::Unary: {
                if (const auto otherUnary = other.tryGet<Unary>()) {
                    return unary.kind == otherUnary->kind
                    && unary.operandPattern->isSubsetOf(*otherUnary->operandPattern);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    bool InstructionOperandPattern::matches(const InstructionOperand& operand) const {
        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                if (const auto bitIndexOperand = operand.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndex.operandPattern->matches(*bitIndexOperand->operand)
                    && bitIndex.subscriptPattern->matches(*bitIndexOperand->subscript);
                }
                return false;
            }
            case InstructionOperandPatternKind::Boolean: {
                if (const auto booleanOperand = operand.tryGet<InstructionOperand::Boolean>()) {
                    return boolean.value == booleanOperand->value;
                }
                return false;
            }
            case InstructionOperandPatternKind::Capture: {
                return capture.operandPattern->matches(operand);
            }
            case InstructionOperandPatternKind::Dereference: {
                if (const auto dereferenceOperand = operand.tryGet<InstructionOperand::Dereference>()) {
                    return dereference.far == dereferenceOperand->far
                    && dereference.size == dereferenceOperand->size
                    && dereference.operandPattern->matches(*dereferenceOperand->operand);
                }
                return false;            
            }
            case InstructionOperandPatternKind::Index: {
                if (const auto indexOperand = operand.tryGet<InstructionOperand::Index>()) {
                    if (index.far == indexOperand->far
                    && index.size == indexOperand->size
                    && index.subscriptScale == indexOperand->subscriptScale) {
                        if (index.operandPattern->matches(*indexOperand->operand)
                        && index.subscriptPattern->matches(*indexOperand->subscript)) {
                            return true;
                        }
                        if (index.subscriptScale == 1
                        && index.subscriptPattern->matches(*indexOperand->operand)
                        && index.operandPattern->matches(*indexOperand->subscript)) {
                            return true;
                        }
                    }
                }
                return false;
            }
            case InstructionOperandPatternKind::IntegerAtLeast: {
                if (const auto integerOperand = operand.tryGet<InstructionOperand::Integer>()) {
                    bool match = integerAtLeast.min <= integerOperand->value;
                    return match; 
                }
                return false;
            }
            case InstructionOperandPatternKind::IntegerRange: {
                if (const auto integerOperand = operand.tryGet<InstructionOperand::Integer>()) {
                    return integerRange.min <= integerOperand->value && integerOperand->value <= integerRange.max;
                }
                return false;
            }
            case InstructionOperandPatternKind::Register: {
                if (const auto registerOperand = operand.tryGet<InstructionOperand::Register>()) {
                    return register_.definition == registerOperand->definition;
                }
                return false;
            }
            case InstructionOperandPatternKind::Unary: {
                if (const auto unOperand = operand.tryGet<InstructionOperand::Unary>()) {
                    return unary.kind == unOperand->kind
                    && unary.operandPattern->matches(*unOperand->operand);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    bool InstructionOperandPattern::extract(const InstructionOperand& operand, std::vector<const InstructionOperand*>& captureList) const {
        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                if (const auto bitIndexOperand = operand.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndex.operandPattern->extract(*bitIndexOperand->operand, captureList)
                    && bitIndex.subscriptPattern->extract(*bitIndexOperand->subscript, captureList);
                }
                return false;
            }
            case InstructionOperandPatternKind::Boolean: return matches(operand);
            case InstructionOperandPatternKind::Capture: {
                if (capture.operandPattern->matches(operand)) {
                    captureList.push_back(&operand);
                    return true;
                }
                return false;
            }
            case InstructionOperandPatternKind::Dereference: {
                if (const auto dereferenceOperand = operand.tryGet<InstructionOperand::Dereference>()) {
                    return dereference.far == dereferenceOperand->far
                    && dereference.size == dereferenceOperand->size
                    && dereference.operandPattern->extract(*dereferenceOperand->operand, captureList);
                }
                return false;
            }
            case InstructionOperandPatternKind::Index: {
                if (const auto indexOperand = operand.tryGet<InstructionOperand::Index>()) {
                    if (index.far == indexOperand->far
                    && index.size == indexOperand->size
                    && index.subscriptScale == indexOperand->subscriptScale) {
                        if (index.operandPattern->matches(*indexOperand->operand)
                        && index.subscriptPattern->matches(*indexOperand->subscript)) {
                            return index.operandPattern->extract(*indexOperand->operand, captureList)
                            && index.subscriptPattern->extract(*indexOperand->subscript, captureList);
                        }
                        // operand[subscript] is the same as *(operand + subscriptScale * subscript), so we can attempt to exploit commutativity rules in some cases.
                        // Commute the operand and subscript, but also extract them in opposite order, so that the captures are correct.
                        else if (index.subscriptScale == 1
                        && index.subscriptPattern->matches(*indexOperand->operand)
                        && index.operandPattern->matches(*indexOperand->subscript)) {
                            return index.subscriptPattern->extract(*indexOperand->operand, captureList)
                            && index.operandPattern->extract(*indexOperand->subscript, captureList);
                        }
                    }
                }
                return false;
            }
            case InstructionOperandPatternKind::IntegerAtLeast: return matches(operand);
            case InstructionOperandPatternKind::IntegerRange: {
                auto matched = matches(operand);
                if (matched) {
                    return true;
                }
                return false;
            }
            case InstructionOperandPatternKind::Register: return matches(operand);
            case InstructionOperandPatternKind::Unary: {
                if (const auto unOperand = operand.tryGet<InstructionOperand::Unary>()) {
                    return unary.kind == unOperand->kind
                    && unary.operandPattern->extract(*unOperand->operand, captureList);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    std::string InstructionOperandPattern::toString() const {
        switch (kind) {
            case InstructionOperandPatternKind::BitIndex: {
                return bitIndex.operandPattern->toString() + " $ " + bitIndex.subscriptPattern->toString();
            }
            case InstructionOperandPatternKind::Boolean: {
                return std::string(boolean.value ? "true" : "false");
            }
            case InstructionOperandPatternKind::Capture: {
                return capture.operandPattern->toString();
            }
            case InstructionOperandPatternKind::Dereference: {
                return "*("
                + dereference.operandPattern->toString()
                + " as "
                + std::string(dereference.far ? "far " : "")
                + "*u" + std::to_string(dereference.size * 8)
                + ")";
            }
            case InstructionOperandPatternKind::Index: {
                return "*(("
                + index.operandPattern->toString()
                + " + " + index.subscriptPattern->toString()
                + (index.subscriptScale > 1 ? " * " + std::to_string(index.subscriptScale) : "")
                + ") as "
                + std::string(index.far ? "far " : "")
                + "*u" + std::to_string(index.size * 8)
                + ")";
            }
            case InstructionOperandPatternKind::IntegerAtLeast: {
                return "{integer >= " + integerAtLeast.min.toString() + "}";
            }
            case InstructionOperandPatternKind::IntegerRange: {
                if (integerRange.min == integerRange.max) {
                    return integerRange.min.toString();
                }
                return "{" + integerRange.min.toString() + ".." + integerRange.max.toString() + "}";
            }
            case InstructionOperandPatternKind::Register: {
                return register_.definition->name.toString();
            }
            case InstructionOperandPatternKind::Unary: {
                return isUnaryPostIncrementOperator(unary.kind)
                    ? unary.operandPattern->toString() + getUnaryOperatorSymbol(unary.kind).toString()
                    : getUnaryOperatorSymbol(unary.kind).toString() + unary.operandPattern->toString();
            }
            default: std::abort(); return "";
        }
    }




    template<>
    void FwdDeleter<InstructionEncoding>::operator()(const InstructionEncoding* ptr) {
        delete ptr;
    }



    int InstructionType::compare(const InstructionType& other) const {
        if (kind != other.kind) {
            return kind < other.kind ? -1 : 1;
        }
        switch (kind) {
            case InstructionTypeKind::BranchKind: return static_cast<int>(branchKind) - static_cast<int>(other.branchKind);
            case InstructionTypeKind::UnaryOperatorKind: return static_cast<int>(unaryOperatorKind) - static_cast<int>(other.unaryOperatorKind);
            case InstructionTypeKind::BinaryOperatorKind: return static_cast<int>(binaryOperatorKind) - static_cast<int>(other.binaryOperatorKind);
            case InstructionTypeKind::VoidIntrinsic: {
                const auto& otherVoidIntrinsic = other.voidIntrinsic;
                if (voidIntrinsic.definition != otherVoidIntrinsic.definition) {
                    return voidIntrinsic.definition < otherVoidIntrinsic.definition ? -1 : 1;
                }
                return 0;
            }
            case InstructionTypeKind::LoadIntrinsic: {
                const auto& otherLoadIntrinsic = other.loadIntrinsic;
                if (loadIntrinsic.definition != otherLoadIntrinsic.definition) {
                    return loadIntrinsic.definition < otherLoadIntrinsic.definition ? -1 : 1;
                }
                return 0;
            }
            default: std::abort(); return 0;
        }
    }

    std::size_t InstructionType::hash() const {
        switch (kind) {
            case InstructionTypeKind::BranchKind: return std::hash<int>()(static_cast<int>(branchKind));
            case InstructionTypeKind::UnaryOperatorKind: return std::hash<int>()(static_cast<int>(unaryOperatorKind));
            case InstructionTypeKind::BinaryOperatorKind: return std::hash<int>()(static_cast<int>(binaryOperatorKind));
            case InstructionTypeKind::VoidIntrinsic: return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(voidIntrinsic.definition));
            case InstructionTypeKind::LoadIntrinsic: return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(loadIntrinsic.definition));
            default: std::abort(); return 0;
        }
    }



    int InstructionSignature::compare(const InstructionSignature& other) const {
        if (this == &other) {
            return 0;
        }

        int result = type.compare(other.type);
        if (result != 0) {
            return result;
        }

        if (requiredModeFlags != other.requiredModeFlags) {
            return requiredModeFlags < other.requiredModeFlags ? -1 : 1;
        }
        
        for (std::size_t i = 0; i != operandPatterns.size() && i != other.operandPatterns.size(); ++i) {
            const auto operandPattern = operandPatterns[i];
            const auto otherOperandPattern = other.operandPatterns[i];

            result = operandPattern->compare(*otherOperandPattern);
            if (result != 0) {
                return result;
            }
        }

        if (operandPatterns.size() != other.operandPatterns.size()) {
            return operandPatterns.size() < other.operandPatterns.size() ? -1 : 1;
        }
        return 0;
    }

    bool InstructionSignature::isSubsetOf(const InstructionSignature& other) const {
        if (this == &other) {
            return true;
        }
        if (type != other.type) {
            return false;
        }
        if (requiredModeFlags != other.requiredModeFlags) {
            return false;
        }
        if (operandPatterns.size() != other.operandPatterns.size()) {
            return false;
        }
        for (std::size_t i = 0; i != operandPatterns.size(); ++i) {
            if (!operandPatterns[i]->isSubsetOf(*other.operandPatterns[i])) {
                return false;
            }
        }
        return true;
    }

    bool InstructionSignature::matches(std::uint32_t modeFlags, const std::vector<InstructionOperandRoot>& operandRoots) const {
        if (requiredModeFlags != 0 && (requiredModeFlags & modeFlags) != requiredModeFlags) {
            return false;
        }
        if (operandPatterns.size() != operandRoots.size()) {
            return false;
        }
        for (std::size_t i = 0; i != operandPatterns.size(); ++i) {
            if (!operandPatterns[i]->matches(*operandRoots[i].operand)) {
                return false;
            }
        }
        return true;
    }

    bool InstructionSignature::extract(const std::vector<InstructionOperandRoot>& operandRoots, std::vector<std::vector<const InstructionOperand*>>& captureLists) const {
        const auto operandRootsCount = operandRoots.size();
        if (captureLists.size() < operandRootsCount) {
            captureLists.resize(operandRootsCount);
        }
        for (auto& captureList : captureLists) {
            captureList.clear();
        }
        for (std::size_t i = 0; i != operandRootsCount; ++i) {
            const auto operandPattern = operandPatterns[i];
            const auto& operand = operandRoots[i].operand;
            auto& captureList = captureLists[i];

            if (!operandPattern->extract(*operand, captureList)) {
                return false;
            }
            if (captureList.size() == 0) {
                captureList.push_back(operand.get());
            }
        }
        return true;
    }



    template<>
    void FwdDeleter<Instruction>::operator()(const Instruction* ptr) {
        delete ptr;
    }
}