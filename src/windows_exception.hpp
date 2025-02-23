#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <Windows.h>

struct WindowsError
{
	DWORD code;
	std::string_view message; //If an error string could not be generated for the error code, will be empty

	inline WindowsError() : WindowsError(GetLastError()) {}
	WindowsError(DWORD errorCode);
	~WindowsError();
};


class WindowsException : public std::exception
{
public:
	inline WindowsException(std::string_view msg) : WindowsException(msg, GetLastError()) { }
	WindowsException(std::string_view msg, DWORD code);

	const char* what() const override { return m_msg.c_str(); }

	const WindowsError& error() const { return m_error; }

private:
	WindowsError m_error;
	std::string m_msg;
};