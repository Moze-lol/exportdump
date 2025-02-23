#include "windows_exception.hpp"
#include <format>

WindowsError::WindowsError(DWORD errorCode) :
	code(errorCode),
	message()
{
	char* formattedMsg = nullptr;
	size_t messageSize = FormatMessageA(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
		nullptr,
		GetLastError(),
		LOCALE_SYSTEM_DEFAULT,
		reinterpret_cast<LPSTR>(&formattedMsg),
		0,
		nullptr);

	if (messageSize)
	{
#pragma warning(suppress: 6387) //FormatMessageA guarantess to allocate the message buffer if the return size is non 0
		message = std::string_view(formattedMsg, messageSize);
	}
}

WindowsError::~WindowsError()
{
	if (!message.empty())
	{
		void* mem = const_cast<char*>(message.data()); //const_cast is disgusting, but using LocalFree() forces it
		LocalFree(mem);
	}
}

WindowsException::WindowsException(std::string_view msg, DWORD code) :
	m_error(code),
	m_msg()
{
	if (m_error.message.empty())
	{
		m_msg = std::format("{} (error code: 0x{:x})", msg, m_error.code);
	}
	else
	{

		m_msg = std::format("{}: {} (error code: 0x{:x})", msg, m_error.message, m_error.code);
	}
}
