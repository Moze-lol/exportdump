#include "cli_app.hpp"
#include "console.hpp"
#include <chrono>

static inline std::filesystem::path getPath(const char* arg)
{
	std::string_view str(arg);

	if (str.starts_with('"'))
	{
		str = std::string_view(str.begin() + 1, str.end());
	}

	if (str.ends_with('"'))
	{
		str = std::string_view(str.begin(), str.end() - 1);
	}

	return str;
}

CLIApp::CLIApp(int argc, const char** argv) :
	m_outType(OutputType::CPP),
	m_inputPath(),
	m_outputPath()
{
	constexpr const char* HELP_MSG = R"(Invalid arguments. Usage: outputdump.exe <input file> <output file> [options]
	<input file>  path to a valid PE file
	<output file> path to the output dump
	options include:
		--text
			File output will be plain text, each line defining a single export in the format: <ORDINAL> <FUNCTION> <NAME> <DEMANGLED NAME>
		--cpp
			File output will be a c++ header file with a compile time lookup table in the namespace exports::<INPUT FILE NAME>
			This is the default mode.
		--json
			File output will be a json file, defined as an array of objects with ordinal, address, mangledName, demangledName;
			This mode is not yet implemented.)";

	if (argc < 3 || argc > 4)
	{
		throw std::invalid_argument(HELP_MSG);
	}

	m_inputPath = getPath(argv[1]);
	m_outputPath = getPath(argv[2]);

	if (argc == 4)
	{
		std::string_view outputTypeArg = argv[3];

		if (outputTypeArg == "--text" || outputTypeArg == "-text")
		{
			m_outType = OutputType::TEXT;
		}
		else if (outputTypeArg == "--cpp" || outputTypeArg == "-cpp")
		{
			m_outType = OutputType::CPP;
		}
		else if (outputTypeArg == "--json" || outputTypeArg == "-json")
		{
			m_outType = OutputType::JSON;
		}
		else
		{
			throw std::invalid_argument(HELP_MSG);
		}
	}
}

void CLIApp::run()
{
	namespace time = std::chrono;
	auto start = time::high_resolution_clock::now();

	PEFile peFile(m_inputPath);

	Console::println("Parsed .dll file: \"{}\"", m_inputPath.filename().string());
	Console::println("Architecture: {}", peFile.is32Bit() ? "x32" : "x64");
	Console::println("{} named function exports", peFile.EATEntryCount());

	std::unique_ptr<IDumpWriter> writer;

	switch (m_outType)
	{
	case OutputType::TEXT:
		writer = std::make_unique<TXTDumpWriter>(m_outputPath);
		break;
	case OutputType::CPP:
		writer = std::make_unique<CPPDumpWriter>(m_outputPath);
		break;
	case OutputType::JSON:
		writer = std::make_unique<JSONDumpWriter>(m_outputPath);
		break;
	}

	Demangler demangler;
	writer->dumpExports(peFile, demangler);

	auto end = time::high_resolution_clock::now();
	auto millis = time::round<time::milliseconds>(end - start);

	Console::println("Export dump written to {} in {}", m_outputPath.filename().string(), millis);
}
