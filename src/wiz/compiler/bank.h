#ifndef WIZ_COMPILER_BANK_H
#define WIZ_COMPILER_BANK_H

#include <string>
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <wiz/compiler/address.h>
#include <wiz/utility/optional.h>
#include <wiz/utility/array_view.h>
#include <wiz/utility/string_view.h>
#include <wiz/utility/source_location.h>

namespace wiz {
    class Report;
    struct IrNode;

    struct BankRegionOwner {
        BankRegionOwner()
        : description(),
        node(nullptr),
        location() {}

        BankRegionOwner(
            StringView description,
            const void* node,
            SourceLocation location)
        : description(description),
        node(node),
        location(location) {}

        StringView description;
        const void* node;
        SourceLocation location;
    };

    enum class BankKind {
        None,
        UninitializedRam,
        InitializedRam,
        ProgramRom,
        DataRom,
        CharacterRom,
    };

    bool isBankKindStored(BankKind kind);
    bool isBankKindWritable(BankKind kind);

    class Bank {
        public:
            // Value used to pad unused bank space.
            enum : std::uint8_t { DefaultPadValue = 0xFF };
        
            Bank(
                StringView name,
                BankKind kind,
                Optional<std::size_t> origin,
                std::size_t capacity,
                std::uint8_t padValue);
            ~Bank();


            StringView getName() const;
            BankKind getKind() const;
            std::size_t getCapacity() const;
            Address getAddress() const;
            std::size_t getRelativePosition() const;
            ArrayView<std::uint8_t> getData() const;
            ArrayView<std::uint8_t> getUsedData() const;
            void setRelativePosition(std::size_t dest);

            void rewind();
            bool reserveRam(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size);
            bool reserveRom(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size);
            bool write(Report* report, StringView description, const void* node, SourceLocation location, const std::vector<std::uint8_t>& values);
            bool absoluteSeek(Report* report, std::size_t dest, const SourceLocation& location);

            std::size_t calculateUsedSize() const;

        private:
            std::string getAddressDescription(std::size_t offset);
            bool reserve(Report* report, StringView description, const void* node, SourceLocation location, std::size_t size);

            StringView name;
            BankKind kind;
            Optional<std::size_t> origin;
            std::size_t relativePosition;
            std::size_t capacity;
            std::vector<std::uint8_t> data;
            std::vector<std::size_t> ownership;

            std::unordered_map<const void*, std::size_t> nodesToOwners;
            std::vector<BankRegionOwner> owners;
    };
}

#endif