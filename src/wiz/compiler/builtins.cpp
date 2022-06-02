#include <algorithm>

#include <wiz/ast/statement.h>
#include <wiz/ast/expression.h>
#include <wiz/ast/type_expression.h>

#include <wiz/compiler/bank.h>
#include <wiz/compiler/version.h>
#include <wiz/compiler/builtins.h>
#include <wiz/compiler/definition.h>
#include <wiz/compiler/instruction.h>
#include <wiz/compiler/symbol_table.h>

namespace wiz {
    namespace {
        const char* const propertyNames[static_cast<std::size_t>(Builtins::Property::Count)] = {
            "(no property)",
            "len",
            "min_value",
            "max_value",
        };

        const char* const declarationAttributeNames[static_cast<std::size_t>(Builtins::DeclarationAttribute::Count)] = {
            "(no declaration attribute)",
            "irq",
            "nmi",
            "fallthrough",
            "align",
        };
    }

    Builtins::Builtins(
        StringPool* stringPool,
        Platform* platform_,
        std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines_)
    : stringPool(stringPool),
    platform(platform_),
    defines(std::move(defines_)),
    scope(std::make_unique<SymbolTable>(nullptr, StringView())),
    declaration(makeFwdUnique<const Statement>(Statement::InternalDeclaration(), SourceLocation(stringPool->intern("<internal>")))),
    boolType(scope->createDefinition(nullptr, Definition::BuiltinBoolType(), stringPool->intern("bool"), declaration.get())), 
    u8Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(0), Int128(UINT8_MAX), 1), stringPool->intern("u8"), declaration.get())),
    u16Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(0), Int128(UINT16_MAX), 2), stringPool->intern("u16"), declaration.get())),
    u24Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(0), Int128(0xFFFFFFU), 3), stringPool->intern("u24"), declaration.get())),
    u32Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(0), Int128(UINT32_MAX), 4), stringPool->intern("u32"), declaration.get())),
    u64Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(0), Int128(UINT64_MAX), 8), stringPool->intern("u64"), declaration.get())),
    i8Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(INT8_MIN), Int128(INT8_MAX), 1), stringPool->intern("i8"), declaration.get())), 
    i16Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(INT16_MIN), Int128(INT16_MAX), 2), stringPool->intern("i16"), declaration.get())),
    i24Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(-0x800000), Int128(0x7FFFFF), 3), stringPool->intern("i24"), declaration.get())),
    i32Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(INT32_MIN), Int128(INT32_MAX), 4), stringPool->intern("i32"), declaration.get())),
    i64Type(scope->createDefinition(nullptr, Definition::BuiltinIntegerType(Int128(INT64_MIN), Int128(INT64_MAX), 8), stringPool->intern("i64"), declaration.get())),
    iexprType(scope->createDefinition(nullptr, Definition::BuiltinIntegerExpressionType(), stringPool->intern("iexpr"), declaration.get())),    
    letType(scope->createDefinition(nullptr, Definition::BuiltinLetType(), stringPool->intern("let"), declaration.get())),
    rangeType(scope->createDefinition(nullptr, Definition::BuiltinRangeType(), stringPool->intern("range"), declaration.get())),
    intrinsicType(scope->createDefinition(nullptr, Definition::BuiltinIntrinsicType(), stringPool->intern("intrinsic"), declaration.get())),
    typeofType(scope->createDefinition(nullptr, Definition::BuiltinTypeOfType(), stringPool->intern("typeof"), declaration.get())),
    hasDef(scope->createDefinition(nullptr, Definition::Let(std::vector<StringView> {stringPool->intern("key")}, nullptr, nullptr), stringPool->intern("__has"), declaration.get())),
    getDef(scope->createDefinition(nullptr, Definition::Let(std::vector<StringView> {stringPool->intern("key"), stringPool->intern("fallback")}, nullptr, nullptr), stringPool->intern("__get"), declaration.get())),
    unitTuple(makeFwdUnique<TypeExpression>(TypeExpression::Tuple({}), declaration->location)) {
        scope->createDefinition(nullptr, Definition::Namespace(scope.get()), stringPool->intern("wiz"), declaration.get());
        scope->createDefinition(nullptr, Definition::BuiltinBankType(BankKind::UninitializedRam), stringPool->intern("vardata"), declaration.get());
        scope->createDefinition(nullptr, Definition::BuiltinBankType(BankKind::InitializedRam), stringPool->intern("varinitdata"), declaration.get());
        scope->createDefinition(nullptr, Definition::BuiltinBankType(BankKind::DataRom), stringPool->intern("constdata"), declaration.get());
        scope->createDefinition(nullptr, Definition::BuiltinBankType(BankKind::ProgramRom), stringPool->intern("prgdata"), declaration.get());
        scope->createDefinition(nullptr, Definition::BuiltinBankType(BankKind::CharacterRom), stringPool->intern("chrdata"), declaration.get());

        addDefineInteger("__version"_sv, Int128(version::ID));

        platform->reserveDefinitions(*this);
    }

    Builtins::~Builtins() {}

    StringPool* Builtins::getStringPool() const {
        return stringPool;
    }

    SymbolTable* Builtins::getBuiltinScope() const {
        return scope.get();
    }

    const Statement* Builtins::getBuiltinDeclaration() const {
        return declaration.get();
    }

    Definition* Builtins::getDefinition(DefinitionType type) const {
        switch (type) {
            default: case Builtins::DefinitionType::None: std::abort(); return nullptr;
            case Builtins::DefinitionType::Bool: return boolType;
            case Builtins::DefinitionType::U8: return u8Type;
            case Builtins::DefinitionType::U16: return u16Type;
            case Builtins::DefinitionType::U24: return u24Type;
            case Builtins::DefinitionType::U32: return u32Type;
            case Builtins::DefinitionType::U64: return u64Type;
            case Builtins::DefinitionType::I8: return i8Type;
            case Builtins::DefinitionType::I16: return i16Type;
            case Builtins::DefinitionType::I24: return i24Type;
            case Builtins::DefinitionType::I32: return i32Type;
            case Builtins::DefinitionType::I64: return i64Type;
            case Builtins::DefinitionType::IExpr: return iexprType;
            case Builtins::DefinitionType::Let: return letType;
            case Builtins::DefinitionType::Range: return rangeType;
            case Builtins::DefinitionType::Intrinsic: return intrinsicType;
            case Builtins::DefinitionType::TypeOf: return typeofType;
            case Builtins::DefinitionType::HasDef: return hasDef;
            case Builtins::DefinitionType::GetDef: return getDef;
        }
    }

    const Expression* Builtins::getDefineExpression(StringView key) const {
        const auto match = defines.find(key);
        if (match != defines.end()) {
            return match->second.get();
        }
        return nullptr;
    }

    void Builtins::addDefineInteger(StringView key, Int128 value) {
        defines.emplace(key,
            makeFwdUnique<const Expression>(Expression::IntegerLiteral(value), declaration->location,
                ExpressionInfo(EvaluationContext::CompileTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(iexprType), declaration->location),
                    Qualifiers {})));
    }

    void Builtins::addDefineBoolean(StringView key, bool value) {
        defines.emplace(key,
            makeFwdUnique<const Expression>(Expression::BooleanLiteral(value), declaration->location,
                ExpressionInfo(EvaluationContext::CompileTime,
                    makeFwdUnique<const TypeExpression>(TypeExpression::ResolvedIdentifier(boolType), declaration->location),
                    Qualifiers {})));
    }

    ArrayView<FwdUniquePtr<const InstructionOperandPattern>> Builtins::getInstructionOperandPatterns() const {
        return ArrayView<FwdUniquePtr<const InstructionOperandPattern>>(instructionOperandPatterns);
    }

    const InstructionOperandPattern* Builtins::addInstructionOperandPattern(FwdUniquePtr<const InstructionOperandPattern> uniquePattern) {
        auto result = uniquePattern.get();
        instructionOperandPatterns.push_back(std::move(uniquePattern));
        return result;
    }

    ArrayView<FwdUniquePtr<const InstructionEncoding>> Builtins::getInstructionEncodings() const {
        return ArrayView<FwdUniquePtr<const InstructionEncoding>>(instructionEncodings);
    }

    const InstructionEncoding* Builtins::addInstructionEncoding(FwdUniquePtr<const InstructionEncoding> uniqueEncoding) {
        auto result = uniqueEncoding.get();
        instructionEncodings.push_back(std::move(uniqueEncoding));
        return result;
    }

    ArrayView<FwdUniquePtr<const Instruction>> Builtins::getInstructions() const {
        return ArrayView<FwdUniquePtr<const Instruction>>(instructions);
    }

    const Instruction* Builtins::addInstruction(FwdUniquePtr<const Instruction> uniqueInstruction) {
        auto& instruction = *uniqueInstruction;
        instructions.push_back(std::move(uniqueInstruction));

        auto& primaryInstructions = primaryInstructionsByInstructionTypes[instruction.signature.type];

        bool specialized = false;

        for (const auto primaryInstruction : primaryInstructions) {
            if (instruction.signature.isSubsetOf(primaryInstruction->signature)) {
                specialized = true;

                auto parent = primaryInstruction;
                retry: {
                    const auto& specializations = specializationsByInstructions[parent];
                    for (const auto specialization : specializations) {
                        if (instruction.signature.isSubsetOf(specialization->signature)) {
                            parent = specialization;
                            goto retry;
                        }
                    }
                }
                
                {
                    auto& specializations = specializationsByInstructions[parent];

                    for (auto it = specializations.begin(); it != specializations.end();) {
                        const auto specialization = *it;
                        if (specialization->signature.isSubsetOf(instruction.signature)) {
                            specializationsByInstructions[&instruction].push_back(specialization);
                            it = specializations.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    specializations.push_back(&instruction);
                }
            }
        }

        if (!specialized) {
            for (auto it = primaryInstructions.begin(); it != primaryInstructions.end();) {
                const auto primaryInstruction = *it;
                if (primaryInstruction->signature.isSubsetOf(instruction.signature)) {
                    specializationsByInstructions[&instruction].push_back(primaryInstruction);
                    it = primaryInstructions.erase(it);
                } else {
                    ++it;
                }
            }

            primaryInstructions.push_back(&instruction);
        }

        return &instruction;
    }

    std::vector<const Instruction*> Builtins::findAllInstructionsByType(const InstructionType& instructionType) const {
        std::vector<const Instruction*> result;

        const auto primaryInstructionsIter = primaryInstructionsByInstructionTypes.find(instructionType);
        if (primaryInstructionsIter != primaryInstructionsByInstructionTypes.end()) {
            const auto& primaryInstructions = primaryInstructionsIter->second;

            for (const auto primaryInstruction : primaryInstructions) {
                result.push_back(primaryInstruction);

                auto specializations = findAllSpecializationsByInstruction(primaryInstruction);
                result.insert(result.end(), specializations.begin(), specializations.end());
            }
        }

        return result;
    }

    std::vector<const Instruction*> Builtins::findAllSpecializationsByInstruction(const Instruction* instruction) const {
        std::vector<const Instruction*> result;

        const auto specializationsIter = specializationsByInstructions.find(instruction);
        if (specializationsIter != specializationsByInstructions.end()) {
            const auto& specializations = specializationsIter->second;
            for (const auto specialization : specializations) {
                result.push_back(specialization);

                auto subSpecializations = findAllSpecializationsByInstruction(specialization);
                result.insert(result.end(), subSpecializations.begin(), subSpecializations.end());
            }
        }

        return result;
    }

    const Instruction* Builtins::selectInstruction(const InstructionType& instructionType, std::uint32_t modeFlags, const std::vector<InstructionOperandRoot>& operandRoots) const {
        const auto primaryInstructionsIter = primaryInstructionsByInstructionTypes.find(instructionType);

        if (primaryInstructionsIter != primaryInstructionsByInstructionTypes.end()) {
            const auto& primaryInstructions = primaryInstructionsIter->second;

            for (const auto primaryInstruction : primaryInstructions) {
                if (primaryInstruction->signature.matches(modeFlags, operandRoots)) {
                    auto bestInstruction = primaryInstruction;
                    retry: {
                        const auto specializationsIter = specializationsByInstructions.find(bestInstruction);
                        if (specializationsIter != specializationsByInstructions.end()) {
                            const auto& specializations = specializationsIter->second;
                            for (const auto specialization : specializations) {
                                if (specialization->signature.matches(modeFlags, operandRoots)) {
                                    bestInstruction = specialization;
                                    goto retry;
                                }
                            }
                        }
                    }
                    
                    return bestInstruction;
                }
            }
        }

        return nullptr;
    }

    void Builtins::addRegisterDecomposition(const Definition* reg, std::vector<Definition*> subRegisters) {
        registerDecompositions[reg] = subRegisters;
    }

    ArrayView<Definition*> Builtins::findRegisterDecomposition(const Definition* reg) const {
        const auto& match = registerDecompositions.find(reg);
        if (match != registerDecompositions.end()) {
            return ArrayView<Definition*>(match->second.data(), match->second.size());
        } else {
            return ArrayView<Definition*>();
        }
    }

    StringView Builtins::getPropertyName(Property prop) const {
        return StringView(propertyNames[static_cast<std::size_t>(prop)]);
    }

    Builtins::Property Builtins::findPropertyByName(StringView name) const {
        static std::unordered_map<StringView, Property> props;
        if (props.empty()) {
            for (std::size_t i = 0; i != sizeof(propertyNames) / sizeof(*propertyNames); ++i) {
                props[StringView(propertyNames[i])] = static_cast<Property>(i);
            }
        }

        const auto match = props.find(name);
        return match != props.end() ? match->second : Property::None;
    }

    Builtins::DeclarationAttribute Builtins::findDeclarationAttributeByName(StringView name) const {
        static std::unordered_map<StringView, DeclarationAttribute> declarationAttributes;
        if (declarationAttributes.empty()) {
            for (std::size_t i = 0; i != sizeof(declarationAttributeNames) / sizeof(*declarationAttributeNames); ++i) {
                declarationAttributes[StringView(declarationAttributeNames[i])] = static_cast<DeclarationAttribute>(i);
            }
        }

        const auto match = declarationAttributes.find(name);
        return match != declarationAttributes.end() ? match->second : DeclarationAttribute::None;
    }

    bool Builtins::isDeclarationAttributeValid(DeclarationAttribute attribute, const Statement* statement) const {
        switch (attribute) {
            case DeclarationAttribute::Irq:
            case DeclarationAttribute::Nmi:
            case DeclarationAttribute::Fallthrough:
                return statement->kind == StatementKind::Func;
            case DeclarationAttribute::Align:
                return statement->kind == StatementKind::Var;
            default: return false;
        }
    }

    std::size_t Builtins::getDeclarationAttributeArgumentCount(DeclarationAttribute attribute) const {
        switch (attribute) {
            case DeclarationAttribute::Irq:
            case DeclarationAttribute::Nmi:
            case DeclarationAttribute::Fallthrough:
                return 0;
            case DeclarationAttribute::Align:
                return 1;
            default: return 0;
        }
    }

    std::size_t Builtins::addModeAttribute(StringView name, std::size_t groupIndex) {
        std::size_t index = modeAttributes.size();
        modeAttributes.push_back(std::make_unique<BuiltinModeAttribute>(name, groupIndex));
        modeAttributesByName[name] = index;
        return index;
    }

    std::size_t Builtins::findModeAttributeByName(StringView name) const {
        const auto match = modeAttributesByName.find(name);
        if (match != modeAttributesByName.end()) {
            return match->second;
        }
        return SIZE_MAX;
    }

    const BuiltinModeAttribute* Builtins::getModeAttribute(std::size_t index) const {
        return modeAttributes[index].get();
    }

    std::size_t Builtins::getModeAttributeCount() const {
        return modeAttributes.size();
    }

    const TypeExpression* Builtins::getUnitTuple() const {
        return unitTuple.get();
    }
}