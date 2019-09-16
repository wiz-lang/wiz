#ifdef _WIN32

#include <wiz/utility/win32.h>

#include <wchar.h>
#include <winternl.h>

namespace wiz {
    namespace {
        enum OBJECT_INFORMATION_CLASS {
            ObjectBasicInformation, 
            ObjectNameInformation,
            ObjectTypeInformation, 
            ObjectAllInformation,
            ObjectDataInformation
        };

        struct UNICODE_STRING {
            USHORT Length;
            USHORT MaximumLength;
            PWSTR Buffer;
        };

        struct OBJECT_NAME_INFORMATION {
            UNICODE_STRING Name;
            WCHAR NameBuffer[1];
        };

        using NtQueryObjectProc = NTSTATUS (NTAPI*)(HANDLE Handle, OBJECT_INFORMATION_CLASS Info, PVOID Buffer, ULONG BufferSize, PULONG ReturnLength);

        NtQueryObjectProc getNtQueryObjectProc() {
            static NtQueryObjectProc proc = nullptr;
            if (proc == nullptr) {
                proc = reinterpret_cast<NtQueryObjectProc>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandle(TEXT("Ntdll.dll")), "NtQueryObject")));
            }
            return proc;
        }

        DWORD getStdHandleIDForFile(std::FILE* file) {
            if (file == stdout) {
                return STD_OUTPUT_HANDLE;
            } else if (file == stderr) {
                return STD_ERROR_HANDLE;
            } else if (file == stdin) {
                return STD_INPUT_HANDLE;
            }
            return 0;
        }
    }

    bool isMinTTY(HANDLE handle) {
        char nameInfoBuffer[1024];
        auto nameInfo = reinterpret_cast<OBJECT_NAME_INFORMATION*>(nameInfoBuffer);
        ULONG returnLength = 0;

        if (!NT_SUCCESS(getNtQueryObjectProc()(handle, ObjectNameInformation, nameInfoBuffer, sizeof(nameInfoBuffer), &returnLength))) {
            return false;
        }

        auto name = nameInfo->Name.Buffer;
        name[nameInfo->Name.Length / sizeof(WCHAR)] = 0;

        const auto msysPrefixMatch = wcsstr(name, L"msys-");
        const auto cygwinPrefixMatch = wcsstr(name, L"cygwin-");
        if (const auto prefixMatch = msysPrefixMatch != nullptr ? msysPrefixMatch : cygwinPrefixMatch) {
            if (const auto ptyMatch = wcsstr(prefixMatch, L"-pty")) {
                if (const auto masterMatch = wcsstr(ptyMatch, L"-master")) {
                    return true;
                }
            }
        }

        return false;
    }

    HANDLE getStdHandleForFile(std::FILE* file) {
        if (const auto handleID = getStdHandleIDForFile(file)) {
            const auto handle = GetStdHandle(handleID);
            return handle != INVALID_HANDLE_VALUE ? handle : nullptr;
        }
        return nullptr;
    }
}

#endif