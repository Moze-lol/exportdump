#include "cli_app.hpp"
#include "console.hpp"

int main(int argc, const char** argv)
{
	try
	{
		CLIApp app(argc, argv);
		app.run();
	}
	catch (const std::exception& e)
	{
		Console::println(e.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}