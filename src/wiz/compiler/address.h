#ifndef WIZ_COMPILER_ADDRESS_H
#define WIZ_COMPILER_ADDRESS_H

#include <cstddef>

#include <wiz/utility/optional.h>

namespace wiz {
    class Bank;

    struct Address {
        Address() = default;
        Address(const Address& other) = default;

        Address(
            Optional<std::size_t> relativePosition,
            Optional<std::size_t> absolutePosition,
            const Bank* bank)
        : relativePosition(relativePosition),
        absolutePosition(absolutePosition),
        bank(bank) {}

        Address& operator =(const Address& other) = default;

        bool operator ==(const Address& other) const {
            return relativePosition.hasValue() == other.relativePosition.hasValue()
            && (!relativePosition.hasValue() || relativePosition.get() == other.relativePosition.get())
            && absolutePosition.hasValue() == other.absolutePosition.hasValue()
            && (!absolutePosition.hasValue() || absolutePosition.get() == other.absolutePosition.get())
            && bank == other.bank;
        }

        bool operator !=(const Address& other) const {
            return !(*this == other);
        }

        Optional<std::size_t> relativePosition;
        Optional<std::size_t> absolutePosition;
        const Bank* bank = nullptr;
    };
}

#endif