## exportdump

Quick CLI reverse engineering tool I wrote to dump all entries in a PE files' Export Address Table (EAT).\
Currently (and probably forever) only runs on Windows x64.\
Born out of pure hatred for Ghidra's python scripting.\
You can read more about the PE file format [here](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).

## Features:
- Supports both x32 and x64 PE files (although the application itself is x64 only)
- When parsing files it does not actually load them into the program (no calls to [LoadLibrary](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya))
- C++ name demangling (uses Dbghelp's [UnDecorateSymbolName](https://learn.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-undecoratesymbolname))
- Supports multiple dump formats:
	+ Raw text, each line represanting an EAT entry, formatted as [ORDINAL] [ADDRESS] [NAME] [DEMANGLED-NAME]
	+ C++ header containing a `constexpr` lookup table, found in `namespace exports::<input filename (no extensions/directory)>`
	+ JSON file, containing the `exports` array of objects with `ordinal,address,name,demangledName` fields.

## Usage
`exportdump.exe <input> <output> [options]`
- `<input file>`: path to the PE file, can be `"` delimited
- `<output file>`: path to dump file, can be `"` delimited
- `[options]`: optional flags. Can be:
	+ `--text`: Dump as raw text
	+ `--cpp`: Dump as C++ header. This is the default dump format
	+ `--json`: Dump as JSON file

\
The C++ generated header contains `namespace export::<filaname>`, that in turn contains:
+ Definition of the `ExportData` structure:
```cpp
struct ExportData
{
	const std::string_view name;
	const std::string_view demangledName;
	const uintptr_t address;
	const uint16_t ordinal;
};
```
+ A compile time array with all the exports:
```cpp
constexpr const size_t g_count;
constexpr const ExportData g_exports[g_count];
```
+ Compile time lookup functions, allowing to get the whole EAT entry info from just one data point: 
```cpp
constexpr const ExportData* findName(std::string_view name);
constexpr const ExportData* findDemangledName(std::string_view demangled);
constexpr const ExportData* findAddress(uintptr_t address);
constexpr const ExportData* findName(uint16_t ordinal);
```

## Example

`exportdump.exe "C:\Windows\System32\kernel32.dll" "kernel32.exports.hpp" --cpp`
```cpp
#include "kernel32.exports.hpp"

int main(int argc, const char** argv)
{
	static_assert(exports::kernel32::findOrdinal(1006)->name == "LoadLibraryA", "Something is very wrong!");
	return 0;
}
```

## Building

The project uses CMake as its build system. While the tool only supports Windows, if I have to manually edit another .vcxproj file
because Visual Studio decided to be special, I'm going to have a conniption.\
Additionally, the latests versions of VS 2022 support CMake projects natively.\
When invoking `cmake` in the terminal, make sure you pass the `--presets` option, as this project heavily depends on CMakePresets.json.