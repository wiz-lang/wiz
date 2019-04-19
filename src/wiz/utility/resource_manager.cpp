#include <wiz/utility/reader.h>
#include <wiz/utility/writer.h>
#include <wiz/utility/resource_manager.h>
#include <wiz/utility/tty.h>

namespace wiz {
    FileResourceManager::FileResourceManager() {}
    FileResourceManager::~FileResourceManager() {}

    std::unique_ptr<Reader> FileResourceManager::openReader(StringView filename, bool allowShellResources) {
        FileReader file;
#ifndef __EMSCRIPTEN__
        if (allowShellResources && filename == "<stdin>"_sv) {
            if (isTTY(stdin)) {
                return std::make_unique<FileReader>(filename, stdin);
            }
            file = FileReader(filename, stdin);
        } else
#endif
        {
            static_cast<void>(allowShellResources);
            file = FileReader(filename);
        }

        if (file.isOpen()) {
            return std::make_unique<MemoryReader>(file.readFully());
        } else {
            return nullptr;
        }
    }

    std::unique_ptr<Writer> FileResourceManager::openWriter(StringView filename) {
        return std::make_unique<FileWriter>(filename);
    }

    MemoryResourceManager::MemoryResourceManager() {}
    MemoryResourceManager::~MemoryResourceManager() {}

    std::unique_ptr<Reader> MemoryResourceManager::openReader(StringView filename, bool allowShellResources) {
        static_cast<void>(allowShellResources);
        
        const auto match = readBuffers.find(filename);
        if (match != readBuffers.end()) {
            return std::make_unique<MemoryReader>(match->second);
        }
        return nullptr;
    }

    std::unique_ptr<Writer> MemoryResourceManager::openWriter(StringView filename) {
        writeBuffers[filename] = std::vector<std::uint8_t>();
        return std::make_unique<MemoryWriter>(writeBuffers[filename]);
    }

    void MemoryResourceManager::registerReadBuffer(StringView filename, const std::string& buffer) {
        readBuffers[filename] = buffer;
    }

    bool MemoryResourceManager::getReadBuffer(StringView filename, std::string& buffer) {
        const auto match = readBuffers.find(filename);
        if (match != readBuffers.end()) {
            buffer = match->second;
            return true;
        } else {
            return false;
        }
    }

    bool MemoryResourceManager::getWriteBuffer(StringView filename, std::vector<std::uint8_t>& buffer) {
        const auto match = writeBuffers.find(filename);
        if (match != writeBuffers.end()) {
            buffer = match->second;
            return true;
        } else {
            return false;
        }
    }
}