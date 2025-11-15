#pragma once

// clang-format off
#include <windows.h>
#include <dbghelp.h>
//clang-format on

#include <csignal>
#include <iostream>
#include <string>
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

    std::cerr << "Stack trace (" << std::dec << static_cast<int>(frames) << " frames):\n";
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

    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
        DWORD accessType = static_cast<DWORD>(er->ExceptionInformation[0]);
        LPCVOID address =
            reinterpret_cast<LPCVOID>(er->ExceptionInformation[1]);

        switch (accessType) {
            case 0:
                std::cerr << "Access violation reading ";
                break;
            case 1:
                std::cerr << "Access violation writing ";
                break;
            case 8:
                std::cerr << "Data Execution Prevention violation at ";
                break;
            default:
                std::cerr << "Unknown value! This should not happen!";
                break;
        }

        std::cerr << "address " << address;
        if (static_cast<uintptr_t>(er->ExceptionInformation[1]) >
            0x00007FFFFFFFFFFF) {
            std::cerr << " (invalid)\n";
        }
        else {
            std::cerr << "\n";
        }

        if (er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR &&
            er->NumberParameters >= 3) {
            DWORD ntstatus = static_cast<DWORD>(er->ExceptionInformation[2]);
            std::cerr << "Underlying NTSTATUS: 0x" << std::hex << ntstatus
                      << std::dec << "\n";
        }
    }

    std::string name;
    std::string output;
    switch (er->ExceptionCode) {
        case EXCEPTION_ACCESS_VIOLATION: {
            name = "Access violation";
            output = "The thread tried to read from or write to a virtual address for which it does not have the appropriate access.";
            break;
        }
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: {
            name = "Array bounds exceeded";
            output = "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.";
            break;
        }
        case EXCEPTION_BREAKPOINT: {
            name = "Breakpoint";
            output = "A breakpoint was encountered.";
            break;
        }
        case EXCEPTION_DATATYPE_MISALIGNMENT: {
            name = "Datatype misalignment";
            output = "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries.";
            break;
        }
        case EXCEPTION_FLT_DENORMAL_OPERAND: {
            name = "Float denormal operand";
            output = "One of the operands in a floating-point operation is denormal. A denormal value is too small to represent as a standard floating-point value.";
            break;
        }
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: {
            name = "Float divide by zero";
            output = "The thread tried to divide a floating-point value by a floating-point divisor of zero.";
            break;
        }
        case EXCEPTION_FLT_INEXACT_RESULT: {
            name = "Float inexact result";
            output = "The result of a floating-point operation cannot be represented exactly as a decimal fraction.";
            break;
        }
        case EXCEPTION_FLT_INVALID_OPERATION: {
            name = "Float invalid operation";
            output = "This exception represents any floating-point exception not included in this list.";
            break;
        }
        case EXCEPTION_FLT_OVERFLOW: {
            name = "Float overflow";
            output = "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.";
            break;
        }
        case EXCEPTION_FLT_STACK_CHECK: {
            name = "Float stack check";
            output = "The stack overflowed or underflowed as the result of a floating-point operation.";
            break;
        }
        case EXCEPTION_FLT_UNDERFLOW: {
            name = "Float underflow";
            output = "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.";
            break;
        }
        case EXCEPTION_ILLEGAL_INSTRUCTION: {
            name = "Illegal instruction";
            output = "The thread tried to execute an invalid instruction.";
            break;
        }
        case EXCEPTION_IN_PAGE_ERROR: {
            name = "In page error";
            output = "The thread tried to access a page that was not present, and the system was unable to load the page. This can occur if a network connection is lost while running a program over the network.";
            break;
        }
        case EXCEPTION_INT_DIVIDE_BY_ZERO: {
            name = "Integer division by zero";
            output = "The thread tried to divide an integer value by an integer divisor of zero.";
            break;
        }
        case EXCEPTION_INT_OVERFLOW: {
            name = "Integer overflow";
            output = "The result of an integer operation caused a carry out of the most significant bit of the result.";
            break;
        }
        case EXCEPTION_INVALID_DISPOSITION: {
            name = "Invalid disposition";
            output = "An exception handler returned an invalid disposition to the exception dispatcher. High-level language programmers should not encounter this.";
            break;
        }
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: {
            name = "Noncontinuable exception";
            output = "The thread tried to continue execution after a noncontinuable exception occurred.";
            break;
        }
        case EXCEPTION_PRIV_INSTRUCTION: {
            name = "Private instruction";
            output = "The thread tried to execute an instruction whose operation is not allowed in the current machine mode.";
            break;
        }
        case EXCEPTION_SINGLE_STEP: {
            name = "Single step";
            output = "A trace trap or other single-instruction mechanism signaled that one instruction has been executed.";
            break;
        }
        case EXCEPTION_STACK_OVERFLOW: {
            name = "Stack overflow";
            output = "The thread used up its stack.";
            break;
        }
        default: {
            name = "Unknown";
            output = "Unknown exception code (" + std::to_string(er->ExceptionCode) + ")";
            break;
        }
    }

    std::cerr << "\n" << name << ": " << output << "\n\n";

    std::cerr << "Exception code: 0x" << std::hex << er->ExceptionCode
              << std::dec << "\n";
    std::cerr << "Exception flags: 0x" << std::hex << er->ExceptionFlags
              << std::dec << "\n";
    std::cerr << "Exception address: " << er->ExceptionAddress << "\n";
    std::cerr << "Number of parameters: " << er->NumberParameters << "\n";

    for (DWORD i = 0; i < er->NumberParameters; ++i) {
        std::cerr << "  Parameter[" << i << "]: 0x" << std::hex
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
    std::cerr << "RIP: 0x" << std::hex << ctx->Rip << "\n\n";

    stacktrace();

    LeaveCriticalSection(&cs);

    std::exit(er->ExceptionCode);
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
