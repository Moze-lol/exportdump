#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <span>
#include <Windows.h>

class Demangler
{
public:
	Demangler();
	~Demangler();
	Demangler(const Demangler&) = delete;
	Demangler(Demangler&&) = delete;

	//Subsequent calls to demangle() invalidate the returned string_view
	std::string_view demangle(std::string_view name) const;

private:
	static constexpr const size_t BUFFER_SIZE = 65'536;
	static inline auto const s_buffer = std::make_unique<char[]>(BUFFER_SIZE);

	HANDLE m_handle;
};