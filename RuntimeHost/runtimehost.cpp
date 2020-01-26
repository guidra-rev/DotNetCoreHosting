#include "pch.h"
#include "framework.h"
#include <sstream>

#include "coreclr_delegates.h"
#include "hostfxr.h"
#include "nethost.h"
#include "error_codes.h"

#include "runtimehost.h"


extern "C" {
    __declspec(dllexport) int __stdcall EntryPoint(EntryPointParams* managed)
    {
        // Load nethost.dll
        HMODULE handleNethost{ LoadLibraryW(managed->nethostPath) };
        if (handleNethost == NULL)
            return DisplayError(L"Failed loading nethost.dll");

        // Get get_hostfxr_path
        auto get_hostfxr_path_handle{ reinterpret_cast<decltype (&get_hostfxr_path)>(GetProcAddress(handleNethost, "get_hostfxr_path")) };
        if (get_hostfxr_path_handle == NULL)
            return DisplayError(L"Failed getting address of get_hostfxr_path");

        // Buffer for hostfxr.dll path
        char_t buffer[MAX_PATH];
        size_t bufferSize{ std::size(buffer) };

        // Locate hostfxr.dll
        int result{ get_hostfxr_path_handle(buffer, &bufferSize, nullptr) };
        if (result == HostApiBufferTooSmall)
            return DisplayError(L"get_hostfxr_path failed, provided buffer (" + std::to_wstring(bufferSize) + L") is too small");

        if (result != 0)
            return DisplayError(L"get_hostfxr_path failed");


        // Load hostfxr.dll
        HMODULE handleHostFxr{ LoadLibraryW(buffer) };
        if (handleHostFxr == NULL)
            return DisplayError(L"Failed loading module : " + std::wstring(buffer));

        // Locate 3 methods in the module
        auto initRuntime{ reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(GetProcAddress(handleHostFxr, "hostfxr_initialize_for_runtime_config")) };
        if (initRuntime == NULL)
            return DisplayError(L"Failed getting address of hostfxr_initialize_for_runtime_config");

        auto getRuntimeDelegate{ reinterpret_cast<hostfxr_get_runtime_delegate_fn>(GetProcAddress(handleHostFxr, "hostfxr_get_runtime_delegate")) };
        if (getRuntimeDelegate == NULL)
            return DisplayError(L"Failed getting address of hostfxr_get_runtime_delegate");

        auto closeHostFxr{ reinterpret_cast<hostfxr_close_fn>(GetProcAddress(handleHostFxr, "hostfxr_close")) };
        if (closeHostFxr == NULL)
            return DisplayError(L"Failed getting address of hostfxr_close");


        // Initialize the runtime
        hostfxr_handle runtimeContext{ nullptr };
        result = initRuntime(managed->runtimeConfigPath, nullptr, &runtimeContext);
        if (result != 0 || runtimeContext == nullptr)
        {
            closeHostFxr(runtimeContext);
            std::wstringstream ss; ss << L"Runtime initialization failed " << std::hex << std::showbase << result;
            return DisplayError(std::wstring(ss.str()));
        }

        // Get the load assembly function pointer
        load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer{ nullptr };
        result = getRuntimeDelegate(runtimeContext, hdt_load_assembly_and_get_function_pointer, (void**)&loadAssemblyAndGetFunctionPointer);
        if (result != 0 || runtimeContext == nullptr || loadAssemblyAndGetFunctionPointer == nullptr)
        {
            closeHostFxr(runtimeContext);
            std::wstringstream ss; ss << L"Failed loading the runtime " << std::hex << std::showbase << result;
            return DisplayError(std::wstring(ss.str()));
        }

        closeHostFxr(runtimeContext);

        // Call managed method
        component_entry_point_fn ManagedEntryPoint{ nullptr };
        result = loadAssemblyAndGetFunctionPointer(
            managed->assemblyPath,
            managed->typeName,
            managed->methodName,
            nullptr,
            nullptr,
            (void**)&ManagedEntryPoint);

        if (result != 0 || ManagedEntryPoint == nullptr) {
            return DisplayError(L"Failed loading managed application");
        }

        // We don't pass any arguments
        ManagedEntryPoint(nullptr, 0);

        return EXIT_SUCCESS;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    //Do not load the runtime in DllMain, this will cause a deadlock
    //see https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


int DisplayError(std::wstring message) {
    DWORD errorCode{ GetLastError() }; // Error codes list : https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes


    LPTSTR lpMsgBuf;
    FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);

    std::wstring formatedLastError{ lpMsgBuf };

    std::wstring result{ L"\n[!] " + message + L". Error code " + std::to_wstring(errorCode) + L" : " + formatedLastError };

    OutputDebugStringW(result.c_str()); // DebugView https://docs.microsoft.com/en-us/sysinternals/downloads/debugview

    return EXIT_FAILURE;
}

