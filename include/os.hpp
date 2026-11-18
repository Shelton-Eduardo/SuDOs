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
    enum class Cursor : std::size_t{start = static_cast<std::size_t>(0)};
    /*
        OpenMode Rules
            Sharing Prop       Opening Prop
        0b  0   0   0   0   '   0   0   0   0
                                            read
                                        write
                                    append(1) / overwrite(0)
                                binary(1) / text(0)
                        Share Read
                    Share Write
                Share Delete
            New File/Overwrite(1) Open_Existing(0)
    */
    enum class OpenMode : std::uint8_t {
        read             = 0b0000'0001,
        write            = 0b0000'0010,
        read_write       = 0b0000'0011,
        append           = 0b0000'0100, // if 0 it means Overwrite
        binary           = 0b0000'1000, // if 0 it means text
        share_none       = 0b0000'0000,
        share_read       = 0b0001'0000,
        share_write      = 0b0010'0000,
        share_read_write = 0b0011'0000,
        share_delete     = 0b0100'0000,
        share_all        = 0b0111'0000,
        new_file         = 0b1000'0000
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
os::OpenMode operator>>(os::OpenMode lhs, int bitshift) noexcept;
os::Cursor operator+(os::Cursor lhs, os::Cursor rhs);
os::Cursor operator-(os::Cursor lhs, os::Cursor rhs);
os::Cursor operator-(os::Cursor lhs, int rhs);
os::Cursor operator/(os::Cursor lhs, os::Cursor rhs);
os::Cursor operator*(os::Cursor lhs, os::Cursor rhs);
os::Cursor operator%(os::Cursor lhs, os::Cursor rhs);


namespace os::exceptions{
    // File Handling Exceptions
    class FileNotFound : public std::exception{};
    class FileAlreadyExists : public std::exception{};
    class FileAccessDenied : public std::exception{};
    class FileReadError : public std::exception{};
    class FileWriteError : public std::exception{};
    class FileSeekError : public std::exception{};
    class FileUnknownError : public std::exception{};
    class FileNotOpenError : public std::exception{};
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
    // Memory Handling Exceptions
    class BufferFailure : public std::exception{};
}

namespace os::GlobalFunctions{
    Wchar_t ascii_to_wide(const char* ascii_str);
    Char wide_to_ascii(const wchar_t* wide_str);
}

class StreamBuffer{
    protected:
        static constexpr std::uint32_t buffer_size = (PAGE_BUFFER_SIZE % 4096 == 0) ? PAGE_BUFFER_SIZE : 4096;
    protected:
        unsigned char* buffer;
        unsigned char* current_cursor;
        unsigned char* end_cursor;
        os::Cursor offset;
    public:
        StreamBuffer();
        ~StreamBuffer();
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
            protected:
                HANDLE file_handle;
                os::OpenMode flags;
            public:
                WindowsStreamBuffer();
                WindowsStreamBuffer(const char* file, os::OpenMode method);
                WindowsStreamBuffer(const wchar_t* file, os::OpenMode method);
                ~WindowsStreamBuffer();

                void open(const char* path, os::OpenMode mode);
                void open(const wchar_t* path, os::OpenMode mode);
                void close();

                // Stream state
                bool is_open() const;

                // Stream Modification
                bool refil(bool update = true) noexcept;
        };
    }

#elif defined(__linux__)
    #include "os/linux.hpp"
#endif

namespace os{

    class Ifstream :
    #if defined(_WIN32) || defined(_WIN64)
        public FileStream::WindowsStreamBuffer
    #else 
        public FileStream::LinuxStreamBuffer
    #endif
    {
        public:
            Ifstream() = default;
            Ifstream(const char* path, os::OpenMode mode = os::OpenMode::read | os::OpenMode::binary);
            #if defined(_WIN32) || defined(_WIN64)
                Ifstream(const wchar_t* path, os::OpenMode mode = os::OpenMode::read | os::OpenMode::binary);
            #endif
            ~Ifstream() = default;

            Ifstream(const Ifstream&) = delete;
            Ifstream(Ifstream&&) noexcept = delete;
            Ifstream& operator=(const Ifstream&) = delete;
            Ifstream& operator=(Ifstream&&) noexcept = delete;

            // Essential Read Operations
            [[nodiscard]] std::size_t read(unsigned char* container, std::size_t bytes) noexcept;
            unsigned char get();

            // Stream Positioning
            void seek(os::Cursor position);
            os::Cursor tell() const;
            };
    
    class Ofstream :
    #if defined(_WIN32) || defined(_WIN64)
        public FileStream::WindowsStreamBuffer
    #else 
        public FileStream::LinuxStreamBuffer
    #endif
    {
        public:
            Ofstream() = default;
            Ofstream(const char* path, os::OpenMode mode = os::OpenMode::write | os::OpenMode::binary | os::OpenMode::new_file);
            #if defined(_WIN32) || defined(_WIN64)
                Ofstream(const wchar_t* path, os::OpenMode mode = os::OpenMode::read | os::OpenMode::binary | os::OpenMode::new_file);
            #endif
            ~Ofstream();

            Ofstream(const Ofstream&) = delete;
            Ofstream(Ofstream&&) noexcept = delete;
            Ofstream& operator=(const Ofstream&) = delete;
            Ofstream& operator=(Ofstream&&) noexcept = delete;

            // Essential Write Operations (binary)
            void write(const unsigned char* container, std::size_t bytes) noexcept;
            void write(const char* container, std::size_t bytes) noexcept;


            void flush() noexcept;

            void close();

            [[nodiscard]] std::size_t operator<<(const char* buffer);
            [[nodiscard]] std::size_t operator<<(const unsigned* buffer);
            [[nodiscard]] std::size_t operator<<(int buffer);
            [[nodiscard]] std::size_t operator<<(short buffer);
            [[nodiscard]] std::size_t operator<<(float buffer);
            [[nodiscard]] std::size_t operator<<(long long buffer);
            [[nodiscard]] std::size_t operator<<(double buffer);

            // Stream Positioning
            void seek(os::Cursor position);
            os::Cursor tell() const;
            };
    
    }
    
#endif // OS_IMPLEMENTATION_HPP
