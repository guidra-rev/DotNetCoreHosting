The goal of this project is to start a .NET Core 3.1 WinForms application, from a native (ie unmanaged) process.
It can be easily modified to start any other type of .NET Core project.

There are [three methods](https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting) that can be used to host .NET Core runtime :
* The "Mscoree" method : the oldest one, also used for the older .NET Framework, [this project](https://www.codeproject.com/Articles/607352/Injecting-NET-Assemblies-Into-Unmanaged-Processes) is a good example.
* The "CoreClrHost" method : introduced in .NET Core 2.1
* The "HostFxr" method : introduced with .Net Core 3.1, it is now the recommended API.

This project uses the "HostFxr" method, and is based on the [official sample](https://github.com/dotnet/samples/tree/master/core/hosting/HostWithHostFxr).

# Code overview

1. The get_hostfxr_path method retrieves the path of `hostfxr.dll`

```C++
char_t buffer[MAX_PATH];
size_t bufferSize = std::size(buffer);
int result = get_hostfxr_path(buffer, &bufferSize, nullptr);
```

2. `hostfxr.dll` exposes methods to load and initialize the runtime

```C++
HMODULE handleHostFxr = LoadLibraryW(buffer);
auto initRuntime = reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>( GetProcAddress(handleHostFxr, "hostfxr_initialize_for_runtime_config") );
auto getRuntimeDelegate = reinterpret_cast<hostfxr_get_runtime_delegate_fn>( GetProcAddress(handleHostFxr, "hostfxr_get_runtime_delegate") );
```

3. The runtime is initilized with a `runtime.json` configuration file. It's automatically generated for every .NET Core project after compilation.

```C++
hostfxr_handle runtimeContext{ nullptr };
result = initRuntime(managed->runtimeConfigPath, nullptr, &runtimeContext); //runtimeConfigPath points to WinFormsSample.runtimeconfig.json
```

4. Load the runtime

```C++
load_assembly_and_get_function_pointer_fn loadAssemblyAndGetFunctionPointer = nullptr;
result = getRuntimeDelegate( runtimeContext, hdt_load_assembly_and_get_function_pointer, (void**)&loadAssemblyAndGetFunctionPointer);
```

5. Execute a specified method from the managed application.
It this sample we start the `HostEntryPoint` method, in the `Program` class, in the `WinFormsApp` namespace, in the `WinFormsApp` assembly.
We do not pass any arguments, but it can be done, as shown in the [official sample](https://github.com/dotnet/samples/tree/master/core/hosting/HostWithHostFxr)

```C++

component_entry_point_fn ManagedEntryPoint = nullptr;
result = loadAssemblyAndGetFunctionPointer(
    assemblyPath.c_str(),   // path to WinFormsSample.dll
    typeName.c_str(),       // "WinFormsSample.Program, WinFormsSample"
    methodName.c_str(),     // "HostEntryPoint"
    nullptr,
    nullptr,
    (void**)&ManagedEntryPoint);

ManagedEntryPoint(nullptr, 0);
```

6. The entry point needs to match this signature

```C#
namespace WinFormsApp
{
    public static class Program
    {
        [STAThread]
        public static int HostEntryPoint(IntPtr arg, int argLength)
        {
            ...
        }
    }
}
```

# Remarks

The following files were copied from the [dot net runtime repo](https://github.com/dotnet/runtime) :
- [`coreclr_delegates.h`](https://github.com/dotnet/runtime/blob/4f9ae42d861fcb4be2fcd5d3d55d5f227d30e723/src/installer/corehost/cli/coreclr_delegates.h)
- [`hostfxr.h`](https://github.com/dotnet/runtime/blob/4f9ae42d861fcb4be2fcd5d3d55d5f227d30e723/src/installer/corehost/cli/hostfxr.h)
- [`error_codes.h`](https://github.com/dotnet/runtime/blob/4f9ae42d861fcb4be2fcd5d3d55d5f227d30e723/src/installer/corehost/error_codes.h) 


## nethost.dll

In order to locate `hostfxr.dll`, you will need a method from `nethost.dll`. `nethost.dll` is shipped with the SDK, after installation it can be found in these directories :

- 32bit `C:\Program Files (x86)\dotnet\packs\Microsoft.NETCore.App.Host.win-x86\3.1.0\runtimes\win-x86\native`
- 64bit `C:\Program Files\dotnet\packs\Microsoft.NETCore.App.Host.win-x64\3.1.0\runtimes\win-x64\native`

I defined a macro called $(NetHostPath) to hold the folder path, each configuration has it's own value. [See here](https://docs.microsoft.com/en-us/cpp/build/working-with-project-properties?view=vs-2019#user-defined-macros) on how to define user macros.

## Debugging

If you want to debug the Winforms application, you can make a call to `Debugger.Launch()`, like I did on the "Debug" button, or attach to a running processs.

You cannot debug both the code loading the runtime (unmanaged) and the managed application at the same time. Even if you select both the native and CoreClr debuggers. This is a [documented](https://github.com/dotnet/samples/tree/master/core/hosting/HostWithHostFxr) limitation.