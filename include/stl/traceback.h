#pragma once

// clang-format off
#include <windows.h>
#include <dbghelp.h>
//clang-format on

#include <csignal>
#include <iostream>
#include <thread>

namespace stl {

void stacktrace() {
    void* stack[62];
    USHORT frames = CaptureStackBackTrace(0, 62, stack, nullptr);
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, nullptr, TRUE);

    SYMBOL_INFO* symbol =
        (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    IMAGEHLP_LINE64 line;
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD displacement;

    std::cerr << "Stack trace (" << frames << " frames):\n";
    for (USHORT i = 0; i < frames; ++i) {
        if (SymFromAddr(process, reinterpret_cast<DWORD64>(stack[i]), nullptr, symbol)) {
            if (SymGetLineFromAddr64(
                    process, reinterpret_cast<DWORD64>(stack[i]), &displacement, &line)) {
                std::cerr << "  " << i << ": " << symbol->Name << " - "
                          << line.FileName << ":" << line.LineNumber << "\n";
            } else {
                std::cerr << "  " << i << ": " << symbol->Name
                          << " - [line info unavailable]\n";
            }
        } else {
            std::cerr << "  " << i << ": [symbol unavailable]\n";
        }
    }

    free(symbol);
}

void exception_handler() {
    std::cerr << "===  TERMINATE HANDLER ===\n";
    std::cerr << "Thread ID: " << std::this_thread::get_id() << "\n";

    std::exception_ptr exception = std::current_exception();
    if (exception) {
        try {
            std::rethrow_exception(exception);
        } catch (const std::exception& e) {
            std::cerr << "Caught std::exception\n";
            std::cerr << "Type: " << typeid(e).name() << "\n";
            std::cerr << "what(): " << e.what() << "\n";
        } catch (const char* s) {
            std::cerr << "Caught C-string exception: " << s << "\n";
        } catch (int code) {
            std::cerr << "Caught int exception with code: " << code << "\n";
        } catch (...) {
            std::cerr << "Caught unknown non-standard exception\n";
        }
    } else {
        std::cerr << "No active exception; probably because this was called manually.\n";
    }

    stacktrace();
    std::abort();
}

LONG WINAPI sehHandler(EXCEPTION_POINTERS* pException) {
    static CRITICAL_SECTION cs;
    static bool csInitialized = false;

    if (!csInitialized) {
        InitializeCriticalSection(&cs);
        csInitialized = true;
    }

    EnterCriticalSection(&cs);

    EXCEPTION_RECORD* er = pException->ExceptionRecord;
    CONTEXT* ctx = pException->ContextRecord;

    std::cerr << "=== WIN32 STRUCTURED EXCEPTION HANDLER ===\n";

    std::cerr << "Exception code: 0x" << std::hex << er->ExceptionCode
              << std::dec << "\n";
    std::cerr << "Exception flags: 0x" << std::hex << er->ExceptionFlags
              << std::dec << "\n";
    std::cerr << "Exception address: " << er->ExceptionAddress << "\n";
    std::cerr << "Number of parameters: " << er->NumberParameters << "\n";

    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
        std::cerr << "\n=== Access Violation / In-Page Error Details ===\n";
        DWORD accessType = static_cast<DWORD>(er->ExceptionInformation[0]);
        LPCVOID address =
            reinterpret_cast<LPCVOID>(er->ExceptionInformation[1]);

        std::cerr << "Operation: ";
        switch (accessType) {
        case 0:
            std::cerr << "Read";
            break;
        case 1:
            std::cerr << "Write";
            break;
        case 8:
            std::cerr << "DEP (Data Execution Prevention) violation";
            break;
        default:
            std::cerr << "Unknown";
            break;
        }
        std::cerr << "\n";

        std::cerr << "Address: " << address << "\n";
        if (static_cast<uintptr_t>(er->ExceptionInformation[1]) >
            0x00007FFFFFFFFFFF) {
            std::cerr << "Address is invalid\n";
        }

        if (er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR &&
            er->NumberParameters >= 3) {
            DWORD ntstatus = static_cast<DWORD>(er->ExceptionInformation[2]);
            std::cerr << "Underlying NTSTATUS: 0x" << std::hex << ntstatus
                      << std::dec << "\n";
        }
    }

    for (DWORD i = 0; i < er->NumberParameters; ++i) {
        std::cerr << "Parameter[" << i << "]: 0x" << std::hex
                  << er->ExceptionInformation[i] << std::dec << "\n";
    }

    std::cerr << "\n=== CPU Context ===\n";

    std::cerr << "RAX: 0x" << std::hex << ctx->Rax << "\n";
    std::cerr << "RBX: 0x" << std::hex << ctx->Rbx << "\n";
    std::cerr << "RCX: 0x" << std::hex << ctx->Rcx << "\n";
    std::cerr << "RDX: 0x" << std::hex << ctx->Rdx << "\n";
    std::cerr << "RSI: 0x" << std::hex << ctx->Rsi << "\n";
    std::cerr << "RDI: 0x" << std::hex << ctx->Rdi << "\n";
    std::cerr << "RBP: 0x" << std::hex << ctx->Rbp << "\n";
    std::cerr << "RSP: 0x" << std::hex << ctx->Rsp << "\n";
    std::cerr << "RIP: 0x" << std::hex << ctx->Rip << "\n";

    stacktrace();

    LeaveCriticalSection(&cs);

    std::terminate();
}

void signal_handler(int signal) {
    std::cerr << "=== Signal Handler ===\n";
    std::string name = "Unknown";
    switch (signal) {
    case SIGSEGV:
        name = "SIGSEGV (Segmentation Fault)";
        break;
    case SIGABRT:
        name = "SIGABRT (Abort)";
        break;
    case SIGFPE:
        name = "SIGFPE (Floating Point Exception)";
        break;
    case SIGILL:
        name = "SIGILL (Illegal Instruction)";
        break;
    case SIGTERM:
        name = "SIGTERM (Termination)";
        break;
    case SIGINT:
        name = "SIGINT (Interrupt)";
        break;
    }

    std::cerr << "Crashed with " << name << " (" << signal << ")\n";
    stacktrace();
}

void install_exception_handler() {
    std::set_terminate(exception_handler);

    SetUnhandledExceptionFilter(sehHandler);

    signal(SIGSEGV, signal_handler);
    signal(SIGABRT, signal_handler);
    signal(SIGFPE, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
}
} // namespace stl
