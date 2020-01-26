#pragma once

struct EntryPointParams {
	wchar_t nethostPath[MAX_PATH];		// nethost.dll full path
	wchar_t assemblyPath[MAX_PATH];		// Path of the dll
	wchar_t typeName[MAX_PATH];			// Format : {Namespace}.{Class}, {Assembly}
	wchar_t methodName[MAX_PATH];		// Static method
	wchar_t runtimeConfigPath[MAX_PATH];// Path of "runtimeconfig.json"
};


extern "C" __declspec(dllexport) int __stdcall EntryPoint(EntryPointParams* managed);


int DisplayError(std::wstring message);