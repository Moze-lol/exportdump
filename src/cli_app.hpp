#pragma once

#include "pe_file.hpp"
#include "demangler.hpp"
#include "dump_writer.hpp"
#include <filesystem>
#include <memory>


class CLIApp
{
public:
	enum class OutputType
	{
		TEXT,
		CPP,
		JSON
	};

	CLIApp(int argc, const char** argv);

	void run();

private:
	OutputType m_outType;
	std::filesystem::path m_inputPath;
	std::filesystem::path m_outputPath;
};