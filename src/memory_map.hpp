#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <span>
#include <Windows.h>

/*
While we could have mapped a view of the entire file in memory, this design uses a "sliding window" approach.
This is manily to avoid imploding a system if the user inputs a very large file.
There will only ever be 1 mapped view of the file at a time, with a window size equal to the system's allocation granularity
(MapViewOfFile has very strict requirements for view offsets).
*/
class MemoryMap
{
public:
	MemoryMap(const std::filesystem::path& path);
	MemoryMap(const MemoryMap&) = delete;
	~MemoryMap();

	size_t size() const { return m_fileSize; }

	void read(std::span<std::byte> buffer, size_t offset = 0);

	template<typename T> requires(std::is_pod_v<T>)
	void read(T& data, size_t offset = 0) { read(std::as_writable_bytes(std::span(&data, 1)), offset); }

	template<typename T> requires(std::is_pod_v<T>)
	T read(size_t offset = 0)
	{
		T data{};
		read(std::as_writable_bytes(std::span(&data, 1)), offset);
		return data;
	}

private:
	inline void mapView(size_t offset);

	std::filesystem::path m_path;
	HANDLE m_file;
	HANDLE m_mmap;
	size_t m_fileSize;
	std::span<std::byte> m_view;
	size_t m_viewOffset;
};