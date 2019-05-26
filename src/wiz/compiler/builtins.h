#ifndef WIZ_COMPILER_BUILTINS_H
#define WIZ_COMPILER_BUILTINS_H

#include <vector>
#include <memory>
#include <utility>
#include <unordered_map>

#include <wiz/compiler/instruction.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/fwd_unique_ptr.h>

namespace wiz {
    struct Statement;
    struct Definition;
    struct Expression;
    struct TypeExpression;

    class Platform;
    class StringPool;
    class SymbolTable;

    struct BuiltinModeAttribute {
        BuiltinModeAttribute(
            StringView name,
            std::size_t groupIndex)
        : name(name),
        groupIndex(groupIndex) {}

        StringView name;
        std::size_t groupIndex;
    };

    class Builtins {
        public:
            enum class DefinitionType {
                None,
                Bool,
                U8,
                U16,
                U24,
                U32,
                U64,
                I8,
                I16,
                I24,
                I32,
                I64,
                IExpr,
                Let,
                Range,
                Intrinsic,
                TypeOf,
                HasDef,
                GetDef,
            };

            enum class Property {
                None,
                Len,
                MinValue,
                MaxValue,

                Count
            };

            enum class FunctionAttribute {
                None,
                Irq,
                Nmi,
                Fallthrough,

                Count
            };

            Builtins(
                StringPool* stringPool,
                Platform* platform,
                std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines);
            ~Builtins();

            template <typename... Args>
            InstructionOperandPattern* createInstructionOperandPattern(Args&&... args) {
                return addInstructionOperandPattern(makeFwdUnique<InstructionOperandPattern>(std::forward<Args>(args)...));
            }

            template <typename... Args>
            InstructionEncoding* createInstructionEncoding(Args&&... args) {
                return addInstructionEncoding(makeFwdUnique<InstructionEncoding>(std::forward<Args>(args)...));
            }

            template <typename... Args>
            Instruction* createInstruction(Args&&... args) {
                return addInstruction(makeFwdUnique<Instruction>(std::forward<Args>(args)...));
            }

            StringPool* getStringPool() const;
            SymbolTable* getBuiltinScope() const;
            const Statement* getBuiltinDeclaration() const;
            Definition* getDefinition(DefinitionType type) const;

            const Expression* getDefineExpression(StringView key) const;
            void addDefineInteger(StringView key, Int128 value);
            void addDefineBoolean(StringView key, bool value);

            ArrayView<FwdUniquePtr<InstructionOperandPattern>> getInstructionOperandPatterns() const;
            InstructionOperandPattern* addInstructionOperandPattern(FwdUniquePtr<InstructionOperandPattern> uniquePattern);

            ArrayView<FwdUniquePtr<InstructionEncoding>> getInstructionEncodings() const;
            InstructionEncoding* addInstructionEncoding(FwdUniquePtr<InstructionEncoding> uniqueEncoding);

            ArrayView<FwdUniquePtr<Instruction>> getInstructions() const;
            Instruction* addInstruction(FwdUniquePtr<Instruction> uniqueInstruction);
            std::vector<const Instruction*> findAllInstructionsByType(const InstructionType& instructionType) const;
            std::vector<const Instruction*> findAllSpecializationsByInstruction(const Instruction* instruction) const;
            const Instruction* selectInstruction(const InstructionType& instructionType, std::uint32_t modeFlags, const std::vector<InstructionOperandRoot>& operandRoots) const;

            StringView getPropertyName(Property prop) const;
            Property findPropertyByName(StringView name) const;

            FunctionAttribute findFunctionAttributeByName(StringView name) const;

            std::size_t addModeAttribute(StringView name, std::size_t groupIndex);
            std::size_t findModeAttributeByName(StringView name) const;
            const BuiltinModeAttribute* getModeAttribute(std::size_t index) const;
            std::size_t getModeAttributeCount() const;

            const TypeExpression* getUnitTuple() const;

        private:
            StringPool* stringPool = nullptr;
            Platform* platform = nullptr;
            std::unordered_map<StringView, FwdUniquePtr<const Expression>> defines;

            std::unique_ptr<SymbolTable> scope;
            FwdUniquePtr<const Statement> declaration;

            Definition* boolType = nullptr;
            Definition* u8Type = nullptr;
            Definition* u16Type = nullptr;
            Definition* u24Type = nullptr;
            Definition* u32Type = nullptr;
            Definition* u64Type = nullptr;
            Definition* i8Type = nullptr;
            Definition* i16Type = nullptr;
            Definition* i24Type = nullptr;
            Definition* i32Type = nullptr;
            Definition* i64Type = nullptr;
            Definition* iexprType = nullptr;
            Definition* letType = nullptr;
            Definition* rangeType = nullptr;
            Definition* intrinsicType = nullptr;
            Definition* typeofType = nullptr;
            Definition* hasDef = nullptr;
            Definition* getDef = nullptr;
            FwdUniquePtr<const TypeExpression> unitTuple;

            std::vector<FwdUniquePtr<InstructionOperandPattern>> instructionOperandPatterns;
            std::vector<FwdUniquePtr<InstructionEncoding>> instructionEncodings;
            std::vector<FwdUniquePtr<Instruction>> instructions;
            std::unordered_map<InstructionType, std::vector<const Instruction*>> primaryInstructionsByInstructionTypes;
            std::unordered_map<const Instruction*, std::vector<const Instruction*>> specializationsByInstructions;

            std::vector<std::unique_ptr<BuiltinModeAttribute>> modeAttributes;
            std::unordered_map<StringView, std::size_t> modeAttributesByName;
    };
}

#endif