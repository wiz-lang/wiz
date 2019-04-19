#ifndef WIZ_COMPILER_ADDRESS_H
#define WIZ_COMPILER_ADDRESS_H

#include <cstddef>

#include <wiz/utility/optional.h>

namespace wiz {
    struct Address {
        Address()
        : relativePosition(),
        absolutePosition() {}

        Address(
            Optional<std::size_t> relativePosition,
            Optional<std::size_t> absolutePosition)
        : relativePosition(relativePosition),
        absolutePosition(absolutePosition) {}

        Address(const Address& other)
        : relativePosition(other.relativePosition),
        absolutePosition(other.absolutePosition) {}

        Address& operator =(const Address& other) {
            if (this != &other) {                
                relativePosition = other.relativePosition;
                absolutePosition = other.absolutePosition;
            }
            return *this;
        }

        bool operator ==(const Address& other) const {
            if (relativePosition.hasValue() != other.relativePosition.hasValue()) {
                return false;
            }
            if (relativePosition.hasValue() && relativePosition.get() != other.relativePosition.get()) {
                return false;
            }
            if (absolutePosition.hasValue() != other.absolutePosition.hasValue()) {
                return false;
            }
            if (absolutePosition.hasValue() && absolutePosition.get() != other.absolutePosition.get()) {
                return false;
            }
            return true;
        }

        bool operator !=(const Address& other) const {
            return !(*this == other);
        }

        Optional<std::size_t> relativePosition;
        Optional<std::size_t> absolutePosition;
    };
}

#endif