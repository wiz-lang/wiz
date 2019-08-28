#include <wiz/compiler/bank.h>
#include <wiz/compiler/ir_node.h>
#include <wiz/utility/report.h>
#include <wiz/utility/int128.h>
#include <wiz/utility/writer.h>

namespace wiz {
    bool isBankKindStored(BankKind kind) {
        switch (kind) {
            case BankKind::None:
            case BankKind::UninitializedRam:
                return false;
            case BankKind::InitializedRam:
            case BankKind::ProgramRom: 
            case BankKind::DataRom:
            case BankKind::CharacterRom:
                return true;
            default: assert(false); std::abort();
        }
    }

    bool isBankKindWritable(BankKind kind) {
        switch (kind) {
            case BankKind::InitializedRam:
            case BankKind::UninitializedRam:
                return true;
            case BankKind::None:
            case BankKind::ProgramRom: 
            case BankKind::DataRom:
            case BankKind::CharacterRom:
                return false;
            default: assert(false); std::abort();
        }
    }

    Bank::Bank(
        StringView name,
        BankKind kind,
        Optional<std::size_t> origin,
        std::size_t capacity,
        std::uint8_t padValue)
    : name(name),
    kind(kind),
    origin(origin),
    relativePosition(0),
    capacity(capacity),
    data(isBankKindStored(kind) ? capacity : 0, padValue),
    ownership(capacity) {}

    Bank::~Bank() {}

    StringView Bank::getName() const {
        return name;
    }

    BankKind Bank::getKind() const {
        return kind;
    }

    std::size_t Bank::getCapacity() const {
        return capacity;
    }

    Address Bank::getAddress() const {
        return Address(
            relativePosition,
            origin.hasValue() ? origin.get() + relativePosition : Optional<std::size_t>(),
            this
        );
    }

    std::size_t Bank::getRelativePosition() const {
        return relativePosition;
    }

    void Bank::setRelativePosition(std::size_t dest) {
        relativePosition = dest;
    }

    ArrayView<std::uint8_t> Bank::getData() const {
        return ArrayView<std::uint8_t>(data);
    }

    ArrayView<std::uint8_t> Bank::getUsedData() const {
        return ArrayView<std::uint8_t>(data.data(), calculateUsedSize());
    }

    void Bank::rewind() {
        relativePosition = 0;
    }

    bool Bank::reserveRam(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size) {
        if (!isBankKindWritable(kind)) {
            report->error(description.toString() + " requires a writable region, which is not allowed in readonly bank `" + name.toString() + "`", location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
            return false;
        } else {
            return reserve(report, description, node, location, size);
        }
    }

    bool Bank::reserveRom(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size) {
        if (!isBankKindStored(kind)) {
            report->error(description.toString() + " requires initialized data, which is not allowed in volatile bank `" + name.toString() + "`", location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
            return false;
        } else {
            return reserve(report, description, node, location, size);
        }
    }

    bool Bank::write(Report* report, StringView description, const void* node, SourceLocation location, const std::vector<std::uint8_t>& values) {
        const auto size = values.size();
        if (relativePosition + size > capacity) {
            report->error(description.toString() + " needs " + std::to_string(size)
                + " byte(s), which exceeds the remaining space in bank `" + name.toString()
                + "` by " + std::to_string(relativePosition + size - capacity) + " byte(s)",
                location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
            return false;
        }

        const auto match = nodesToOwners.find(node);
        if (match == nodesToOwners.end()) {
            report->error("attempt to write to " + getAddressDescription(relativePosition)
                + " in bank `" + name.toString()
                + "`, with " + description.toString()
                + " that never reserved any space for itself",
                location, ReportErrorFlags::of<ReportErrorFlagType::Fatal, ReportErrorFlagType::InternalError>());
            return false;
        }

        const auto ownerID = match->second;
        for (std::size_t i = 0; i != size; ++i) {
            if (ownership[relativePosition + i] != ownerID) {
                report->error("write conflict encountered at " + getAddressDescription(relativePosition + i)
                    + " while attempting to write byte " + std::to_string(i) + " of " + std::to_string(size)
                    + " byte(s) for " + description.toString(),
                    location, ReportErrorFlags::of<ReportErrorFlagType::InternalError, ReportErrorFlagType::Continued>());

                if (const auto previousID = ownership[relativePosition + i]) {
                    const auto& previous = owners[previousID - 1];
                    report->error("address was supposed to be reserved here, by " + previous.description.toString(), previous.location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                } else {
                    report->error("address was never reserved when it was supposed to be", location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                }
                return false;
            }
        }

        for (const auto& value : values) {
            data[relativePosition++] = value;
        }
        return true;
    }

    bool Bank::absoluteSeek(Report* report, std::size_t dest, const SourceLocation& location) {
        if (origin.hasValue()) {
            const auto originValue = origin.get();

            if (dest < originValue || dest >= originValue + capacity) {
                report->error("attempt to seek to invalid address `0x" + Int128(dest).toString(16) 
                    + "` in bank `" + name.toString()
                    + "`, which exceeds its address range of `0x" + Int128(originValue).toString(16)
                    + "` .. `" + Int128(originValue + capacity - 1).toString(16) + "`",
                    location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                return false;
            }

            relativePosition = dest - originValue;
            return true;
        } else {
            origin = dest;
            return true;
        }
    }

    std::size_t Bank::calculateUsedSize() const {
        for (std::size_t i = ownership.size() - 1; i < ownership.size(); --i) {
            if (ownership[i] != 0) {
                return i + 1;
            }
        }
        return 0;
    }

    std::string Bank::getAddressDescription(std::size_t offset) {
        if (origin.hasValue()) {
            return "absolute address 0x" + Int128(origin.get() + offset).toString(16);
        } else {
            return "relative position " + std::to_string(offset);
        }
    }

    bool Bank::reserve(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size) {
        if (relativePosition + size > capacity) {
            report->error(description.toString() + " needs " + std::to_string(size)
                + " byte(s), which exceeds the remaining space in bank `" + name.toString()
                + "` by " + std::to_string(relativePosition + size - capacity) + " byte(s)",
                location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
            return false;
        }

        const auto match = nodesToOwners.find(node);
        std::size_t ownerID;
        if (match == nodesToOwners.end()) {
            owners.push_back(BankRegionOwner(description, node, location));
            ownerID = owners.size();
            nodesToOwners[node] = ownerID;
        } else {
            ownerID = match->second;
        }

        for (std::size_t i = 0; i != size; ++i) {
            if (const auto previousID = ownership[relativePosition + i]) {
                const auto& previous = owners[previousID - 1];
                report->error("overlap conflict encountered at " + getAddressDescription(relativePosition + i)
                    + " while reserving byte " + std::to_string(i) + " of " + std::to_string(size)
                    + " byte(s) needed for " + description.toString(),
                    location, ReportErrorFlags::of<ReportErrorFlagType::Continued>());
                report->error("address was previously reserved here, by " + previous.description.toString(), previous.location, ReportErrorFlags::of<ReportErrorFlagType::Fatal>());
                return false;
            }

            ownership[relativePosition + i] = ownerID;
        }

        relativePosition += size;
        return true;
    }
}