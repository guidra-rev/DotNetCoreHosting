#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <iostream>
#include "../RuntimeHost/runtimehost.h"
#include <string>
#include <filesystem>
#include <psapi.h>
#include <shellapi.h>

std::wstring ToAbsolutePath(std::wstring relativePath);
int DisplayError(std::wstring errorMsg);
int LoadAndStart(int argc, char* argv[]);

std::uintptr_t ComputeEntryPointRVA(std::filesystem::path dllAbsolutePath) {
	
	HMODULE moduleHandle = LoadLibraryExW(dllAbsolutePath.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (moduleHandle == NULL) {
		DisplayError(L"LoadLibraryW failed");
		return 0;
	}

	FARPROC EntryPoint = GetProcAddress(moduleHandle, MAKEINTRESOURCEA(1)); // HACK : the entry point should always be the first exported method
	//FARPROC EntryPoint = GetProcAddress(moduleHandle, "EntryPoint");		// FIXME : returns NULL with x86 (works with x64)
	if (EntryPoint == NULL) {
		return DisplayError(L"GetProcAddress failed");
		return 0;
	}

	std::uintptr_t entryPointRVA{ (std::uintptr_t)EntryPoint - (std::uintptr_t)moduleHandle };

	if (moduleHandle != NULL)
		FreeLibrary(moduleHandle);

	return entryPointRVA;
}

std::uintptr_t Inject(HANDLE hProcess, std::filesystem::path dllAbsolutePath) {
	// allocate space for dll path
	LPVOID dllPathAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT, PAGE_READWRITE);
	if (dllPathAddress == NULL)
		return DisplayError(L"VirtualAllocEx failed");

	// write dll path in allocated space
	BOOL result = WriteProcessMemory(hProcess, dllPathAddress, dllAbsolutePath.c_str(), MAX_PATH, NULL);
	if (result == 0)
		return DisplayError(L"WriteProcessMemory failed");

	// get handle to Kernel32
	HMODULE hKernel32 = GetModuleHandleW(L"Kernel32");
	if (hKernel32 == NULL)
		return DisplayError(L"GetModuleHandleW failed");

	// get LoadLibraryW address
	FARPROC LoadLibraryWAddress = GetProcAddress(hKernel32, "LoadLibraryW");
	if (LoadLibraryWAddress == NULL)
		return DisplayError(L"GetProcAddress failed");

	// load dll in target process
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryWAddress, dllPathAddress, 0, NULL);
	if (hThread == NULL)
		return DisplayError(L"CreateRemoteThread failed");

	// wait for the thread to finish
	DWORD waitResult = WaitForSingleObject(hThread, INFINITE);
	if (waitResult == WAIT_FAILED)
		return DisplayError(L"WaitForSingleObject failed");

	// get LoadLibraryW returned value (module base)
	DWORD exitCode;
	result = GetExitCodeThread(hThread, &exitCode);
	if (result == 0)
		return DisplayError(L"GetExitCodeThread failed");

	// check if LoadLibraryW succeeded
	if (exitCode == NULL)
		return DisplayError(L"LoadLibraryW failed"); // here GetLastError() will not work, since it's store in Thread-local storage

	std::uintptr_t moduleBaseAddress = (std::uintptr_t) exitCode;

	// dispose
	VirtualFreeEx(hProcess, dllPathAddress, 0, MEM_RELEASE);

	if (hThread != NULL)
		CloseHandle(hThread);

	return moduleBaseAddress;
}


std::uintptr_t GetInjectedModuleBase(HANDLE hProcess, std::wstring dllName) {
	const int MAX_MODULES = 1024;
	HMODULE hMods[MAX_MODULES];
	DWORD cbNeeded;

	// enumerate modules
	if (!EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
		DisplayError(L"EnumProcessModules failed");
		return 0;
	}

	int moduleCount = cbNeeded / sizeof(HMODULE);

	for (int i = 0; i < moduleCount; i++)
	{
		TCHAR moduleFullPathBuffer[MAX_PATH];
		if (!GetModuleFileNameEx(hProcess, hMods[i], moduleFullPathBuffer, sizeof(moduleFullPathBuffer) / sizeof(TCHAR))) {
			DisplayError(L"GetModuleFileNameEx failed");
			return 0;
		}

		std::filesystem::path moduleFullPath{ moduleFullPathBuffer };
		std::filesystem::path moduleName = moduleFullPath.filename();
		
		if (wcscmp(moduleName.c_str(), dllName.c_str()) == 0) {
			return (std::uintptr_t) hMods[i];
		}
	}

	//not found
	return 0;
}

int main(int argc, char* argv[])
{

	//debugging
	//return LoadAndStart(argc, argv);

	// x64 or x86
#ifdef _WIN64
	const std::wstring projectPlatform{ LR"(x64\)" };
	const std::wstring notepadPath{ LR"(C:\Windows\notepad.exe)" };
	const std::wstring nethostPath{ NetHostPathx64 };
#else
	const std::wstring projectPlatform{ LR"(x86\)" };
	const std::wstring notepadPath{ LR"(C:\Windows\SysWOW64\notepad.exe)" };
	const std::wstring nethostPath{ NetHostPathx86 };
#endif // _WIN64

	// Debug or Release
#ifdef _DEBUG
	const std::wstring projectConfiguration{ LR"(Debug\)" };
#else
	const std::wstring projectConfiguration{ LR"(Release\)" };
#endif // _DEBUG


	// start notepad.exe
	/*STARTUPINFOW si{};
	PROCESS_INFORMATION pi{};
	if (!CreateProcessW(notepadPath.c_str(), NULL, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
		return DisplayError(L"CreateProcessW failed");*/

	// start notepad.exe elevated
	SHELLEXECUTEINFOW shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo);
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = L"runas";               
	shExInfo.lpFile = notepadPath.c_str();   
	shExInfo.lpParameters = L"";                
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_SHOW;
	shExInfo.hInstApp = 0;

	if (!ShellExecuteExW(&shExInfo) || shExInfo.hProcess == 0)
		return DisplayError(L"ShellExecuteExW failed");

	DWORD pid = GetProcessId(shExInfo.hProcess);
	if(pid == 0)
		return DisplayError(L"GetProcessId failed");

	// get process handle passing in the process ID
	HANDLE hProcess = OpenProcess(
		PROCESS_QUERY_INFORMATION |
		PROCESS_CREATE_THREAD |
		PROCESS_VM_OPERATION |
		PROCESS_VM_READ |
		PROCESS_VM_WRITE,
		FALSE, pid);
	if (hProcess == NULL)
		return DisplayError(L"OpenProcess failed");

	// search RuntimeHost.dll in the same directory
	std::wstring dllName{ L"RuntimeHost.dll" };
	std::filesystem::path dllAbsolutePath{ argv[0] };
	dllAbsolutePath = dllAbsolutePath.remove_filename();
	dllAbsolutePath.concat(dllName);

	// load dll in current process to retrieve EntryPoint address
	std::uintptr_t entryPointRVA  = ComputeEntryPointRVA(dllAbsolutePath);
	if (entryPointRVA == 0)
		return EXIT_FAILURE;

	// inject dll
	Inject(hProcess, dllAbsolutePath);

	std::uintptr_t moduleBaseAddress = GetInjectedModuleBase(hProcess, dllName);
	if (moduleBaseAddress == 0)
		return EXIT_FAILURE;

	// calculate entry point address
	std::uintptr_t entryPointAddress = moduleBaseAddress + entryPointRVA;

	//entry point parameters
	std::wstring assemblyPath		{ ToAbsolutePath(LR"(..\WinFormsSample\bin\)" + projectPlatform + projectConfiguration + LR"(netcoreapp3.1\WinFormsSample.dll)") };
	std::wstring typeName			{ L"WinFormsSample.Program, WinFormsSample" };
	std::wstring methodName			{ L"HostEntryPoint" };
	std::wstring runtimeConfigPath	{ ToAbsolutePath(LR"(..\WinFormsSample\bin\)" + projectPlatform + projectConfiguration + LR"(netcoreapp3.1\WinFormsSample.runtimeconfig.json)") };

	EntryPointParams params{};
	wcscpy_s(params.assemblyPath,		MAX_PATH, assemblyPath.c_str());
	wcscpy_s(params.typeName,			MAX_PATH, typeName.c_str());
	wcscpy_s(params.methodName,			MAX_PATH, methodName.c_str());
	wcscpy_s(params.runtimeConfigPath,	MAX_PATH, runtimeConfigPath.c_str());
	wcscpy_s(params.nethostPath,		MAX_PATH, nethostPath.c_str());

	const int STRING_PARAMS_COUNT = 5; // ugly
	SIZE_T paramsBufferSize = STRING_PARAMS_COUNT * MAX_PATH * sizeof(wchar_t);

	// allocate space for params
	LPVOID paramsAddress = VirtualAllocEx(hProcess, NULL, paramsBufferSize, MEM_COMMIT, PAGE_READWRITE);
	if (paramsAddress == NULL)
		return DisplayError(L"VirtualAllocEx failed");

	// write params in allocated space
	BOOL result = WriteProcessMemory(hProcess, paramsAddress, &params, paramsBufferSize, NULL);
	if (result == 0)
		return DisplayError(L"WriteProcessMemory failed");

	// execute entry point
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE) entryPointAddress, paramsAddress, NULL, NULL);
	if (hThread == NULL)
		return DisplayError(L"CreateRemoteThread failed");

	// join thread
	DWORD waitResultEntry = WaitForSingleObject(hThread, INFINITE);
	if (waitResultEntry == WAIT_FAILED)
		return DisplayError(L"WaitForSingleObject failed");

	// get EntryPoint returned value
	DWORD exitCodeEntry;
	result = GetExitCodeThread(hThread, &exitCodeEntry);
	if (result == 0)
		return DisplayError(L"GetExitCodeThread failed");


	//
	// Dispose
	//
	VirtualFreeEx(hProcess, paramsAddress, 0, MEM_RELEASE);

	if (hThread != NULL)	CloseHandle(hThread);
	//if (pi.hProcess != NULL)	CloseHandle(pi.hProcess);
	//if (pi.hThread != NULL)		CloseHandle(pi.hThread);
	if (hProcess != NULL)		CloseHandle(hProcess);

	return EXIT_SUCCESS;
}

std::wstring ToAbsolutePath(std::wstring relativePath) {
	wchar_t buffer[MAX_PATH];

	if (!GetFullPathNameW(relativePath.c_str(), MAX_PATH, buffer, NULL)) {
		DisplayError(L"GetFullPathNameW failed");
		return nullptr;
	}

	return std::wstring { buffer };
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


//for Debugging
int LoadAndStart(int argc, char* argv[]) {
	// x64 or x86
#ifdef _WIN64
	const std::wstring projectPlatform{ LR"(x64\)" };
	const std::wstring nethostPath{ NetHostPathx64 };
#else
	const std::wstring projectPlatform{ LR"(x86\)" };
	const std::wstring nethostPath{ NetHostPathx86 };
#endif // _WIN64

	// Debug or Release
#ifdef _DEBUG
	const std::wstring projectConfiguration{ LR"(Debug\)" };
#else
	const std::wstring projectConfiguration{ LR"(Release\)" };
#endif // _DEBUG

	// search RuntimeHost.dll from the same directory
	std::filesystem::path dllAbsolutePath{ argv[0] };
	dllAbsolutePath = dllAbsolutePath.remove_filename();
	dllAbsolutePath.concat(L"RuntimeHost.dll");

	HMODULE moduleHandle{ LoadLibraryW(dllAbsolutePath.c_str()) };
	if (moduleHandle == NULL) return DisplayError(L"LoadLibraryW failed");

	
	//auto entryPoint{ reinterpret_cast<decltype (&EntryPoint)>(GetProcAddress(moduleHandle, "EntryPoint")) };
	auto entryPoint{ reinterpret_cast<decltype (&EntryPoint)>(GetProcAddress(moduleHandle, MAKEINTRESOURCEA(1))) };
	if (entryPoint == NULL) return DisplayError(L"GetProcAddress failed");

	//entry point parameters
	std::wstring assemblyPath{ ToAbsolutePath(LR"(..\WinFormsSample\bin\)" + projectPlatform + projectConfiguration + LR"(netcoreapp3.1\WinFormsSample.dll)") };
	std::wstring typeName{ L"WinFormsSample.Program, WinFormsSample" };
	std::wstring methodName{ L"HostEntryPoint" };
	std::wstring runtimeConfigPath{ ToAbsolutePath(LR"(..\WinFormsSample\bin\)" + projectPlatform + projectConfiguration + LR"(netcoreapp3.1\WinFormsSample.runtimeconfig.json)") };

	EntryPointParams params{};
	wcscpy_s(params.assemblyPath, MAX_PATH, assemblyPath.c_str());
	wcscpy_s(params.typeName, MAX_PATH, typeName.c_str());
	wcscpy_s(params.methodName, MAX_PATH, methodName.c_str());
	wcscpy_s(params.runtimeConfigPath, MAX_PATH, runtimeConfigPath.c_str());
	wcscpy_s(params.nethostPath, MAX_PATH, nethostPath.c_str());

	entryPoint(&params);

	if (!FreeLibrary(moduleHandle)) return DisplayError(L"FreeLibrary failed");


	return EXIT_SUCCESS;
}