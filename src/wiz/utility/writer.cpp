#include <wiz/utility/text.h>
#include <wiz/utility/writer.h>

namespace wiz {
    FileWriter::FileWriter(
        StringView filename)
    : filename(filename),
    file(nullptr, [](std::FILE*) { return 0; }) {
        if (const auto f = std::fopen(filename.getData(), "wb")) {
            file = std::unique_ptr<std::FILE, decltype(&std::fclose)>(f, std::fclose);
        }
    }

    FileWriter::~FileWriter() {}

    bool FileWriter::isOpen() const {
        return file != nullptr;
    }

    bool FileWriter::write(const std::vector<std::uint8_t>& data) {
        if (!isOpen()) {
            return false;
        }
        if (data.size() == 0) {
            return true;
        }
        return std::fwrite(&data[0], data.size(), 1, file.get()) == 1;
    }

    bool FileWriter::write(StringView data) {
        if (!isOpen()) {
            return false;
        }
        if (data.size() == 0) {
            return true;
        }
        return std::fwrite(&data[0], data.getLength(), 1, file.get()) == 1;
    }

    bool FileWriter::writeLine(StringView data) {
        if (!isOpen()) {
            return false;
        }
        return write(data)
        && write(text::OsNewLine);
    }


    MemoryWriter::MemoryWriter(std::vector<std::uint8_t>& buffer)
    : buffer(buffer) {}

    MemoryWriter::~MemoryWriter() {}

    bool MemoryWriter::isOpen() const {
        return true;
    }

    bool MemoryWriter::write(const std::vector<std::uint8_t>& data) {
        buffer.insert(buffer.end(), data.begin(), data.end());
        return true;
    }

    bool MemoryWriter::write(StringView data) {
        buffer.insert(buffer.end(), data.begin(), data.end());
        return true;
    }

    bool MemoryWriter::writeLine(StringView data) {
        return write(data)
        && write(text::OsNewLine);
    }
}
    