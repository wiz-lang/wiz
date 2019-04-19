#ifndef WIZ_UTILITY_WRITER_H
#define WIZ_UTILITY_WRITER_H

#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <wiz/utility/string_view.h>

namespace wiz {
    class Writer {
        public:
            virtual ~Writer() {}
            virtual bool isOpen() const = 0;
            virtual bool write(const std::vector<std::uint8_t>& data) = 0;
    };

    class FileWriter : public Writer {
        public:
            FileWriter(StringView filename);
            virtual ~FileWriter() override;

            virtual bool isOpen() const override;
            virtual bool write(const std::vector<std::uint8_t>& data) override;

        private:
            FileWriter(const FileWriter&) = delete;  
            FileWriter& operator=(const FileWriter&) = delete;

            StringView filename;
            std::unique_ptr<std::FILE, decltype(&std::fclose)> file;
    };

    class MemoryWriter : public Writer {
        public:
            MemoryWriter(std::vector<std::uint8_t>& buffer);
            virtual ~MemoryWriter() override;
            
            virtual bool isOpen() const override;
            virtual bool write(const std::vector<std::uint8_t>& data) override;

        private:
            std::vector<std::uint8_t>& buffer;
    };
}

#endif
