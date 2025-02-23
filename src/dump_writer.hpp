#pragma once

#include "pe_file.hpp"
#include "demangler.hpp"
#include <cstddef>
#include <format>
#include <filesystem>
#include <span>

class IDumpWriter
{
public:
	IDumpWriter(const std::filesystem::path& path);
	IDumpWriter(const IDumpWriter&) = delete;
	virtual ~IDumpWriter();

	inline void dumpExports(const PEFile& peFile, const Demangler& demangler)
	{
		dumpExportsImpl(peFile, demangler);
		flush();
	}

protected:
	virtual void dumpExportsImpl(const PEFile& peFile, const Demangler& demangler) = 0;

	void write(std::span<const std::byte> data);
	inline void write(std::string_view str) { write(std::as_bytes(std::span(str.begin(), str.end()))); }
	inline void writeln(const std::string& str) { write(str + '\n'); }

	template<typename... Args>
	inline void write(const std::format_string<Args...> format, Args&&... args) { write(std::format(format, std::forward<Args>(args)...)); }

	template<typename... Args>
	inline void writeln(const std::format_string<Args...> format, Args&&... args) { writeln(std::format(format, std::forward<Args>(args)...)); }

	const std::filesystem::path m_path;

private:
	void flush();

	HANDLE m_file; //My hatred for C++ IO streams is unending. (while I could've used C's FILE operations, I prefer more control over errors and buffering)
	std::byte* m_writeBuffer;
	size_t m_writeBufferSize;
	size_t m_writeBufferOffset;
};


class TXTDumpWriter : public IDumpWriter
{
public:
	using IDumpWriter::IDumpWriter;
protected:
	void dumpExportsImpl(const PEFile& peFile, const Demangler& demangler) override;
};

class CPPDumpWriter : public IDumpWriter
{
public:
	using IDumpWriter::IDumpWriter;
protected:
	void dumpExportsImpl(const PEFile& peFile, const Demangler& demangler) override;
};

class JSONDumpWriter : public IDumpWriter
{
public:
	using IDumpWriter::IDumpWriter;
protected:
	void dumpExportsImpl(const PEFile& peFile, const Demangler& demangler) override;
};