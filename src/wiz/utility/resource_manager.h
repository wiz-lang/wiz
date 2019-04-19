#ifndef WIZ_UTILITY_RESOURCE_SYSTEM_H
#define WIZ_UTILITY_RESOURCE_SYSTEM_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <wiz/utility/string_view.h>

namespace wiz {
    class Reader;
    class Writer;
    class ResourceManager {
        public:
            virtual ~ResourceManager() {}
            virtual std::unique_ptr<Reader> openReader(StringView filename, bool allowShellResources) = 0;
            virtual std::unique_ptr<Writer> openWriter(StringView filename) = 0;
    };

    class FileResourceManager : public ResourceManager {
        public:
            FileResourceManager();
            virtual ~FileResourceManager() override;

            virtual std::unique_ptr<Reader> openReader(StringView filename, bool allowShellResources) override;
            virtual std::unique_ptr<Writer> openWriter(StringView filename) override;
    };

    class MemoryResourceManager : public ResourceManager {
        public:
            MemoryResourceManager();
            virtual ~MemoryResourceManager() override;

            virtual std::unique_ptr<Reader> openReader(StringView filename, bool allowShellResources) override;
            virtual std::unique_ptr<Writer> openWriter(StringView filename) override;

            void registerReadBuffer(StringView filename, const std::string& buffer);
            bool getReadBuffer(StringView filename, std::string& result);
            bool getWriteBuffer(StringView filename, std::vector<std::uint8_t>& result);

        private:
            std::unordered_map<StringView, std::string> readBuffers;
            std::unordered_map<StringView, std::vector<std::uint8_t>> writeBuffers;
    };
}

#endif
