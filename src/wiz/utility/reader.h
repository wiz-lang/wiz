#ifndef WIZ_UTILITY_READER_H
#define WIZ_UTILITY_READER_H

#include <string>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <wiz/utility/string_view.h>

namespace wiz {
    class Reader {
        public:
            virtual ~Reader() {}
            virtual bool isOpen() const = 0;
            virtual bool readLine(std::string& result) = 0;
            virtual std::string readFully() = 0;
    };

    class FileReader : public Reader {
        public:
            FileReader();
            FileReader(StringView filename);
            FileReader(StringView filename, std::FILE* file);
            FileReader(FileReader&& reader) = default;
            virtual ~FileReader() override;

            FileReader& operator =(FileReader&& reader) = default;

            virtual bool isOpen() const override;
            virtual bool readLine(std::string& result) override;
            virtual std::string readFully() override;

        private:
            FileReader(const FileReader&) = delete;  
            FileReader& operator=(const FileReader&) = delete;

            StringView filename;
            std::unique_ptr<std::FILE, decltype(&std::fclose)> file;
    };

    class MemoryReader : public Reader {
        public:
            MemoryReader(const std::string& buffer);
            virtual ~MemoryReader() override;
            
            virtual bool isOpen() const override;
            virtual bool readLine(std::string& result) override;
            virtual std::string readFully() override;
        private:
            std::string buffer;
            std::size_t offset;
    };
}

#endif
