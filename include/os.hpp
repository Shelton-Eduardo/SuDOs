#if !defined(OS_IMPLEMENTATION_HPP)
#define OS_IMPLEMENTATION_HPP

#if !defined(PAGE_BUFFER_SIZE)
#define PAGE_BUFFER_SIZE 4096
#endif

#if !defined(PATH_MAX_SIZE)
#define PATH_MAX_SIZE 1024
#endif

// Global headers
#include <exception>
#include <cstdint>
#include <cstddef>


namespace os{
    enum class Cursor : std::size_t{};
    enum class OpenMode : std::uint8_t{
        read = 0b0000'0001,
        write = 0b0000'0010,
        read_write = 0b0000'0011,
        append = 0b0000'0100,
        binary = 0b0001'0000,
        text = 0b0010'0000
    };

    struct Wchar_t{
        wchar_t* data = nullptr;
        Wchar_t() = default;
        Wchar_t(std::size_t len);
        ~Wchar_t() noexcept;
        Wchar_t(const Wchar_t& source) = delete;
        Wchar_t& operator=(const Wchar_t& source) = delete;
        Wchar_t(Wchar_t&& source) noexcept;
        Wchar_t& operator=(Wchar_t&& source) noexcept;

        explicit operator bool() const{return (data) ? true : false;}
    };

    struct Char{
        char* data = nullptr;
        Char() = default;
        Char(std::size_t len);
        ~Char() noexcept;
        Char(const Char& source) = delete;
        Char& operator=(const Char& source) = delete;
        Char(Char&& source) noexcept;
        Char& operator=(Char&& source) noexcept;

        explicit operator bool() const{return (data) ? true : false;}
    };
}

os::OpenMode operator|(os::OpenMode lhs, os::OpenMode rhs);
os::OpenMode operator&(os::OpenMode lhs, os::OpenMode rhs);
os::Cursor operator+(os::Cursor lhs, os::Cursor rhs);
os::Cursor operator-(os::Cursor lhs, os::Cursor rhs);

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

namespace os::GlobalFunctions{
    Wchar_t ascii_to_wide(const char* ascii_str);
    Char wide_to_ascii(const wchar_t* wide_str);
}

class StreamBuffer{
    protected:
        static constexpr std::size_t buffer_size = (PAGE_BUFFER_SIZE % 4096 == 0) ? PAGE_BUFFER_SIZE : 4096;
    private:
        unsigned char* buffer;
        unsigned char* current_cursor;
        unsigned char* end_cursor;
        os::Cursor offset;
    public:
        StreamBuffer();
        ~StreamBuffer() = default;
};



#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <Windows.h>

    namespace FileStream{
        // FileStream class for Windows
        class WindowsStreamBuffer : public StreamBuffer{
            private:
                HANDLE file_handle;
            public:
                WindowsStreamBuffer() = default;
                ~WindowsStreamBuffer() = default;

                void open(const char* path, os::OpenMode mode);
                void open(const wchar_t* path, os::OpenMode mode);
                void close();
                std::size_t read(char* buffer, std::size_t size);
                std::size_t write(const char* buffer, std::size_t size);

                // Stream state
                bool is_open() const;
        };

        class Ifstream : public WindowsStreamBuffer{
            public:
                Ifstream() = default;
                Ifstream(const char* path, os::OpenMode mode = os::OpenMode::read | os::OpenMode::binary);
                Ifstream(const wchar_t* path, os::OpenMode mode = os::OpenMode::read | os::OpenMode::binary);
                ~Ifstream() = default;

                Ifstream(const Ifstream&) = delete;
                Ifstream(Ifstream&&) noexcept = delete;
                Ifstream& operator=(const Ifstream&) = delete;
                Ifstream& operator=(Ifstream&&) noexcept = delete;

                // Essential Read Operations
                std::size_t read(void* container, std::size_t bytes) noexcept;
                unsigned char get();

                // Stream Positioning
                void seek(os::Cursor position);
                os::Cursor tell() const;
        };
    }

#elif defined(__linux__)
    #include "os/linux.hpp"
#endif

#endif // OS_IMPLEMENTATION_HPP
