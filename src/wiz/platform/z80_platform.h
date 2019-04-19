#ifndef WIZ_PLATFORM_Z80_H
#define WIZ_PLATFORM_Z80_H

#include <wiz/platform/platform.h>
#include <wiz/utility/variant.h>

namespace wiz {
    struct Expression;

    class Z80Platform : public Platform {
        public:
            Z80Platform();
            ~Z80Platform() override;

            void reserveDefinitions(Builtins& builtins) override;
            Definition* getPointerSizedType() const override;
            Definition* getFarPointerSizedType() const override;
            std::unique_ptr<PlatformTestAndBranch> getTestAndBranch(const Compiler& compiler, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const override;
            Definition* getZeroFlag() const override;
            Int128 getPlaceholderValue() const override;

        private:
            Definition* pointerSizedType = nullptr;
            Definition* farPointerSizedType = nullptr;

            Definition* a = nullptr;
            Definition* b = nullptr;
            Definition* zero = nullptr;
            Definition* carry = nullptr;
            Definition* cmp = nullptr;
            Definition* bit = nullptr;
            Definition* decrement_branch_not_zero = nullptr;
    };
}

#endif
