#pragma once

#include <format>
#include <iostream>
#include <string>
#include <string_view>

class Console
{
public:
	Console() = delete;

	inline static void println(std::string_view str)
	{
		std::cout << str << std::endl;
	}

	template<typename... Args>
	inline static void println(const std::format_string<Args...> format, Args&&... args)
	{
		std::string str = std::format(format, std::forward<Args>(args)...);
		println(str);
	}

	inline static void print(std::string_view str)
	{
		std::cout << str << std::flush;
	}

	template<typename... Args>
	inline static void print(const std::format_string<Args...> format, Args&&... args)
	{
		std::string str = std::format(format, std::forward<Args>(args)...);
		print(str);
	}
};