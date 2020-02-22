#include <cstddef>
#include <algorithm>

#include <wiz/ast/expression.h>
#include <wiz/compiler/bank.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/utility/overload.h>

namespace wiz {
    template <>
    void FwdDeleter<InstructionOperand>::operator()(const InstructionOperand* ptr) {
        delete ptr;
    }

    FwdUniquePtr<InstructionOperand> InstructionOperand::clone() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndex = variant.get<BitIndex>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::BitIndex(
                    bitIndex.operand->clone(),
                    bitIndex.subscript->clone()));
            }
            case VariantType::typeIndexOf<Binary>(): {
                const auto& bin = variant.get<Binary>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Binary(
                    bin.kind,
                    bin.left->clone(),
                    bin.right->clone()));
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& boolean = variant.get<Boolean>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(
                    boolean.value));
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereference = variant.get<Dereference>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(
                    dereference.far,
                    dereference.operand->clone(),
                    dereference.size));
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& index = variant.get<Index>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                    index.far,
                    index.operand->clone(),
                    index.subscript->clone(),
                    index.subscriptScale,
                    index.size));
            }
            case VariantType::typeIndexOf<Integer>(): {
                const auto& integer = variant.get<Integer>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(
                    integer.value, integer.placeholder));
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& reg = variant.get<Register>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Register(
                    reg.definition));
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& un = variant.get<Unary>();
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Unary(
                    un.kind,
                    un.operand->clone()));
            }
            default: std::abort(); return nullptr;
        }
    }

    int InstructionOperand::compare(const InstructionOperand& other) const {
        if (this == &other) {
            return 0;
        }

        if (variant.index() != other.variant.index()) {
            return variant.index() < other.variant.index() ? -1 : 1;
        }

        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndex = variant.get<BitIndex>();
                const auto& otherBitIndex = other.variant.get<BitIndex>();
                int result = bitIndex.operand->compare(*otherBitIndex.operand);
                if (result != 0) {
                    return result;
                }
                return bitIndex.subscript->compare(*otherBitIndex.subscript);
            }
            case VariantType::typeIndexOf<Binary>(): {
                const auto& bin = variant.get<Binary>();
                const auto& otherBin = other.variant.get<Binary>();
                if (bin.kind != otherBin.kind) {
                    return bin.kind < otherBin.kind ? -1 : 1;
                }
                int result = bin.left->compare(*otherBin.left);
                if (result != 0) {
                    return result;
                }
                return bin.right->compare(*otherBin.right);
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& boolean = variant.get<Boolean>();
                const auto& otherBoolean = other.variant.get<Boolean>();
                return (boolean.value ? 1 : 0) - (otherBoolean.value ? 1 : 0);
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereference = variant.get<Dereference>();
                const auto& otherDereference = other.variant.get<Dereference>();
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
            case VariantType::typeIndexOf<Index>(): {
                const auto& index = variant.get<Index>();
                const auto& otherIndex = other.variant.get<Index>();
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
            case VariantType::typeIndexOf<Integer>(): {
                const auto& integer = variant.get<Integer>();
                const auto& otherInteger = other.variant.get<Integer>();
                if (integer.value != otherInteger.value) {
                    return integer.value < otherInteger.value ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& reg = variant.get<Register>();
                const auto& otherReg = other.variant.get<Register>();
                if (reg.definition != otherReg.definition) {
                    return reg.definition < otherReg.definition ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& un = variant.get<Unary>();
                const auto& otherUn = other.variant.get<Unary>();
                if (un.kind != otherUn.kind) {
                    return un.kind < otherUn.kind ? -1 : 1;
                }
                return un.operand->compare(*otherUn.operand);
            }
            default: std::abort(); return -1;
        }
    }

    std::string InstructionOperand::toString() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndex = variant.get<BitIndex>();
                return bitIndex.operand->toString() + " $ " + bitIndex.subscript->toString();
            }
            case VariantType::typeIndexOf<Binary>(): {
                const auto& bin = variant.get<Binary>();
                return "(" + bin.left->toString()
                + " " + getBinaryOperatorSymbol(bin.kind).toString() + " "
                + bin.right->toString() + ")";
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& boolean = variant.get<Boolean>();
                return boolean.placeholder ? "{bool}" : std::string(boolean.value ? "true" : "false");
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereference = variant.get<Dereference>();
                return "*("
				+ dereference.operand->toString()
				+ " as "
				+ std::string(dereference.far ? "far " : "")
				+ "*u" + std::to_string(dereference.size * 8)
				+ ")";
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& index = variant.get<Index>();
                return "*(("
                + index.operand->toString()
                + " + " + index.subscript->toString()
                + (index.subscriptScale > 1 ? " * " + std::to_string(index.subscriptScale) : "")
                + ") as "
				+ std::string(index.far ? "far " : "")
				+ "*u" + std::to_string(index.size * 8)
				+ ")";
            }
            case VariantType::typeIndexOf<Integer>(): {
                const auto& integer = variant.get<Integer>();
                return integer.placeholder ? "{integer}" : integer.value.toString();
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& reg = variant.get<Register>();
                return reg.definition->name.toString();
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& un = variant.get<Unary>();               
                switch (un.kind) {
                    case UnaryOperatorKind::PostIncrement:
                    case UnaryOperatorKind::PostDecrement:
                        return un.operand->toString() + getUnaryOperatorSymbol(un.kind).toString();
                    default:
                        return getUnaryOperatorSymbol(un.kind).toString() + un.operand->toString();
                }
            }
            default: std::abort(); return "";
        }
    }




    template <>
    void FwdDeleter<InstructionOperandPattern>::operator()(const InstructionOperandPattern* ptr) {
        delete ptr;
    }

    FwdUniquePtr<InstructionOperandPattern> InstructionOperandPattern::clone() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::BitIndex(
                    bitIndexPattern.operandPattern->clone(),
                    bitIndexPattern.subscriptPattern->clone()));
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& booleanPattern = variant.get<Boolean>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Boolean(booleanPattern.value));
            }
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(capturePattern.operandPattern->clone()));
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    dereferencePattern.far,
                    dereferencePattern.operandPattern->clone(),
                    dereferencePattern.size));
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    indexPattern.far,
                    indexPattern.operandPattern->clone(),
                    indexPattern.subscriptPattern->clone(),
                    indexPattern.subscriptScale,
                    indexPattern.size));
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): {
                const auto& atLeastPattern = variant.get<IntegerAtLeast>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerAtLeast(atLeastPattern.min));
            }
            case VariantType::typeIndexOf<IntegerRange>(): {
                const auto& rangePattern = variant.get<IntegerRange>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerRange(rangePattern.min, rangePattern.max));
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& registerPattern = variant.get<Register>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Register(registerPattern.definition));
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Unary(
                    unPattern.kind,
                    unPattern.operandPattern->clone()));
            }
            default: std::abort(); return nullptr;
        }
    }

    int InstructionOperandPattern::compare(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return 0;
        }

        if (variant.index() != other.variant.index()) {
            return variant.index() < other.variant.index() ? -1 : 1;
        }

        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                const auto& otherBitIndexPattern = other.variant.get<BitIndex>();
                int result = bitIndexPattern.operandPattern->compare(*otherBitIndexPattern.operandPattern);
                if (result != 0) {
                    return result;
                }
                return bitIndexPattern.subscriptPattern->compare(*otherBitIndexPattern.subscriptPattern);
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& booleanPattern = variant.get<Boolean>();
                const auto& otherBooleanPattern = other.variant.get<Boolean>();
                return (booleanPattern.value ? 1 : 0) - (otherBooleanPattern.value ? 1 : 0);
            }
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                const auto& otherCapturePattern = other.variant.get<Capture>();
                return capturePattern.operandPattern->compare(*otherCapturePattern.operandPattern);
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                const auto& otherDereferencePattern = other.variant.get<Dereference>();
                if (dereferencePattern.far != otherDereferencePattern.far) {
                    return (dereferencePattern.far ? 1 : 0) - (otherDereferencePattern.far ? 1 : 0);
                }
                return dereferencePattern.operandPattern->compare(*otherDereferencePattern.operandPattern);
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                const auto& otherIndexPattern = other.variant.get<Index>();
                if (indexPattern.far != otherIndexPattern.far) {
                    return (indexPattern.far ? 1 : 0) - (otherIndexPattern.far ? 1 : 0);
                }
                int result = indexPattern.operandPattern->compare(*otherIndexPattern.operandPattern);
                if (result != 0) {
                    return result;
                }
                result = indexPattern.subscriptPattern->compare(*otherIndexPattern.subscriptPattern);
                if (result != 0) {
                    return result;
                }
                if (indexPattern.subscriptScale != otherIndexPattern.subscriptScale) {
                    return indexPattern.subscriptScale < otherIndexPattern.subscriptScale ? -1 : 1;
                }
                if (indexPattern.size != otherIndexPattern.size) {
                    return indexPattern.size < otherIndexPattern.size ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): {
                const auto& atLeastPattern = variant.get<IntegerAtLeast>();
                const auto& otherAtLeastPattern = other.variant.get<IntegerAtLeast>();
                if (atLeastPattern.min != otherAtLeastPattern.min) {
                    return atLeastPattern.min < otherAtLeastPattern.min ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<IntegerRange>(): {
                const auto& rangePattern = variant.get<IntegerRange>();
                const auto& otherRangePattern = other.variant.get<IntegerRange>();
                if (rangePattern.min != otherRangePattern.min) {
                    return rangePattern.min < otherRangePattern.min ? -1 : 1;
                }
                if (rangePattern.max != otherRangePattern.max) {
                    return rangePattern.max < otherRangePattern.max ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& registerPattern = variant.get<Register>();
                const auto& otherRegisterPattern = other.variant.get<Register>();
                if (registerPattern.definition != otherRegisterPattern.definition) {
                    return registerPattern.definition < otherRegisterPattern.definition ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                const auto& otherUnPattern = other.variant.get<Unary>();
                if (unPattern.kind != otherUnPattern.kind) {
                    return unPattern.kind < otherUnPattern.kind ? -1 : 1;
                }
                return unPattern.operandPattern->compare(*otherUnPattern.operandPattern);
            }
            default: std::abort(); return -1;
        }
    }

    bool InstructionOperandPattern::isSubsetOf(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return true;
        }

        if (const auto otherCapturePattern = other.variant.tryGet<Capture>()) {
            return isSubsetOf(*otherCapturePattern->operandPattern);
        }

        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                if (const auto otherIndexPattern = other.variant.tryGet<BitIndex>()) {
                    return bitIndexPattern.operandPattern->isSubsetOf(*otherIndexPattern->operandPattern)
                    && bitIndexPattern.subscriptPattern->isSubsetOf(*otherIndexPattern->subscriptPattern);
                }
                return false;
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& booleanPattern = variant.get<Boolean>();
                if (const auto otherBooleanPattern = other.variant.tryGet<Boolean>()) {
                    return booleanPattern.value == otherBooleanPattern->value;
                }
                return false;
            }
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                return capturePattern.operandPattern->isSubsetOf(other);
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                if (const auto otherDereferencePattern = other.variant.tryGet<Dereference>()) {
                    return dereferencePattern.far == otherDereferencePattern->far
                    && dereferencePattern.size == otherDereferencePattern->size
                    && dereferencePattern.operandPattern->isSubsetOf(*otherDereferencePattern->operandPattern);
                }
                return false;
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                if (const auto otherIndexPattern = other.variant.tryGet<Index>()) {
                    return indexPattern.far == otherIndexPattern->far
                    && indexPattern.size == otherIndexPattern->size
                    && indexPattern.subscriptScale == otherIndexPattern->subscriptScale
                    && indexPattern.operandPattern->isSubsetOf(*otherIndexPattern->operandPattern)
                    && indexPattern.subscriptPattern->isSubsetOf(*otherIndexPattern->subscriptPattern);
                }
                return false;
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): {
                const auto& atLeastPattern = variant.get<IntegerAtLeast>();
                if (const auto otherAtLeastPattern = other.variant.tryGet<IntegerAtLeast>()) {
                    return atLeastPattern.min >= otherAtLeastPattern->min;
                }
                return false;
            }
            case VariantType::typeIndexOf<IntegerRange>(): {
                const auto& rangePattern = variant.get<IntegerRange>();
                if (const auto otherRangePattern = other.variant.tryGet<IntegerRange>()) {
                    return otherRangePattern->min <= rangePattern.min && rangePattern.max <= otherRangePattern->max;
                }
                return false;
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& registerPattern = variant.get<Register>();
                if (const auto otherRegisterPattern = other.variant.tryGet<Register>()) {
                    return registerPattern.definition == otherRegisterPattern->definition;
                }
                return false;
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                if (const auto otherUnPattern = other.variant.tryGet<Unary>()) {
                    return unPattern.kind == otherUnPattern->kind
                    && unPattern.operandPattern->isSubsetOf(*otherUnPattern->operandPattern);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    bool InstructionOperandPattern::matches(const InstructionOperand& operand) const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                if (const auto bitIndexOperand = operand.variant.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndexPattern.operandPattern->matches(*bitIndexOperand->operand)
                    && bitIndexPattern.subscriptPattern->matches(*bitIndexOperand->subscript);
                }
                return false;
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& booleanPattern = variant.get<Boolean>();
                if (const auto booleanOperand = operand.variant.tryGet<InstructionOperand::Boolean>()) {
                    return booleanPattern.value == booleanOperand->value;
                }
                return false;
            }
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                return capturePattern.operandPattern->matches(operand);
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                if (const auto dereferenceOperand = operand.variant.tryGet<InstructionOperand::Dereference>()) {
                    return dereferencePattern.far == dereferenceOperand->far
                    && dereferencePattern.size == dereferenceOperand->size
                    && dereferencePattern.operandPattern->matches(*dereferenceOperand->operand);
                }
                return false;            
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                if (const auto indexOperand = operand.variant.tryGet<InstructionOperand::Index>()) {
                    if (indexPattern.far == indexOperand->far
                    && indexPattern.size == indexOperand->size
                    && indexPattern.subscriptScale == indexOperand->subscriptScale) {
                        if (indexPattern.operandPattern->matches(*indexOperand->operand)
                        && indexPattern.subscriptPattern->matches(*indexOperand->subscript)) {
                            return true;
                        }
                        if (indexPattern.subscriptScale == 1
                        && indexPattern.subscriptPattern->matches(*indexOperand->operand)
                        && indexPattern.operandPattern->matches(*indexOperand->subscript)) {
                            return true;
                        }
                    }
                }
                return false;
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): {
                const auto& atLeastPattern = variant.get<IntegerAtLeast>();
                if (const auto integerOperand = operand.variant.tryGet<InstructionOperand::Integer>()) {
                    bool match = atLeastPattern.min <= integerOperand->value;
                    return match; 
                }
                return false;
            }
            case VariantType::typeIndexOf<IntegerRange>(): {
                const auto& rangePattern = variant.get<IntegerRange>();
                if (const auto integerOperand = operand.variant.tryGet<InstructionOperand::Integer>()) {
                    return rangePattern.min <= integerOperand->value && integerOperand->value <= rangePattern.max;
                }
                return false;
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& registerPattern = variant.get<Register>();
                if (const auto registerOperand = operand.variant.tryGet<InstructionOperand::Register>()) {
                    return registerPattern.definition == registerOperand->definition;
                }
                return false;
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                if (const auto unOperand = operand.variant.tryGet<InstructionOperand::Unary>()) {
                    return unPattern.kind == unOperand->kind
                    && unPattern.operandPattern->matches(*unOperand->operand);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    bool InstructionOperandPattern::extract(const InstructionOperand& operand, std::vector<const InstructionOperand*>& captureList) const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                if (const auto bitIndexOperand = operand.variant.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndexPattern.operandPattern->extract(*bitIndexOperand->operand, captureList)
                    && bitIndexPattern.subscriptPattern->extract(*bitIndexOperand->subscript, captureList);
                }
                return false;
            }
            case VariantType::typeIndexOf<Boolean>(): return matches(operand);
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                if (capturePattern.operandPattern->matches(operand)) {
                    captureList.push_back(&operand);
                    return true;
                }
                return false;
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                if (const auto dereferenceOperand = operand.variant.tryGet<InstructionOperand::Dereference>()) {
                    return dereferencePattern.far == dereferenceOperand->far
                    && dereferencePattern.size == dereferenceOperand->size
                    && dereferencePattern.operandPattern->extract(*dereferenceOperand->operand, captureList);
                }
                return false;
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                if (const auto indexOperand = operand.variant.tryGet<InstructionOperand::Index>()) {
                    if (indexPattern.far == indexOperand->far
                    && indexPattern.size == indexOperand->size
                    && indexPattern.subscriptScale == indexOperand->subscriptScale) {
                        if (indexPattern.operandPattern->matches(*indexOperand->operand)
                        && indexPattern.subscriptPattern->matches(*indexOperand->subscript)) {
                            return indexPattern.operandPattern->extract(*indexOperand->operand, captureList)
                            && indexPattern.subscriptPattern->extract(*indexOperand->subscript, captureList);
                        }
                        // operand[subscript] is the same as *(operand + subscriptScale * subscript), so we can attempt to exploit commutativity rules in some cases.
                        // Commute the operand and subscript, but also extract them in opposite order, so that the captures are correct.
                        else if (indexPattern.subscriptScale == 1
                        && indexPattern.subscriptPattern->matches(*indexOperand->operand)
                        && indexPattern.operandPattern->matches(*indexOperand->subscript)) {
                            return indexPattern.subscriptPattern->extract(*indexOperand->operand, captureList)
                            && indexPattern.operandPattern->extract(*indexOperand->subscript, captureList);
                        }
                    }
                }
                return false;
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): return matches(operand);
            case VariantType::typeIndexOf<IntegerRange>(): {
                auto matched = matches(operand);
                if (matched) {
                    return true;
                }
                return false;
            }
            case VariantType::typeIndexOf<Register>(): return matches(operand);
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                if (const auto unOperand = operand.variant.tryGet<InstructionOperand::Unary>()) {
                    return unPattern.kind == unOperand->kind
                    && unPattern.operandPattern->extract(*unOperand->operand, captureList);
                }
                return false;
            }
            default: std::abort(); return false;
        }
    }

    std::string InstructionOperandPattern::toString() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BitIndex>(): {
                const auto& bitIndexPattern = variant.get<BitIndex>();
                return bitIndexPattern.operandPattern->toString() + " $ " + bitIndexPattern.subscriptPattern->toString();
            }
            case VariantType::typeIndexOf<Boolean>(): {
                const auto& booleanPattern = variant.get<Boolean>();
                return std::string(booleanPattern.value ? "true" : "false");
            }
            case VariantType::typeIndexOf<Capture>(): {
                const auto& capturePattern = variant.get<Capture>();
                return capturePattern.operandPattern->toString();
            }
            case VariantType::typeIndexOf<Dereference>(): {
                const auto& dereferencePattern = variant.get<Dereference>();
                return "*("
                + dereferencePattern.operandPattern->toString()
                + " as "
				+ std::string(dereferencePattern.far ? "far " : "")
				+ "*u" + std::to_string(dereferencePattern.size * 8)
				+ ")";
            }
            case VariantType::typeIndexOf<Index>(): {
                const auto& indexPattern = variant.get<Index>();
                return "*(("
                + indexPattern.operandPattern->toString()
                + " + " + indexPattern.subscriptPattern->toString()
                + (indexPattern.subscriptScale > 1 ? " * " + std::to_string(indexPattern.subscriptScale) : "")
                + ") as "
				+ std::string(indexPattern.far ? "far " : "")
				+ "*u" + std::to_string(indexPattern.size * 8)
				+ ")";
            }
            case VariantType::typeIndexOf<IntegerAtLeast>(): {
                const auto& atLeastPattern = variant.get<IntegerAtLeast>();
                return "{integer >= " + atLeastPattern.min.toString() + "}";
            }
            case VariantType::typeIndexOf<IntegerRange>(): {
                const auto& rangePattern = variant.get<IntegerRange>();
                if (rangePattern.min == rangePattern.max) {
                    return rangePattern.min.toString();
                }
                return "{" + rangePattern.min.toString() + ".." + rangePattern.max.toString() + "}";
            }
            case VariantType::typeIndexOf<Register>(): {
                const auto& registerPattern = variant.get<Register>();
                return registerPattern.definition->name.toString();
            }
            case VariantType::typeIndexOf<Unary>(): {
                const auto& unPattern = variant.get<Unary>();
                switch (unPattern.kind) {
                    case UnaryOperatorKind::PostIncrement:
                    case UnaryOperatorKind::PostDecrement:
                        return unPattern.operandPattern->toString() + getUnaryOperatorSymbol(unPattern.kind).toString();
                    default:
                        return getUnaryOperatorSymbol(unPattern.kind).toString() + unPattern.operandPattern->toString();
                }
            }
            default: std::abort(); return "";
        }
    }




    template<>
    void FwdDeleter<InstructionEncoding>::operator()(const InstructionEncoding* ptr) {
        delete ptr;
    }



    int InstructionType::compare(const InstructionType& other) const {
        if (variant.index() != other.variant.index()) {
            return variant.index() - other.variant.index();
        }
        switch (variant.index()) {
            case VariantType::typeIndexOf<BranchKind>(): return static_cast<int>(variant.get<BranchKind>()) - static_cast<int>(other.variant.get<BranchKind>());
            case VariantType::typeIndexOf<UnaryOperatorKind>(): return static_cast<int>(variant.get<UnaryOperatorKind>()) - static_cast<int>(other.variant.get<UnaryOperatorKind>());
            case VariantType::typeIndexOf<BinaryOperatorKind>(): return static_cast<int>(variant.get<BinaryOperatorKind>()) - static_cast<int>(other.variant.get<BinaryOperatorKind>());
            case VariantType::typeIndexOf<VoidIntrinsic>(): {
                const auto& value = variant.get<VoidIntrinsic>();
                const auto& otherValue = other.variant.get<VoidIntrinsic>();
                if (value.definition != otherValue.definition) {
                    return value.definition < otherValue.definition ? -1 : 1;
                }
                return 0;
            }
            case VariantType::typeIndexOf<LoadIntrinsic>(): {
                const auto& value = variant.get<LoadIntrinsic>();
                const auto& otherValue = other.variant.get<LoadIntrinsic>();
                if (value.definition != otherValue.definition) {
                    return value.definition < otherValue.definition ? -1 : 1;
                }
                return 0;
            }
            default: std::abort(); return 0;
        }
    }

    std::size_t InstructionType::hash() const {
        switch (variant.index()) {
            case VariantType::typeIndexOf<BranchKind>(): return std::hash<int>()(static_cast<int>(variant.get<BranchKind>()));
            case VariantType::typeIndexOf<UnaryOperatorKind>(): return std::hash<int>()(static_cast<int>(variant.get<UnaryOperatorKind>()));
            case VariantType::typeIndexOf<BinaryOperatorKind>(): return std::hash<int>()(static_cast<int>(variant.get<BinaryOperatorKind>()));
            case VariantType::typeIndexOf<VoidIntrinsic>(): return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(variant.get<VoidIntrinsic>().definition));
            case VariantType::typeIndexOf<LoadIntrinsic>(): return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(variant.get<LoadIntrinsic>().definition));
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