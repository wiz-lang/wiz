#include <algorithm>
#include <iterator>

#include <wiz/utility/reader.h>

namespace wiz {
    FileReader::FileReader()
    : filename(),
    file(nullptr, [](std::FILE*) { return 0; }) {}

    FileReader::FileReader(
        StringView filename)
    : filename(filename),
    file(nullptr, [](std::FILE*) { return 0; }) {
        if (const auto f = std::fopen(filename.getData(), "rb")) {
            file = std::unique_ptr<std::FILE, decltype(&std::fclose)>(f, std::fclose);
        }
    }

    FileReader::FileReader(
        StringView filename,
        std::FILE* file)
    : filename(filename),
    file(file, [](std::FILE*) { return 0; }) {}
    
    FileReader::~FileReader() {}

    bool FileReader::isOpen() const {
        return file != nullptr;
    }

    bool FileReader::readLine(std::string& result) {
        if (!isOpen()) {
            return false;
        }

        result.clear();

        auto f = file.get();
        bool eol = false;
        bool eof = false;
        while (!eol && !eof) {
            char buffer[512];
            if (!std::fgets(buffer, sizeof(buffer), f)) {
                eof = true;
            } else {
                result.append(buffer);

                const auto len = result.length();
                if ((len > 2 && result[len - 2] == '\r' && result[len - 1] == '\n')
                || (len >= 1 && (result[len - 1] == '\r' || result[len - 1] == '\n'))) {
                    eol = true;
                }
            }
        }

        return !eof || result.length() > 0;
    }

    std::string FileReader::readFully() {
        auto f = file.get();
        if (f == stdin) {
            const auto BUFFER_SIZE = 1024;
            char buffer[BUFFER_SIZE];

            std::string result;
            while (std::fgets(buffer, BUFFER_SIZE, stdin) != nullptr) {
                result += buffer;
            }
            return result;
        } else {
            const auto current = std::ftell(f);
            std::fseek(f, 0, SEEK_END);

            const auto end = std::ftell(f);
            const auto size = end - current;
            std::fseek(f, current, SEEK_SET);

            std::string result(size, '\0');
            std::fread(&result[0], size, 1, f);
            return result;
        }
    }

    MemoryReader::MemoryReader(const std::string& buffer)
    : buffer(buffer), offset(0) {}

    MemoryReader::~MemoryReader() {}

    bool MemoryReader::isOpen() const {
        return true;
    }

    bool MemoryReader::readLine(std::string& result) {
        if (offset >= buffer.length()) {
            return false;
        }
     
        bool eof = false;
        const auto old = offset;
        offset = buffer.find_first_of("\r\n", offset);
     
        if (offset >= buffer.length()) {
            offset = buffer.length() - 1;
            eof = true;
        } else {
            if (offset < buffer.length() - 1
            && buffer[offset] == '\r'
            && buffer[offset + 1] == '\n') {
                offset++;
            }
        }
     
        offset++;
        result = buffer.substr(old, offset - old);

        return !eof || result.length() > 0;
    }

    std::string MemoryReader::readFully() {
        if (offset >= buffer.length()) {
            return "";
        }
        const auto origin = offset;
        offset = buffer.length();
        return buffer.substr(origin, buffer.length());
    }
}