#pragma once

#include "memory_map.hpp"
#include <filesystem>
#include <string>
#include <string_view>
#include <span>
#include <Windows.h>

class EATIterator;
struct EATEntry;

class PEFile
{
public:
	using offset_t = DWORD; //PE files only define 32bit offsets (of any kind)
	using size_t = DWORD;
	using address_t = DWORD;
	using ordinal_t = WORD;

	PEFile(const std::filesystem::path& path);
	~PEFile();
	PEFile(const PEFile&) = delete;
	PEFile(PEFile&&) = delete;

	inline const std::filesystem::path& path() const { return m_path; }
	inline bool is32Bit() const { return m_is32Bit; }


	size_t EATEntryCount() const { return m_eat.entries; }
	EATEntry EATEntryAt(size_t index) const;

	EATIterator begin() const;
	EATIterator end() const;

private:
	inline offset_t RVAToFileOffset(offset_t rva) const;
	inline const auto* ntHeaders32() const { return reinterpret_cast<IMAGE_NT_HEADERS32*>(m_ntHeader.data()); }
	inline const auto* ntHeaders64() const { return reinterpret_cast<IMAGE_NT_HEADERS64*>(m_ntHeader.data()); }

	template<typename T>
	inline const T* EATReadData(offset_t fileOffset) const { return reinterpret_cast<T*>(m_eat.bytes.data() + fileOffset - m_eat.offset); }

	std::filesystem::path m_path;
	MemoryMap m_mmap;
	IMAGE_DOS_HEADER m_dosHeader;
	std::span<std::byte> m_ntHeader; //Raw memory, since NtHeaders differ in layout depending on arch
	std::span<IMAGE_SECTION_HEADER> m_sections;

	struct EAT
	{
		std::span<std::byte> bytes; 
		offset_t offset; 
		size_t entries;
		const address_t* addresses;
		const address_t* names; //The name array is actually an array of RVAs.
		const ordinal_t* ordinals;
	} m_eat;

	bool m_is32Bit;
};

struct EATEntry
{
	const std::string_view name;
	const PEFile::address_t address;
	const PEFile::ordinal_t ordinal;
};

class EATIterator
{
public:
	EATEntry operator*() const { return m_pe.EATEntryAt(m_index); }
	auto& operator++() { ++m_index; return *this; }
	auto operator++(int) { auto temp = *this; ++(*this); return temp; }
	auto operator<=>(const EATIterator&) const = default;
	auto operator==(const EATIterator& other) const { return std::addressof(m_pe) == std::addressof(other.m_pe) && m_index == other.m_index; }

private:
	friend class PEFile;
	EATIterator(const PEFile& pe, PEFile::size_t index) : m_pe(pe), m_index(index) { }

	const PEFile& m_pe;
	PEFile::size_t  m_index;
};
