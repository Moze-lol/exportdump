#include "memory_map.hpp"
#include "windows_exception.hpp"
#include <cassert>
#include <algorithm>

static const DWORD ALLOCATION_GRANULARITY =  //Usually 65k
[]() 
{
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwAllocationGranularity;
}();

static const size_t MAX_VIEW_SIZE = ALLOCATION_GRANULARITY; //Could be any multiple of this, but a 65k read window is fine

MemoryMap::MemoryMap(const std::filesystem::path& path) :
	m_path(path),
	m_file(INVALID_HANDLE_VALUE),
	m_mmap(),
	m_view(),
	m_viewOffset(-1)
{
	m_file = CreateFileW(m_path.wstring().c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);

	if (m_file == INVALID_HANDLE_VALUE)
	{
		throw WindowsException("Failed to memory map input file (CreateFile)");
	}

	LARGE_INTEGER size{};
	if (!GetFileSizeEx(m_file, &size))
	{
		throw WindowsException("Failed to memory map input file (GetFileSizeEx)");
	}
	m_fileSize = size.QuadPart;

	m_mmap = CreateFileMappingA(m_file, nullptr, PAGE_READONLY, 0, 0, nullptr);

	if (!m_mmap)
	{
		throw WindowsException("Failed to memory map input file (CreateFileMapping)");
	}

	//The view will actually be mapped on the first read
}

MemoryMap::~MemoryMap()
{
	UnmapViewOfFile(m_view.data());
	CloseHandle(m_mmap);
	CloseHandle(m_file);
}

void MemoryMap::read(std::span<std::byte> buffer, size_t offset)
{
	assert((offset + buffer.size_bytes()) < m_fileSize);
	size_t bytesRead = 0;

	do
	{
		if (offset < m_viewOffset || (offset + buffer.size_bytes() - bytesRead) > (m_viewOffset + m_view.size_bytes()))
		{
			mapView(offset);
		}

		//Have to account for extra bytes that are not aligned to the allocation granularity
		size_t viewOffset = offset - m_viewOffset; //Will only ever be > 0 for the first iteratiom
		size_t toRead = std::min(buffer.size_bytes() - bytesRead, m_view.size_bytes() - viewOffset);
		memcpy(buffer.data() + bytesRead, m_view.data() + viewOffset, toRead);
		bytesRead += toRead;
		offset += toRead;
	} while (bytesRead < buffer.size_bytes());
}

inline void MemoryMap::mapView(size_t offset)
{
	assert(offset < m_fileSize);

	UnmapViewOfFile(m_view.data());

	size_t viewStart = (offset / ALLOCATION_GRANULARITY) * ALLOCATION_GRANULARITY; //Round to the nearest multiple
	size_t viewEnd = std::min(viewStart + MAX_VIEW_SIZE, viewStart + (m_fileSize - viewStart)); //Dont map over the maximum file size, bad things happen
	size_t viewSize = viewEnd - viewStart;

	ULARGE_INTEGER largeViewStart{ .QuadPart = viewStart };
	m_view = std::span(reinterpret_cast<std::byte*>(MapViewOfFile(m_mmap, FILE_MAP_READ, largeViewStart.HighPart, largeViewStart.LowPart, viewSize)), viewSize);

	if (!m_view.data())
	{
		throw WindowsException("Failed to memory map input file (MapViewofFile)");
	}

	m_viewOffset = viewStart;
}