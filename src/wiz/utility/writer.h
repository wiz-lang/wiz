#ifndef WIZ_UTILITY_WRITER_H
#define WIZ_UTILITY_WRITER_H

#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <wiz/utility/string_view.h>

namespace wiz {
    enum class WriterMode {
        Text,
        Binary,
    };

    class Writer {
        public:
            virtual ~Writer() {}
            virtual bool isOpen() const = 0;
            virtual bool write(const std::vector<std::uint8_t>& data) = 0;
            virtual bool write(StringView data) = 0;
            virtual bool writeLine(StringView data) = 0;
    };

    class FileWriter : public Writer {
        public:
            FileWriter(StringView filename);
            ~FileWriter() override;

            bool isOpen() const override;
            bool write(const std::vector<std::uint8_t>& data) override;
            bool write(StringView data) override;
            bool writeLine(StringView data) override;

        private:
            FileWriter(const FileWriter&) = delete;  
            FileWriter& operator=(const FileWriter&) = delete;

            StringView filename;
            std::unique_ptr<std::FILE, decltype(&std::fclose)> file;
    };

    class MemoryWriter : public Writer {
        public:
            MemoryWriter(std::vector<std::uint8_t>& buffer);
            ~MemoryWriter() override;
            
            bool isOpen() const override;
            bool write(const std::vector<std::uint8_t>& data) override;
            bool write(StringView data) override;
            bool writeLine(StringView data) override;

        private:
            std::vector<std::uint8_t>& buffer;
    };
}

#endif
