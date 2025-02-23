#include "demangler.hpp"
#include "windows_exception.hpp"
#include "console.hpp"
#include <DbgHelp.h>

Demangler::Demangler() :
	m_handle(nullptr)
{
	HANDLE currentProcess = GetCurrentProcess();

	if (!DuplicateHandle(currentProcess, currentProcess, currentProcess, &m_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
	{
		throw WindowsException("Failed to initialize sysmbol demangler");
	}

	if (!SymInitialize(m_handle, nullptr, TRUE))
	{
		throw WindowsException("Failed to initialize sysmbol demangler");
	}
}

Demangler::~Demangler()
{
	SymCleanup(m_handle);
	CloseHandle(m_handle);
}

std::string_view Demangler::demangle(std::string_view name) const
{
	size_t writtenChars = UnDecorateSymbolName(
		name.data(), 
		s_buffer.get(),
		BUFFER_SIZE,
		UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_ALLOCATION_MODEL | UNDNAME_NO_ACCESS_SPECIFIERS | UNDNAME_NO_RETURN_UDT_MODEL | UNDNAME_NO_MS_KEYWORDS
	);

	return std::string_view(s_buffer.get(), writtenChars);
}
