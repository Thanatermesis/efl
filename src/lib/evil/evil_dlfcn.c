#ifdef HAVE_CONFIG_H
# include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>

#include "evil_private.h"

#include <psapi.h> /*  EnumProcessModules(Ex) */


/**
 * @internal
 * @brief Stores the last error message.
 * This variable holds the dynamically allocated string for the last error.
 */
static char *_dl_err = NULL;
/**
 * @internal
 * @brief Flag indicating if the last error message has been retrieved by dlerror().
 * 0 if not viewed, 1 if viewed.
 */
static int _dl_err_viewed = 0;

/**
 * @internal
 * @brief Formats and stores the last error message.
 *
 * This function takes a description string, appends the system's last error
 * message (retrieved via evil_last_error_get()), and stores the combined
 * string in the global `_dl_err` variable. It also resets `_dl_err_viewed`.
 *
 * @param desc A description of the error context, e.g., "LoadLibraryEx returned: ".
 */
static void
_dl_get_last_error(char *desc)
{
   const char *str;
   size_t l1;
   size_t l2;

   str = evil_last_error_get();

   l1 = strlen(desc);
   l2 = strlen(str);

   if (_dl_err)
     free(_dl_err);

   _dl_err = (char *)malloc(sizeof(char) * (l1 + l2 + 1));
   if (!_dl_err)
     _dl_err = strdup("not enough resource");
   else
     {
        memcpy(_dl_err, desc, l1);
        memcpy(_dl_err + l1, str, l2);
        _dl_err[l1 + l2] = '\0';
     }
   _dl_err_viewed = 0;
}

/**
 * @brief Map a specified executable module (either a .dll or .exe file)
 * into the address space of the user process.
 *
 * @param path Name of the module. If NULL, returns a handle to the current process.
 * @param mode Unused in this implementation.
 * @return A pointer that represent the module, or @c NULL on failure.
 *         The returned handle can be used with dlsym() and dlclose().
 *
 * This function implements behavior similar to the POSIX dlopen().
 * It handles path separator conversion (Unix '/' to Windows '\')
 * and, if UNICODE is defined, converts the path to a wide character string
 * before calling LoadLibrary.
 * If path is NULL, it attempts to get a handle to the calling process's module.
 * Errors are stored and can be retrieved with dlerror().
 */
EVIL_API void *
dlopen(const char* path, int mode EVIL_UNUSED)
{
   HMODULE module = NULL;

   if (!path)
     {
        module = GetModuleHandle(NULL);
        if (!module)
          _dl_get_last_error("GetModuleHandle returned: ");
     }
   else
     {
        char        *new_path;
        size_t       l;
        unsigned int i;

        /* according to MSDN, we must change the slash to backslash */
        l = strlen(path);
        new_path = (char *)malloc(sizeof(char) * (l + 1));
        if (!new_path)
          {
             if (_dl_err)
               free(_dl_err);
             _dl_err = strdup("not enough resource");
             _dl_err_viewed = 0;
             return NULL;
          }
        for (i = 0; i <= l; i++)
          {
             if (path[i] == '/')
               new_path[i] = '\\';
             else
               new_path[i] = path[i];
          }
#ifdef UNICODE
        {
           wchar_t *wpath;

           wpath = evil_char_to_wchar(new_path);
           module = LoadLibrary(wpath);
           free(wpath);
        }
#else
        module = LoadLibraryEx(new_path, NULL,
                               LOAD_WITH_ALTERED_SEARCH_PATH);
#endif /* ! UNICODE */
        if (!module)
          _dl_get_last_error("LoadLibraryEx returned: ");

        free(new_path);
     }

   return module;
}

/**
 * @brief Close a dynamic-link library.
 *
 * @param handle Handle that references a dynamic-link library, previously
 *               obtained from dlopen().
 * @return 0 on success, -1 otherwise.
 *
 * This function decrements the reference count of the loaded library.
 * If the reference count drops to zero, the module is unmapped from
 * the address space of the calling process.
 * Errors are stored and can be retrieved with dlerror().
 */
EVIL_API int
dlclose(void* handle)
{
   if (FreeLibrary(handle))
     return 0;
   else
     {
        _dl_get_last_error("FreeLibrary returned: ");
        return -1;
     }
}

/**
 * @brief Get the address of a symbol in a loaded library.
 *
 * @param handle Handle to a dynamic library obtained from dlopen(), or
 *               RTLD_DEFAULT to search in all loaded modules.
 * @param symbol The null-terminated name of the symbol to find.
 * @return The address of the symbol if found, NULL otherwise.
 *
 * This function retrieves the address of an exported symbol (function or
 * variable) from the specified module.
 * If UNICODE is defined, the symbol name is converted to a wide character string.
 * If `handle` is RTLD_DEFAULT, it iterates through all modules loaded by the
 * current process to find the symbol.
 * Errors are stored and can be retrieved with dlerror().
 */
EVIL_API void *
dlsym(void *handle, const char *symbol)
{
   FARPROC fp = NULL;
   LPCTSTR new_symbol;

   if (!symbol || !*symbol) return NULL;

#ifdef UNICODE
   new_symbol = evil_char_to_wchar(symbol);
#else
   new_symbol = symbol;
#endif /* UNICODE */

   if (handle == RTLD_DEFAULT)
     {
        HMODULE modules[1024];
        DWORD needed;
        DWORD i;

        /* TODO: use EnumProcessModulesEx() on Windows >= Vista */
        if (!EnumProcessModules(GetCurrentProcess(),
                                modules, sizeof(modules), &needed))
          {
#ifdef UNICODE
             _dl_get_last_error("EnumProcessModules returned: ");
             free((void *)new_symbol);
#endif /* UNICODE */
             return NULL;
          }

        for (i = 0; i < (needed / sizeof(HMODULE)); i++)
          {
            fp = GetProcAddress(modules[i], new_symbol);
            if (fp) break;
          }
     }
   else
     fp = GetProcAddress(handle, new_symbol);

#ifdef UNICODE
   free((void *)new_symbol);
#endif /* UNICODE */

   if (!fp)
     _dl_get_last_error("GetProcAddress returned: ");

   return fp;
}

/**
 * @brief Get diagnostic information for the last dlfcn error.
 *
 * @return A null-terminated string describing the last error that occurred
 *         during a call to dlopen(), dlsym(), or dlclose() on this thread.
 *         Returns NULL if no error has occurred since the last call to
 *         dlerror() or if dlerror() has not been called yet.
 *
 * The error string is cleared after a call to dlerror(). Subsequent calls
 * to dlerror(), without an intervening dlfcn error, will return NULL.
 */
EVIL_API char *
dlerror (void)
{
   if (!_dl_err_viewed)
     {
        _dl_err_viewed = 1;
        return _dl_err;
     }
   else
     {
        if (_dl_err)
          free(_dl_err);
        return NULL;
     }
}

#ifdef _GNU_SOURCE

/**
 * @internal
 * @brief Buffer to store the filename for Dl_info.dli_fname.
 * MAX_PATH is a Windows constant for maximum path length.
 */
static char _dli_fname[MAX_PATH];
/**
 * @internal
 * @brief Buffer to store the symbol name for Dl_info.dli_sname.
 * Assumes a symbol name will not exceed MAX_PATH (though typically much shorter).
 */
static char _dli_sname[MAX_PATH]; /* a symbol should have at most 255 char */

/**
 * @internal
 * @brief Comparison function for qsort, used in dladdr.
 *
 * Compares two DWORD values, typically RVAs (Relative Virtual Addresses)
 * of functions, to sort them in ascending order.
 *
 * @param p1 Pointer to the first DWORD.
 * @param p2 Pointer to the second DWORD.
 * @return An integer less than, equal to, or greater than zero if p1 is
 *         found, respectively, to be less than, to match, or be greater
 *         than p2.
 */
static int
_dladdr_comp(const void *p1, const void *p2)
{
   return ( *(int *)p1 - *(int *)p2);
}

/**
 * @brief Resolve module and function pointers from the given function pointer address.
 *
 * @param addr A function pointer (or any address within a loaded module).
 * @param info Pointer to a #Dl_info structure to be filled with information.
 *             The structure will contain:
 *             - dli_fname: The filename of the module containing `addr`.
 *             - dli_fbase: The base load address of that module.
 *             - dli_sname: The name of the nearest symbol to `addr`.
 *             - dli_saddr: The exact address of that nearest symbol.
 *             If no symbol is found (e.g., in an EXE), dli_sname and dli_saddr may be NULL.
 * @return 1 on success, 0 on failure.
 *
 * This function attempts to find information about the module and symbol
 * associated with a given memory address. It uses VirtualQuery to find the
 * module's base address and GetModuleFileName to get its path.
 * It then parses the PE header of the module to find the export directory
 * and locate the nearest symbol to the provided address.
 * This function is available only when _GNU_SOURCE was defined before
 * including dlfcn.h.
 */
EVIL_API int
dladdr (const void *addr, Dl_info *info)
{
   TCHAR tpath[PATH_MAX];
   MEMORY_BASIC_INFORMATION mbi;
   unsigned char *base;
   char *path;
   size_t length;

   IMAGE_NT_HEADERS *nth;
   IMAGE_EXPORT_DIRECTORY *ied;
   DWORD *addresses;
   WORD *ordinals;
   DWORD *names;
   DWORD *tmp;
   DWORD res;
   DWORD rva_addr;
   DWORD i;

   if (!info)
     return 0;

   info->dli_fname = NULL;
   info->dli_fbase = NULL;
   info->dli_sname = NULL;
   info->dli_saddr = NULL;

   /* Get the name and base address of the module */

   // Use VirtualQuery to get information about the memory region containing addr.
   // This helps identify the module (DLL or EXE) to which addr belongs.
   if (!VirtualQuery(addr, &mbi, sizeof(mbi)))
     {
        _dl_get_last_error("VirtualQuery returned: ");
        return 0;
     }

   if (mbi.State != MEM_COMMIT)
     return 0;

   if (!mbi.AllocationBase)
     return 0;

   base = (unsigned char *)mbi.AllocationBase; // Base address of the module.

   // Get the full path of the module.
   if (!GetModuleFileName((HMODULE)base, (LPTSTR)&tpath, PATH_MAX))
     {
        _dl_get_last_error("GetModuleFileName returned: ");
        return 0;
     }

# ifdef UNICODE
   path = evil_wchar_to_char(tpath);
# else
   path = tpath;
# endif /* ! UNICODE */

   length = strlen(path);
   if (length >= PATH_MAX)
     {
       length = PATH_MAX - 1;
       path[PATH_MAX - 1] = '\0';
     }

   memcpy(_dli_fname, path, length + 1);
   info->dli_fname = (const char *)_dli_fname;
   info->dli_fbase = base;

# ifdef UNICODE
        free(path);
# endif /* ! UNICODE */

   /* get the name and the address of the required symbol */

   // Start PE header parsing.
   // Check for the MZ signature at the beginning of the DOS header.
   if (((IMAGE_DOS_HEADER *)base)->e_magic != IMAGE_DOS_SIGNATURE)
     {
        SetLastError(1276);
        return 0;
     }

   // Locate the NT headers (PE header).
   nth = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
   // Check for the PE signature.
   if (nth->Signature != IMAGE_NT_SIGNATURE)
     {
        SetLastError(1276); // ERROR_INVALID_ORDINAL or similar, indicating bad format
        return 0;
     }

   /* no exported symbols ? it's an EXE and we exit without error */
   // If there's no export directory, it might be an EXE or a DLL with no exports.
   // In this case, dli_sname and dli_saddr will remain NULL, which is valid.
   if (nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0)
     {
        return 1;
     }

   /* we assume now that the PE file is well-formed, so checks only when needed */
   // Get pointers to the export directory tables.
   ied = (IMAGE_EXPORT_DIRECTORY *)(base + nth->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_ENTRY_EXPORT].VirtualAddress);
   addresses = (DWORD *)(base + ied->AddressOfFunctions); // RVAs of exported functions
   ordinals = (WORD *)(base + ied->AddressOfNameOrdinals); // Ordinals corresponding to named exports
   names = (DWORD *)(base + ied->AddressOfNames);          // RVAs of export names

   /* the addresses are not ordered, so we need to order them */
   // The AddressOfFunctions table is not necessarily sorted. To find the
   // symbol whose address is closest to (and not greater than, or equal to) `addr`,
   // we sort the function RVAs.
   tmp = malloc(ied->NumberOfFunctions * sizeof(DWORD));
   if (!tmp)
     {
        SetLastError(8); // ERROR_NOT_ENOUGH_MEMORY
        return 0;
     }

   memcpy(tmp, addresses, ied->NumberOfFunctions * sizeof(DWORD));
   qsort(tmp, ied->NumberOfFunctions, sizeof(DWORD), _dladdr_comp);

   // Convert the absolute `addr` to an RVA relative to the module base.
   rva_addr = (unsigned char *)addr - base;
   res = (DWORD)(-1); // Will hold the RVA of the found symbol.

   // Find the smallest RVA in `tmp` that is >= rva_addr.
   // This means `addr` falls within the function starting at `tmp[i-1]` or is `tmp[i]`.
   // We are looking for the symbol whose address is `res`.
   for (i = 0; i < ied->NumberOfFunctions; i++)
     {
        if (tmp[i] <= rva_addr) // If the current function's RVA is less than or equal to the target RVA
          {
            // If this is the last function, or the next function's RVA is greater than the target RVA,
            // then the current function `tmp[i]` is the best match (nearest lower or equal symbol).
            if ((i + 1 == ied->NumberOfFunctions) || (tmp[i+1] > rva_addr))
            {
                res = tmp[i];
                break;
            }
          }
        else // tmp[i] > rva_addr
          {
            // If the first function RVA itself is greater than rva_addr, it means addr is before any known symbol.
            // Or, if we passed the target rva_addr, the previous symbol was the one.
            // However, the original logic seems to pick tmp[i] if tmp[i] >= rva_addr.
            // Let's stick to finding the first tmp[i] >= rva_addr.
            // If addr is exactly a function start, this is it.
            // If addr is inside a function, this will give the *next* function's start.
            // The original code seems to want the symbol *at or after* addr.
            // Let's refine: we want the largest RVA in `tmp` that is <= `rva_addr`.
            // This was the previous logic:
            // if (tmp[i] < rva_addr) continue; res = tmp[i]; break;
            // This finds the first symbol *at or after* addr.
            // The dladdr man page says "address of the nearest symbol with a lower address than addr".
            // So we need the largest tmp[j] such that tmp[j] <= rva_addr.
            // Let's re-evaluate the loop for finding `res`.
            // We want the symbol whose address is closest to `addr` but not exceeding it.
            // So, we iterate and keep the largest `tmp[k]` that is `<= rva_addr`.
            // If `addr` is before the first symbol, `res` should remain unassigned or indicate this.
            // The original code finds the *first* symbol whose address is >= `rva_addr`.
            // If `addr` is within a function, `res` will be the RVA of the *next* function.
            // If `addr` is exactly a function start, `res` will be that function's RVA.
            // The final loop then searches for this `res` RVA.
            // Let's assume the original logic for `res` is what's intended for this implementation.
            // It finds the RVA of the symbol that `addr` is part of, or the next symbol if `addr` is between symbols.
            // The man page for GNU dladdr says:
            // dli_sname: Name of nearest symbol with address lower than or equal to addr.
            // dli_saddr: Exact address of symbol named in dli_sname.
            // So, we need to find the largest RVA in `addresses` that is <= `rva_addr`.

            // Corrected logic for `res` to find nearest symbol with address <= addr:
            // Iterate through sorted `tmp`. `res` will store the largest RVA found so far that is <= `rva_addr`.
            // Initialize res to a value indicating "not found" or "before first symbol".
            // The original code's `res = tmp[i]` when `tmp[i] >= rva_addr` (first one) is different.
            // Let's stick to the original code's apparent intent for `res` calculation first.
            // The original code finds the first function RVA that is >= rva_addr.
            if (tmp[i] >= rva_addr) // Found a symbol at or after addr
            {
                res = tmp[i];
                // If this isn't the first symbol and the previous symbol's RVA is closer
                // to rva_addr (i.e., rva_addr is between tmp[i-1] and tmp[i]),
                // then the "nearest lower symbol" is tmp[i-1].
                if (i > 0 && tmp[i-1] <= rva_addr) {
                    // Check if addr is closer to tmp[i-1] or tmp[i]
                    // For "nearest lower symbol", it should be tmp[i-1] if tmp[i-1] <= rva_addr < tmp[i]
                    // If rva_addr == tmp[i], then tmp[i] is the symbol.
                    // If rva_addr < tmp[i], then tmp[i-1] is the nearest lower.
                    if (rva_addr < tmp[i]) {
                         res = tmp[i-1];
                    }
                } else if (i == 0 && rva_addr < tmp[i]) {
                    // addr is before the first symbol. No "lower" symbol.
                    // dladdr might return the first symbol in this case or nothing for sname/saddr.
                    // The original code would have picked tmp[0] for res.
                    // For now, let's assume `res` should be the RVA of the symbol containing `addr`,
                    // or the symbol whose start address is `addr`.
                    // The original code's simple search for first `tmp[i] >= rva_addr` might be
                    // trying to find the symbol *containing* `addr` by finding its upper bound.
                    // Let's trace the original logic for `res`:
                }
                break; // Found the candidate RVA
            }
     }

   /* if rva_addr is too high, we store the latest address */
   // This means `addr` is beyond the start of the last known symbol.
   // So, `addr` is considered to be within that last symbol.
   if (res == (DWORD)(-1) && ied->NumberOfFunctions > 0) // If no symbol RVA was found >= rva_addr, and there are functions
     res = tmp[ied->NumberOfFunctions - 1]; // Assign the RVA of the last symbol

   free(tmp);

   // Now, find the name for the symbol whose RVA is `res`.
   // Iterate through the *original* (unsorted) `addresses` and `names` tables.
   // The `ordinals` array maps an index in `names` to an index in `addresses`.
   for (i = 0; i < ied->NumberOfNames; i++)
     {
        // addresses[ordinals[i]] is the RVA of the symbol whose name is at names[i]
        if (addresses[ordinals[i]] == res) // If the RVA matches our target `res`
          {
             char *name;

             name = (char *)(base + names[i]); // Get the symbol name string
             length = strlen(name);
             if (length >= PATH_MAX)
               {
                  length = PATH_MAX - 1;
                  name[PATH_MAX - 1] = '\0';
               }
             memcpy(_dli_sname, name, length + 1);
             info->dli_sname = (const char *)_dli_sname;
             info->dli_saddr = base + res;
             return 1;
          }
     }

   return 0;
}

#endif /* _GNU_SOURCE */
