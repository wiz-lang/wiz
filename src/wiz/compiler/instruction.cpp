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
        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndex) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::BitIndex(
                    bitIndex.operand->clone(),
                    bitIndex.subscript->clone()));
            },
            [&](const Binary& bin) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Binary(
                    bin.kind,
                    bin.left->clone(),
                    bin.right->clone()));
            },
            [&](const Boolean& boolean) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Boolean(
                    boolean.value));
            },
            [&](const Dereference& dereference) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Dereference(
                    dereference.far,
                    dereference.operand->clone(),
                    dereference.size));
            },
            [&](const Index& index) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Index(
                    index.far,
                    index.operand->clone(),
                    index.subscript->clone(),
                    index.subscriptScale,
                    index.size));
            },
            [&](const Integer& integer) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Integer(
                    integer.value, integer.placeholder));
            },
            [&](const Register& reg) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Register(
                    reg.definition));
            },
            [&](const Unary& un) {
                return makeFwdUnique<InstructionOperand>(InstructionOperand::Unary(
                    un.kind,
                    un.operand->clone()));
            }
        ));
    }

    int InstructionOperand::compare(const InstructionOperand& other) const {
        if (this == &other) {
            return 0;
        }

        if (variant.index() != other.variant.index()) {
            return variant.index() < other.variant.index() ? -1 : 1;
        }

        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndex) {
                const auto& otherBitIndex = other.variant.get<BitIndex>();
                int result = bitIndex.operand->compare(*otherBitIndex.operand);
                if (result != 0) {
                    return result;
                }
                return bitIndex.subscript->compare(*otherBitIndex.subscript);
            },
            [&](const Binary& bin) {
                const auto& otherBin = other.variant.get<Binary>();
                if (bin.kind != otherBin.kind) {
                    return bin.kind < otherBin.kind ? -1 : 1;
                }
                int result = bin.left->compare(*otherBin.left);
                if (result != 0) {
                    return result;
                }
                return bin.right->compare(*otherBin.right);
            },
            [&](const Boolean& boolean) {
                const auto& otherBoolean = other.variant.get<Boolean>();
                return (boolean.value ? 1 : 0) - (otherBoolean.value ? 1 : 0);
            },
            [&](const Dereference& dereference) {
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
            },
            [&](const Index& index) {
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
            },
            [&](const Integer& integer) {
                const auto& otherInteger = other.variant.get<Integer>();
                if (integer.value != otherInteger.value) {
                    return integer.value < otherInteger.value ? -1 : 1;
                }
                return 0;
            },
            [&](const Register& reg) {
                const auto& otherReg = other.variant.get<Register>();
                if (reg.definition != otherReg.definition) {
                    return reg.definition < otherReg.definition ? -1 : 1;
                }
                return 0;
            },
            [&](const Unary& un) {
                const auto& otherUn = other.variant.get<Unary>();
                if (un.kind != otherUn.kind) {
                    return un.kind < otherUn.kind ? -1 : 1;
                }
                return un.operand->compare(*otherUn.operand);
            }
        ));
    }

    std::string InstructionOperand::toString() const {
        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndex) {
                return bitIndex.operand->toString() + " $ " + bitIndex.subscript->toString();
            },
            [&](const Binary& bin) {               
                return "(" + bin.left->toString()
                + " " + getBinaryOperatorSymbol(bin.kind).toString() + " "
                + bin.right->toString() + ")";
            },
            [&](const Boolean& boolean) {
                return boolean.placeholder ? "{bool}" : std::string(boolean.value ? "true" : "false");
            },
            [&](const Dereference& dereference) {
                return "*("
				+ dereference.operand->toString()
				+ " as "
				+ std::string(dereference.far ? "far " : "")
				+ "*u" + std::to_string(dereference.size * 8)
				+ ")";
            },
            [&](const Index& index) {
                return "*(("
                + index.operand->toString()
                + " + " + index.subscript->toString()
                + (index.subscriptScale > 1 ? " * " + std::to_string(index.subscriptScale) : "")
                + ") as "
				+ std::string(index.far ? "far " : "")
				+ "*u" + std::to_string(index.size * 8)
				+ ")";
            },
            [&](const Integer& integer) {
                return integer.placeholder ? "{integer}" : integer.value.toString();
            },
            [&](const Register& reg) {               
                return reg.definition->name.toString();
            },
            [&](const Unary& un) {               
                switch (un.kind) {
                    case UnaryOperatorKind::PreIncrement: return "++" + un.operand->toString();
                    case UnaryOperatorKind::PreDecrement: return "--" + un.operand->toString();
                    case UnaryOperatorKind::PostIncrement: return un.operand->toString() + "++";
                    case UnaryOperatorKind::PostDecrement: return un.operand->toString() + "--";
                    default: std::abort(); return std::string();
                }
            }
        ));
    }




    template <>
    void FwdDeleter<InstructionOperandPattern>::operator()(const InstructionOperandPattern* ptr) {
        delete ptr;
    }

    FwdUniquePtr<InstructionOperandPattern> InstructionOperandPattern::clone() const {
        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndexPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::BitIndex(
                    bitIndexPattern.operandPattern->clone(),
                    bitIndexPattern.subscriptPattern->clone()));
            },
            [&](const Boolean& booleanPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Boolean(booleanPattern.value));
            },
            [&](const Capture& capturePattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Capture(capturePattern.operandPattern->clone()));
            },
            [&](const Dereference& dereferencePattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Dereference(
                    dereferencePattern.far,
                    dereferencePattern.operandPattern->clone(),
                    dereferencePattern.size));
            },
            [&](const Index& indexPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Index(
                    indexPattern.far,
                    indexPattern.operandPattern->clone(),
                    indexPattern.subscriptPattern->clone(),
                    indexPattern.subscriptScale,
                    indexPattern.size));
            },
            [&](const IntegerAtLeast& atLeastPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerAtLeast(atLeastPattern.min));
            },
            [&](const IntegerRange& rangePattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::IntegerRange(rangePattern.min, rangePattern.max));
            },
            [&](const Register& registerPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Register(registerPattern.definition));
            },
            [&](const Unary& unPattern) {
                return makeFwdUnique<InstructionOperandPattern>(InstructionOperandPattern::Unary(
                    unPattern.kind,
                    unPattern.operandPattern->clone()));
            }
        ));
    }

    int InstructionOperandPattern::compare(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return 0;
        }

        if (variant.index() != other.variant.index()) {
            return variant.index() < other.variant.index() ? -1 : 1;
        }

        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndexPattern) {
                const auto& otherBitIndexPattern = other.variant.get<BitIndex>();
                int result = bitIndexPattern.operandPattern->compare(*otherBitIndexPattern.operandPattern);
                if (result != 0) {
                    return result;
                }
                return bitIndexPattern.subscriptPattern->compare(*otherBitIndexPattern.subscriptPattern);
            },
            [&](const Boolean& booleanPattern) {
                const auto& otherBooleanPattern = other.variant.get<Boolean>();
                return (booleanPattern.value ? 1 : 0) - (otherBooleanPattern.value ? 1 : 0);
            },
            [&](const Capture& capturePattern) {
                const auto& otherCapturePattern = other.variant.get<Capture>();
                return capturePattern.operandPattern->compare(*otherCapturePattern.operandPattern);
            },
            [&](const Dereference& dereferencePattern) {
                const auto& otherDereferencePattern = other.variant.get<Dereference>();
                if (dereferencePattern.far != otherDereferencePattern.far) {
                    return (dereferencePattern.far ? 1 : 0) - (otherDereferencePattern.far ? 1 : 0);
                }
                return dereferencePattern.operandPattern->compare(*otherDereferencePattern.operandPattern);
            },
            [&](const Index& indexPattern) {
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
            },
            [&](const IntegerAtLeast& atLeastPattern) {
                const auto& otherAtLeastPattern = other.variant.get<IntegerAtLeast>();
                if (atLeastPattern.min != otherAtLeastPattern.min) {
                    return atLeastPattern.min < otherAtLeastPattern.min ? -1 : 1;
                }
                return 0;
            },
            [&](const IntegerRange& rangePattern) {
                const auto& otherRangePattern = other.variant.get<IntegerRange>();
                if (rangePattern.min != otherRangePattern.min) {
                    return rangePattern.min < otherRangePattern.min ? -1 : 1;
                }
                if (rangePattern.max != otherRangePattern.max) {
                    return rangePattern.max < otherRangePattern.max ? -1 : 1;
                }
                return 0;
            },
            [&](const Register& registerPattern) {
                const auto& otherRegisterPattern = other.variant.get<Register>();
                if (registerPattern.definition != otherRegisterPattern.definition) {
                    return registerPattern.definition < otherRegisterPattern.definition ? -1 : 1;
                }
                return 0;
            },
            [&](const Unary& unPattern) {
                const auto& otherUnPattern = other.variant.get<Unary>();
                if (unPattern.kind != otherUnPattern.kind) {
                    return unPattern.kind < otherUnPattern.kind ? -1 : 1;
                }
                return unPattern.operandPattern->compare(*otherUnPattern.operandPattern);
            }
        ));
    }

    bool InstructionOperandPattern::isSubsetOf(const InstructionOperandPattern& other) const {
        if (this == &other) {
            return true;
        }
        if (const auto otherCapturePattern = other.variant.tryGet<Capture>()) {
            return isSubsetOf(*otherCapturePattern->operandPattern);
        }

        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndexPattern) {
                if (const auto otherIndexPattern = other.variant.tryGet<BitIndex>()) {
                    return bitIndexPattern.operandPattern->isSubsetOf(*otherIndexPattern->operandPattern)
                    && bitIndexPattern.subscriptPattern->isSubsetOf(*otherIndexPattern->subscriptPattern);
                }
                return false;
            },
            [&](const Boolean& booleanPattern) {
                if (const auto otherBooleanPattern = other.variant.tryGet<Boolean>()) {
                    return booleanPattern.value == otherBooleanPattern->value;
                }
                return false;
            },
            [&](const Capture&) { return false; },
            [&](const Dereference& dereferencePattern) {
                if (const auto otherDereferencePattern = other.variant.tryGet<Dereference>()) {
                    return dereferencePattern.far == otherDereferencePattern->far
                    && dereferencePattern.size == otherDereferencePattern->size
                    && dereferencePattern.operandPattern->isSubsetOf(*otherDereferencePattern->operandPattern);
                }
                return false;
            },
            [&](const Index& indexPattern) {
                if (const auto otherIndexPattern = other.variant.tryGet<Index>()) {
                    return indexPattern.far == otherIndexPattern->far
                    && indexPattern.size == otherIndexPattern->size
                    && indexPattern.subscriptScale == otherIndexPattern->subscriptScale
                    && indexPattern.operandPattern->isSubsetOf(*otherIndexPattern->operandPattern)
                    && indexPattern.subscriptPattern->isSubsetOf(*otherIndexPattern->subscriptPattern);
                }
                return false;
            },
            [&](const IntegerAtLeast& atLeastPattern) {
                if (const auto otherAtLeastPattern = other.variant.tryGet<IntegerAtLeast>()) {
                    return atLeastPattern.min >= otherAtLeastPattern->min;
                }
                return false;
            },
            [&](const IntegerRange& rangePattern) {
                if (const auto otherRangePattern = other.variant.tryGet<IntegerRange>()) {
                    return otherRangePattern->min <= rangePattern.min && rangePattern.max <= otherRangePattern->max;
                }
                return false;
            },
            [&](const Register& registerPattern) {
                if (const auto otherRegisterPattern = other.variant.tryGet<Register>()) {
                    return registerPattern.definition == otherRegisterPattern->definition;
                }
                return false;
            },
            [&](const Unary& unPattern) {
                if (const auto otherUnPattern = other.variant.tryGet<Unary>()) {
                    return unPattern.kind == otherUnPattern->kind
                    && unPattern.operandPattern->isSubsetOf(*otherUnPattern->operandPattern);
                }
                return false;
            }
        ));
    }

    bool InstructionOperandPattern::matches(const InstructionOperand& operand) const {
        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndexPattern) {
                if (const auto bitIndexOperand = operand.variant.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndexPattern.operandPattern->matches(*bitIndexOperand->operand)
                    && bitIndexPattern.subscriptPattern->matches(*bitIndexOperand->subscript);
                }
                return false;
            },
            [&](const Boolean& booleanPattern) {
                if (const auto booleanOperand = operand.variant.tryGet<InstructionOperand::Boolean>()) {
                    return booleanPattern.value == booleanOperand->value;
                }
                return false;
            },
            [&](const Capture& capturePattern) {
                return capturePattern.operandPattern->matches(operand);
            },
            [&](const Dereference& dereferencePattern) {
                if (const auto dereferenceOperand = operand.variant.tryGet<InstructionOperand::Dereference>()) {
                    return dereferencePattern.far == dereferenceOperand->far
                    && dereferencePattern.size == dereferenceOperand->size
                    && dereferencePattern.operandPattern->matches(*dereferenceOperand->operand);
                }
                return false;            
            },
            [&](const Index& indexPattern) {
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
            },
            [&](const IntegerAtLeast& atLeastPattern) {
                if (const auto integerOperand = operand.variant.tryGet<InstructionOperand::Integer>()) {
                    bool match = atLeastPattern.min <= integerOperand->value;
                    return match; 
                }
                return false;
            },
            [&](const IntegerRange& rangePattern) {
                if (const auto integerOperand = operand.variant.tryGet<InstructionOperand::Integer>()) {
                    return rangePattern.min <= integerOperand->value && integerOperand->value <= rangePattern.max;
                }
                return false;
            },
            [&](const Register& registerPattern) {
                if (const auto registerOperand = operand.variant.tryGet<InstructionOperand::Register>()) {
                    return registerPattern.definition == registerOperand->definition;
                }
                return false;
            },
            [&](const Unary& unPattern) {
                if (const auto unOperand = operand.variant.tryGet<InstructionOperand::Unary>()) {
                    return unPattern.kind == unOperand->kind
                    && unPattern.operandPattern->matches(*unOperand->operand);
                }
                return false;
            }
        ));
    }

    bool InstructionOperandPattern::extract(const InstructionOperand& operand, std::vector<const InstructionOperand*>& captureList) const {
        return variant.visit(makeOverload(
            [&](const BitIndex& bitIndexPattern) {
                if (const auto bitIndexOperand = operand.variant.tryGet<InstructionOperand::BitIndex>()) {
                    return bitIndexPattern.operandPattern->extract(*bitIndexOperand->operand, captureList)
                    && bitIndexPattern.subscriptPattern->extract(*bitIndexOperand->subscript, captureList);
                }
                return false;
            },
            [&](const Boolean&) { return matches(operand); },
            [&](const Capture& capturePattern) {
                if (capturePattern.operandPattern->matches(operand)) {
                    captureList.push_back(&operand);
                    return true;
                }
                return false;
            },
            [&](const Dereference& dereferencePattern) {
                if (const auto dereferenceOperand = operand.variant.tryGet<InstructionOperand::Dereference>()) {
                    return dereferencePattern.far == dereferenceOperand->far
                    && dereferencePattern.size == dereferenceOperand->size
                    && dereferencePattern.operandPattern->extract(*dereferenceOperand->operand, captureList);
                }
                return false;
            },
            [&](const Index& indexPattern) {
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
            },
            [&](const IntegerAtLeast&) { return matches(operand); },
            [&](const IntegerRange&) { return matches(operand); },
            [&](const Register&) { return matches(operand); },
            [&](const Unary& unPattern) {
                if (const auto unOperand = operand.variant.tryGet<InstructionOperand::Unary>()) {
                    return unPattern.kind == unOperand->kind
                    && unPattern.operandPattern->extract(*unOperand->operand, captureList);
                }
                return false;
            }
        ));
    }

    std::string InstructionOperandPattern::toString() const {
        return variant.visit(makeOverload(         
            [&](const BitIndex& bitIndexPattern) {
                return bitIndexPattern.operandPattern->toString() + " $ " + bitIndexPattern.subscriptPattern->toString();
            },
            [&](const Boolean& booleanPattern) {
                return std::string(booleanPattern.value ? "true" : "false");
            },
            [&](const Capture& capturePattern) {
                return capturePattern.operandPattern->toString();
            },
            [&](const Dereference& dereferencePattern) {
                return "*("
                + dereferencePattern.operandPattern->toString()
                + " as "
				+ std::string(dereferencePattern.far ? "far " : "")
				+ "*u" + std::to_string(dereferencePattern.size * 8)
				+ ")";
            },
            [&](const Index& indexPattern) {
                return "*(("
                + indexPattern.operandPattern->toString()
                + " + " + indexPattern.subscriptPattern->toString()
                + (indexPattern.subscriptScale > 1 ? " * " + std::to_string(indexPattern.subscriptScale) : "")
                + ") as "
				+ std::string(indexPattern.far ? "far " : "")
				+ "*u" + std::to_string(indexPattern.size * 8)
				+ ")";
            },
            [&](const IntegerAtLeast& atLeastPattern) {
                return "{integer >= " + atLeastPattern.min.toString() + "}";
            },
            [&](const IntegerRange& rangePattern) {
                if (rangePattern.min == rangePattern.max) {
                    return rangePattern.min.toString();
                }
                return "{" + rangePattern.min.toString() + ".." + rangePattern.max.toString() + "}";
            },
            [&](const Register& registerPattern) {
                return registerPattern.definition->name.toString();
            },
            [&](const Unary& unPattern) { 
                switch (unPattern.kind) {
                    case UnaryOperatorKind::PreIncrement: return "++" + unPattern.operandPattern->toString();
                    case UnaryOperatorKind::PreDecrement: return "--" + unPattern.operandPattern->toString();
                    case UnaryOperatorKind::PostIncrement: return unPattern.operandPattern->toString() + "++";
                    case UnaryOperatorKind::PostDecrement: return unPattern.operandPattern->toString() + "--";
                    default: std::abort(); return std::string();
                }
            }
        ));
    }




    template<>
    void FwdDeleter<InstructionEncoding>::operator()(const InstructionEncoding* ptr) {
        delete ptr;
    }



    int InstructionType::compare(const InstructionType& other) const {
        if (variant.index() != other.variant.index()) {
            return variant.index() - other.variant.index();
        }
        return variant.visit(makeOverload(
            [&](BranchKind value) { return static_cast<int>(value) - static_cast<int>(other.variant.get<BranchKind>()); },
            [&](UnaryOperatorKind value) { return static_cast<int>(value) - static_cast<int>(other.variant.get<UnaryOperatorKind>()); },
            [&](BinaryOperatorKind value) { return static_cast<int>(value) - static_cast<int>(other.variant.get<BinaryOperatorKind>()); },
            [&](const VoidIntrinsic& value) {
                const auto& otherValue = other.variant.get<VoidIntrinsic>();
                if (value.definition != otherValue.definition) {
                    return value.definition < otherValue.definition ? -1 : 1;
                }
                return 0;
            },
            [&](const LoadIntrinsic& value) {
                const auto& otherValue = other.variant.get<LoadIntrinsic>();
                if (value.definition != otherValue.definition) {
                    return value.definition < otherValue.definition ? -1 : 1;
                }
                return 0;
            }
        ));
    }

    std::size_t InstructionType::hash() const {
        return variant.visit(makeOverload(
            [&](BranchKind value) { return std::hash<int>()(static_cast<int>(value)); },
            [&](UnaryOperatorKind value) { return std::hash<int>()(static_cast<int>(value)); },
            [&](BinaryOperatorKind value) { return std::hash<int>()(static_cast<int>(value)); },
            [&](VoidIntrinsic value) { return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(value.definition)); },
            [&](LoadIntrinsic value) { return std::hash<std::uintptr_t>()(reinterpret_cast<std::uintptr_t>(value.definition)); }
        ));
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