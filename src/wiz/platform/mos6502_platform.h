#ifndef WIZ_PLATFORM_MOS6502_H
#define WIZ_PLATFORM_MOS6502_H

#include <wiz/platform/platform.h>

namespace wiz {
    class Mos6502Platform : public Platform {
        public:
            enum class Revision {
                Base6502,
                Base65C02,
                Wdc65C02,
                Rockwell65C02,
                Huc6280,
            };

            Mos6502Platform(Revision revision);
            ~Mos6502Platform() override;

            void reserveDefinitions(Builtins& builtins) override;
            Definition* getPointerSizedType() const override;
            Definition* getFarPointerSizedType() const override;
            std::unique_ptr<PlatformTestAndBranch> getTestAndBranch(const Compiler& compiler, const Definition* type, BinaryOperatorKind op, const Expression* left, const Expression* right, std::size_t distanceHint) const override;
            Definition* getZeroFlag() const override;
            Int128 getPlaceholderValue() const override;

        private:
            Revision revision;

            Definition* pointerSizedType = nullptr;
            Definition* farPointerSizedType = nullptr;

            Definition* a = nullptr;
            Definition* x = nullptr;
            Definition* y = nullptr;
            Definition* zero = nullptr;
            Definition* carry = nullptr;
            Definition* nointerrupt = nullptr;
            Definition* decimal = nullptr;
            Definition* overflow = nullptr;
            Definition* negative = nullptr;

            Definition* cmp = nullptr;
            Definition* bit = nullptr;
            Definition* tst = nullptr;
    };
}

#endif
