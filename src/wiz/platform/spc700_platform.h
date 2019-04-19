#ifndef WIZ_PLATFORM_SPC700_H
#define WIZ_PLATFORM_SPC700_H

#include <wiz/platform/platform.h>
#include <wiz/utility/variant.h>

namespace wiz {
    struct Expression;
    struct Instruction;

    class Spc700Platform : public Platform {
        public:
            Spc700Platform();
            ~Spc700Platform() override;

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
			Definition* x = nullptr;
			Definition* y = nullptr;
			Definition* ya = nullptr;
			Definition* negative = nullptr;
			Definition* overflow = nullptr;
			Definition* direct_page = nullptr;
			Definition* break_flag = nullptr;
			Definition* half_carry = nullptr;
			Definition* interrupt = nullptr;
			Definition* zero = nullptr;
			Definition* carry = nullptr;
			Definition* cmp = nullptr;
			Definition* compare_branch_not_equal = nullptr;
			Definition* decrement_branch_not_zero = nullptr;

            Instruction* cbneDirect = nullptr;
            Instruction* cbneDirectIndexed = nullptr;
    };
}

#endif
