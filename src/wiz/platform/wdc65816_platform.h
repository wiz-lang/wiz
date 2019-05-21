#ifndef WIZ_PLATFORM_WDC65816_H
#define WIZ_PLATFORM_WDC65816_H

#include <wiz/platform/platform.h>
#include <wiz/utility/variant.h>

namespace wiz {
    class Wdc65816Platform : public Platform {
        public:
            Wdc65816Platform();
            ~Wdc65816Platform() override;

            void reserveDefinitions(Builtins& builtins) override;
            Definition* getPointerSizedType() const override;
            Definition* getFarPointerSizedType() const override;
            std::unique_ptr<PlatformTestAndBranch> getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const override;
            Definition* getZeroFlag() const override;
            Int128 getPlaceholderValue() const override;

        private:
            std::uint32_t modeMem8 = 0;
            std::uint32_t modeMem16 = 0;
            std::uint32_t modeIdx8 = 0;
            std::uint32_t modeIdx16 = 0;
            std::uint32_t modeRel = 0;
            std::uint32_t modeAbs = 0;

            Definition* pointerSizedType = nullptr;
            Definition* farPointerSizedType = nullptr;

            Definition* a = nullptr;
            Definition* aa = nullptr;
            Definition* x = nullptr;
            Definition* xx = nullptr;
            Definition* y = nullptr;
            Definition* yy = nullptr;
            Definition* zero = nullptr;
            Definition* carry = nullptr;
            Definition* nointerrupt = nullptr;
            Definition* decimal = nullptr;
            Definition* overflow = nullptr;
            Definition* negative = nullptr;

            Definition* cmp = nullptr;
            Definition* bit = nullptr;
    };
}

#endif
