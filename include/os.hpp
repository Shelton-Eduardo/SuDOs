#if !defined(OS_IMPLEMENTATION_HPP)
#define OS_IMPLEMENTATION_HPP

#if !defined(PAGE_BUFFER_SIZE)
#define PAGE_BUFFER_SIZE 4096
#endif

// Global headers
#include <exception>
#include <streambuf>
#include <cstdint>
#include <cstddef>
#include <istream>
#include <ostream>


enum class Cursor : std::size_t{};
enum class OpenMode : std::uint8_t{
    read = 0b0000'0001,
    write = 0b0000'0010,
    read_write = 0b0000'0011,
    append = 0b0000'0100,
    binary = 0b0001'0000,
    text = 0b0010'0000
};

namespace os::exceptions{
    // File Handling Exceptions
    class FileNotFound : public std::exception{};
    class FileAlreadyExists : public std::exception{};
    class FileAccessDenied : public std::exception{};
    class FileReadError : public std::exception{};
    class FileWriteError : public std::exception{};
    class FileSeekError : public std::exception{};
    // Directory Handling Exceptions
    class DirectoryNotFound : public std::exception{};
    class DirectoryAlreadyExists : public std::exception{};
    class DirectoryAccessDenied : public std::exception{};
    class DirectoryReadError : public std::exception{};
    class DirectoryWriteError : public std::exception{};
    // Path Handling Exceptions
    class PathNotFound : public std::exception{};
    class PathAlreadyExists : public std::exception{};
    class PathAccessDenied : public std::exception{};
    class PathReadError : public std::exception{};
    class PathWriteError : public std::exception{};
    class PathInvalid : public std::exception{};
    class PathTooLong : public std::exception{};
    class PathNotAbsolute : public std::exception{};
    class PathNotRelative : public std::exception{};
    class PathNotCanonical : public std::exception{};
    class PathNotNormalized : public std::exception{};
}

class StreamBuffer{
    protected:
        static constexpr std::size_t buffer_size = (PAGE_BUFFER_SIZE % 4096 == 0) ? PAGE_BUFFER_SIZE : 4096;
    private:
        unsigned char* buffer[buffer_size];
        Cursor cursor{0};
    public:
        StreamBuffer() = default;
        ~StreamBuffer() = default;

        std::size_t read(unsigned char* data, std::size_t size);
        std::size_t write(const unsigned char* data, std::size_t size);
        void seek(Cursor position);
};

#endif // OS_IMPLEMENTATION_HPP
