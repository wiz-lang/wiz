#ifndef WIZ_PLATFORM_PLATFORM_H
#define WIZ_PLATFORM_PLATFORM_H

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>

#include <wiz/utility/string_pool.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/fwd_unique_ptr.h>
#include <wiz/compiler/instruction.h>

namespace wiz {
    class Compiler;
    struct Statement;
    struct Definition;

    class Builtins;

    struct Expression;
    struct InstructionType;
    struct InstructionOperand;

    enum class BinaryOperatorKind;

    struct PlatformBranch {
        PlatformBranch(
            Definition* flag,
            bool value,
            bool success)
        : flag(flag),
        value(value),
        success(success) {}

        Definition* flag;
        bool value;
        bool success;
    };

    struct PlatformTestAndBranch {
        PlatformTestAndBranch(
            const InstructionType& testInstructionType,
            const std::vector<const Expression*>& testOperands,
            std::vector<PlatformBranch> branches)
        : testInstructionType(testInstructionType),
        testOperands(testOperands),
        branches(std::move(branches)) {}

        InstructionType testInstructionType;
        std::vector<const Expression*> testOperands;
        std::vector<PlatformBranch> branches;
    };

    class Platform {
        public:
            virtual ~Platform() {}
            
            virtual void reserveDefinitions(Builtins& builtins) = 0;
            virtual Definition* getPointerSizedType() const = 0;
            virtual Definition* getFarPointerSizedType() const = 0;
            virtual std::unique_ptr<PlatformTestAndBranch> getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const = 0;
            virtual Definition* getZeroFlag() const = 0;
            virtual Int128 getPlaceholderValue() const = 0;
    };

    class PlatformCollection {
        public:
            PlatformCollection();
            ~PlatformCollection();

            std::size_t getPlatformCount() const;
            Platform* getPlatform(std::size_t index) const;
            std::size_t getPlatformNameCount() const;
            StringView getPlatformName(std::size_t index) const;
            void addPlatform(StringView name, std::unique_ptr<Platform> platform);
            void addFileExtension(StringView extension, StringView platformName);
            void addPlatformAlias(StringView aliasName, StringView originalName);
            Platform* findByName(StringView name) const;
            Platform* findByFileExtension(StringView extension) const;

        private:
            std::vector<std::unique_ptr<Platform>> platforms;
            std::vector<StringView> platformNames;
            std::unordered_map<StringView, Platform*> platformsByName;
            std::unordered_map<StringView, Platform*> platformsByFileExtension;
    };
}

#endif
