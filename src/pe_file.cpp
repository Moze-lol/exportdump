#include "pe_file.hpp"
#include "windows_exception.hpp"
#include <format>
#include <cassert>

PEFile::PEFile(const std::filesystem::path& path) :
	m_path(path),
	m_mmap(path),
	m_dosHeader(),
	m_ntHeader(new std::byte[sizeof(IMAGE_NT_HEADERS64)], sizeof(IMAGE_NT_HEADERS64)), //IMAGE_NT_HEADERS64 will always use more memory than IMAGE_NT_HEADERS32, so we just allocate enough for both
	m_sections(),
	m_eat(),
	m_is32Bit(false)
{
	//NT and DOS headers
	m_mmap.read(m_dosHeader);
	m_mmap.read(m_ntHeader, m_dosHeader.e_lfanew);

	if (m_dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
	{
		throw std::runtime_error("Invalid PE file (IMAGE_DOS_HEADER.e_magic is incorrect!");
	}

	//The offset to the optional header is equal regardless of architecture
	WORD magic =  m_mmap.read<WORD>(m_dosHeader.e_lfanew + offsetof(IMAGE_NT_HEADERS, OptionalHeader));

	switch (magic)
	{
	case 0x20b: //64-bit magic identifier
		m_is32Bit = false;
		break;
	case 0x10b: //32-bit magic identifier
		m_is32Bit = true;
		break;
	default:
		throw std::runtime_error("Invalid PE file (IMAGE_NT_HEADERS.OptionalHeader.Magic is incorrect!");
	}

	//IMAGE_SECTION_HEADER data

	offset_t sectionsOffset;
	size_t numSections;

	if (m_is32Bit)
	{
		auto ntHeader = ntHeaders32();
		numSections = ntHeader->FileHeader.NumberOfSections;
		sectionsOffset = m_dosHeader.e_lfanew + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + ntHeader->FileHeader.SizeOfOptionalHeader; //Equivalent to IMAGE_FIRST_SECTION
		/*
		The reason we cant just use IMAGE_FIRST_SECTION is the way we read from the memory map.If the image section array happens to cross the boundry of our
		sliding window, bad things happen.
		*/
	}
	else
	{
		auto ntHeader = ntHeaders64();
		numSections = ntHeader->FileHeader.NumberOfSections;
		sectionsOffset = m_dosHeader.e_lfanew + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) + ntHeader->FileHeader.SizeOfOptionalHeader; //Equivalent to IMAGE_FIRST_SECTION
	}

	m_sections = std::span(new IMAGE_SECTION_HEADER[numSections], numSections);
	m_mmap.read(std::as_writable_bytes(m_sections), sectionsOffset);

	//EAT data

	size_t eatSize;

	if (m_is32Bit)
	{
		auto data = ntHeaders32()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		eatSize = data.Size;
		m_eat.offset = RVAToFileOffset(data.VirtualAddress);
	}
	else
	{
		auto data = ntHeaders64()->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
		eatSize = data.Size;
		m_eat.offset = RVAToFileOffset(data.VirtualAddress);
	}

	m_eat.bytes = std::span(new std::byte[eatSize], eatSize);
	m_mmap.read(m_eat.bytes, m_eat.offset);
	/*
	Again, have to load the entire EAT because of how we read the memory mapped file.
	It does allow us to use the EAT memory directly however, without needing any secondary storage.
	*/

	auto EATHeader = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(m_eat.bytes.data());
	m_eat.entries = EATHeader->NumberOfFunctions; //We only care about functions with name exports.
	m_eat.names = EATReadData<address_t>(RVAToFileOffset(EATHeader->AddressOfNames));
	m_eat.addresses = EATReadData<address_t>(RVAToFileOffset(EATHeader->AddressOfFunctions));
	m_eat.ordinals = EATReadData<ordinal_t>(RVAToFileOffset(EATHeader->AddressOfNameOrdinals));
}

PEFile::~PEFile()
{
	delete[] m_ntHeader.data();
	delete[] m_sections.data();
	delete[] m_eat.bytes.data();
}

EATEntry PEFile::EATEntryAt(size_t index) const
{
	assert(index < m_eat.entries);

	return {
		EATReadData<char>(RVAToFileOffset(m_eat.names[index])),
		m_eat.addresses[index],
		m_eat.ordinals[index]
	};
}

EATIterator PEFile::begin() const
{
	return EATIterator(*this, 0);
}

EATIterator PEFile::end() const
{
	return EATIterator(*this, m_eat.entries);
}

inline PEFile::offset_t PEFile::RVAToFileOffset(offset_t rva) const
{
	/*
	PE Virtual Addresses are supposed to be mapped in virtual memory, however we arent actually loading the DLL in to our address space.
	So we have to do this tomfoolery to get our offsets.
	*/
	for (const IMAGE_SECTION_HEADER& sectionHeader : m_sections)
	{
		if (rva >= sectionHeader.VirtualAddress && rva < (sectionHeader.VirtualAddress + sectionHeader.SizeOfRawData))
		{
			return rva - sectionHeader.VirtualAddress + sectionHeader.PointerToRawData;
		}
	}

	throw std::runtime_error("Failed to parse EAT: invalid RVA");
}