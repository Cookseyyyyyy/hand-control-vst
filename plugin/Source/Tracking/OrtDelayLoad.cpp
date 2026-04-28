#ifdef _WIN32

/**
 * OrtDelayLoad.cpp
 *
 * When onnxruntime.dll is delay-loaded, Windows resolves it on first use.
 * Ableton (and other DAW scanners) load our VST3 into a process whose working
 * directory is not the plugin bundle, so the default delay-load search would
 * fail to find the DLL in Contents/x86_64-win/.
 *
 * We register a custom delay-load failure hook that locates the DLL relative
 * to the loaded module's own path (using GetModuleFileName on a symbol in
 * this translation unit), and loads it with LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
 * so Windows looks in the same directory as the caller.
 *
 * This approach:
 *   - Requires zero cooperation from the host process.
 *   - Does not pollute the system PATH.
 *   - Is safe to call from DllMain-adjacent contexts (no allocation,
 *     just a kernel call).
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <delayimp.h>
#include <string>

namespace
{
    static HMODULE g_ort_module = nullptr;

    static HMODULE loadOrtFromBundleDir()
    {
        if (g_ort_module)
            return g_ort_module;

        // Get the path of our own DLL by using a local symbol.
        wchar_t path[MAX_PATH] = {};
        HMODULE self = nullptr;
        ::GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&loadOrtFromBundleDir),
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
        HMODULE h = ::LoadLibraryExW(dllPath.c_str(), nullptr, 0x100);
        if (h)
            g_ort_module = h;

        return h;
    }

    // Delay-load notification hook. Intercepted when the loader is about to
    // resolve onnxruntime.dll for the first time.
    static FARPROC WINAPI delayLoadHook(unsigned dliNotify,
                                        DelayLoadInfo* pdli)
    {
        if (dliNotify == dliNotePreLoadLibrary)
        {
            if (pdli && pdli->szDll &&
                ::_strnicmp(pdli->szDll, "onnxruntime", 11) == 0)
            {
                HMODULE h = loadOrtFromBundleDir();
                if (h)
                    return reinterpret_cast<FARPROC>(h);
            }
        }
        return nullptr;
    }
}

// Register the hook by writing to the linker-provided pointer.
ExternC const PfnDliHook __pfnDliNotifyHook2 = delayLoadHook;

#endif
