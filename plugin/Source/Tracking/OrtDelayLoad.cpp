 #ifdef _WIN32

/**
 * OrtDelayLoad.cpp
 *
 * Despite the filename, we no longer use MSVC delay-load for onnxruntime.dll.
 * Some DAW scanners still reject a VST3 with *any* ONNX Runtime import entry,
 * even a delay import. Instead we keep ONNX Runtime entirely out of the import
 * table and load it manually from the plugin binary's own directory.
 *
 * `handcontrolLoadOrtApiBaseFromPluginDir()` is called from
 * `OnnxHandTracker::initialise()` before any ONNX Runtime C++ wrapper is used.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <onnxruntime_c_api.h>
#include <string>

extern "C" const OrtApiBase* handcontrolLoadOrtApiBaseFromPluginDir();

using OrtGetApiBaseFn = const OrtApiBase* (*)();

namespace
{
    static HMODULE g_ort_module = nullptr;
    static const OrtApiBase* g_api_base = nullptr;

    static HMODULE loadOrtFromPluginDir()
    {
        if (g_ort_module)
            return g_ort_module;

        // Get the path of our own DLL by using a local symbol.
        wchar_t path[MAX_PATH] = {};
        HMODULE self = nullptr;
        ::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&loadOrtFromPluginDir),
            &self);

        if (self == nullptr)
            return nullptr;

        ::GetModuleFileNameW(self, path, MAX_PATH);

        // Strip the filename to get the containing directory.
        wchar_t* last = wcsrchr(path, L'\\');
        if (last) *(last + 1) = L'\0';

        std::wstring dllPath = std::wstring(path) + L"onnxruntime.dll";

        // LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR (0x100) makes Windows search the
        // directory containing the DLL being loaded for its own dependencies.
        HMODULE h = ::LoadLibraryExW(dllPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
        if (h)
            g_ort_module = h;

        return h;
    }
}

extern "C" const OrtApiBase* handcontrolLoadOrtApiBaseFromPluginDir()
{
    if (g_api_base)
        return g_api_base;

    HMODULE h = loadOrtFromPluginDir();
    if (! h)
        return nullptr;

    auto* getApiBase = reinterpret_cast<OrtGetApiBaseFn>(
        ::GetProcAddress(h, "OrtGetApiBase"));
    if (! getApiBase)
        return nullptr;

    g_api_base = getApiBase();
    return g_api_base;
}

#endif
