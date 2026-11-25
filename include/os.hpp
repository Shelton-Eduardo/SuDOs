#if !defined(OS_IMPLEMENTATION_HPP)
#define OS_IMPLEMENTATION_HPP

#if !defined(PAGE_BUFFER_SIZE)
#define PAGE_BUFFER_SIZE 4096
#endif

#if !defined(WRITE_MAX_SIZE)
#define WRITE_MAX_SIZE 2048
#endif

#if !defined(PATH_MAX_SIZE)
#define PATH_MAX_SIZE 1024
#endif

#ifndef MAX_GET_RANGE
    #define MAX_GET_RANGE 30
#endif

// Global headers
#include <exception>
#include <cstdint>
#include <cstddef>
#include <ostream>


namespace os{
    enum class Cursor : std::size_t{start = static_cast<std::size_t>(0),
                                    npos  = static_cast<std::size_t>(-1)};
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
        new_file         = 0b1000'0000, // if 0 it means find existing
        NONE             = 0b0000'0000
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

    constexpr OpenMode operator|(OpenMode lhs, OpenMode rhs) noexcept {
        return static_cast<OpenMode>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
    }

    constexpr OpenMode operator&(OpenMode lhs, OpenMode rhs) noexcept {
        return static_cast<OpenMode>(static_cast<std::uint8_t>(lhs) & static_cast<std::uint8_t>(rhs));
    }

    constexpr OpenMode operator>>(OpenMode lhs, int bitshift) noexcept {
        return static_cast<OpenMode>(static_cast<std::uint8_t>(lhs) >> bitshift);
    }

    constexpr Cursor operator+(Cursor lhs, Cursor rhs) noexcept {
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) + static_cast<std::size_t>(rhs));
    }

    constexpr Cursor operator-(Cursor lhs, Cursor rhs) noexcept {
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) - static_cast<std::size_t>(rhs));
    }

    constexpr Cursor operator-(Cursor lhs, int rhs) noexcept {
        if (lhs == os::Cursor::start) return static_cast<os::Cursor>(0);
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) - static_cast<std::size_t>(rhs));
    }

    constexpr Cursor operator/(Cursor lhs, Cursor rhs) noexcept {
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) / static_cast<std::size_t>(rhs));
    }

    constexpr Cursor operator*(Cursor lhs, Cursor rhs) noexcept {
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) * static_cast<std::size_t>(rhs));
    }

    constexpr Cursor operator%(Cursor lhs, Cursor rhs) noexcept {
        return static_cast<Cursor>(static_cast<std::size_t>(lhs) % static_cast<std::size_t>(rhs));
    }
}

namespace os::exceptions{
    // File Handling Exceptions
    class FileNotFound : public std::exception {
        private:
            const char* msg;
        public:
            FileNotFound(const char* m = "File not found.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileAlreadyExists : public std::exception {
        private:
            const char* msg;
        public:
            FileAlreadyExists(const char* m = "File already exists.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileAccessDenied : public std::exception {
        private:
            const char* msg;
        public:
            FileAccessDenied(const char* m = "File access denied.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileReadError : public std::exception {
        private:
            const char* msg;
        public:
            FileReadError(const char* m = "Failed to read from file.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileWriteError : public std::exception {
        private:
            const char* msg;
        public:
            FileWriteError(const char* m = "Failed to write to file.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileSeekError : public std::exception {
        private:
            const char* msg;
        public:
            FileSeekError(const char* m = "Failed to seek within file.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileUnknownError : public std::exception {
        private:
            const char* msg;
        public:
            FileUnknownError(const char* m = "An unknown file error occurred.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class FileNotOpenError : public std::exception {
        private:
            const char* msg;
        public:
            FileNotOpenError(const char* m = "File is not open.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };

    // Directory Handling Exceptions
    class DirectoryNotFound : public std::exception {
        private:
            const char* msg;
        public:
            DirectoryNotFound(const char* m = "Directory not found.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class DirectoryAlreadyExists : public std::exception {
        private:
            const char* msg;
        public:
            DirectoryAlreadyExists(const char* m = "Directory already exists.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class DirectoryAccessDenied : public std::exception {
        private:
            const char* msg;
        public:
            DirectoryAccessDenied(const char* m = "Directory access denied.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class DirectoryReadError : public std::exception {
        private:
            const char* msg;
        public:
            DirectoryReadError(const char* m = "Failed to read directory.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class DirectoryWriteError : public std::exception {
        private:
            const char* msg;
        public:
            DirectoryWriteError(const char* m = "Failed to write to directory.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };

    // Path Handling Exceptions
    class PathNotFound : public std::exception {
        private:
            const char* msg;
        public:
            PathNotFound(const char* m = "Path not found.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathAlreadyExists : public std::exception {
        private:
            const char* msg;
        public:
            PathAlreadyExists(const char* m = "Path already exists.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathAccessDenied : public std::exception {
        private:
            const char* msg;
        public:
            PathAccessDenied(const char* m = "Path access denied.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathReadError : public std::exception {
        private:
            const char* msg;
        public:
            PathReadError(const char* m = "Failed to read path.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathWriteError : public std::exception {
        private:
            const char* msg;
        public:
            PathWriteError(const char* m = "Failed to write to path.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathInvalid : public std::exception {
        private:
            const char* msg;
        public:
            PathInvalid(const char* m = "Invalid path.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathTooLong : public std::exception {
        private:
            const char* msg;
        public:
            PathTooLong(const char* m = "Path exceeds maximum allowed length.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathNotAbsolute : public std::exception {
        private:
            const char* msg;
        public:
            PathNotAbsolute(const char* m = "Path is not absolute.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathNotRelative : public std::exception {
        private:
            const char* msg;
        public:
            PathNotRelative(const char* m = "Path is not relative.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathNotCanonical : public std::exception {
        private:
            const char* msg;
        public:
            PathNotCanonical(const char* m = "Path is not canonical.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class PathNotNormalized : public std::exception {
        private:
            const char* msg;
        public:
            PathNotNormalized(const char* m = "Path is not normalized.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };

    // Memory & System Handling Exceptions
    class BufferFailure : public std::exception {
        private:
            const char* msg;
        public:
            BufferFailure(const char* m = "Stream buffer allocation failed.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
    class SystemFailureError : public std::exception {
        private:
            const char* msg;
        public:
            SystemFailureError(const char* m = "An underlying operating system failure occurred.") noexcept : msg(m) {}
            const char* what() const noexcept override {
                return msg;
            }
    };
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
                [[nodiscard]] bool is_open() const;

                // Stream Modification
                bool refil(bool update = true) noexcept;
        };
    }

#elif defined(__linux__)
        #ifndef _GNU_SOURCE
            #define _GNU_SOURCE
        #endif
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <cerrno>

    #define _FILE_OFFSET_BITS 64

    namespace FileStream{
        // FileStream class for Linux/Debian Enviroments
        class LinuxStreamBuffer : public StreamBuffer{
            protected:
                int file_handle;
                os::OpenMode flags;
                unsigned char padding[3] = {}; // currently, we have three bytes of padding, can be useful in the future for additional flag tracking
            public:
                LinuxStreamBuffer();
                LinuxStreamBuffer(const char* file, os::OpenMode method);
                LinuxStreamBuffer(const wchar_t* file, os::OpenMode method);
                ~LinuxStreamBuffer() noexcept;

                void open(const char* path, os::OpenMode method);
                void open(const wchar_t* path, os::OpenMode method);
                void close();

                // Stream state
                [[nodiscard]] bool is_open() const;

                // Stream Modification
                bool refil(bool update = true) noexcept;
        };
    }
    
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

            explicit operator bool() const noexcept;

            // Essential Read Operations
            [[nodiscard]] std::size_t read(unsigned char* container, std::size_t bytes) noexcept;
            unsigned char get();

            Ifstream& operator>>(unsigned char& container);
            Ifstream& operator>>(char& container);
            Ifstream& operator>>(long double& container);
            Ifstream& operator>>(double& container);
            Ifstream& operator>>(short& container);
            Ifstream& operator>>(float& container);
            Ifstream& operator>>(long& container);
            Ifstream& operator>>(int& container);
            Ifstream& operator>>(unsigned int& container);
            Ifstream& operator>>(unsigned long& container);
            Ifstream& operator>>(unsigned short& container);
            Ifstream& operator>>(unsigned long long& container);

            // Stream Positioning
            void seek(os::Cursor position);
            os::Cursor tell() const;

            static constexpr unsigned char EndOfFileEOF = -1;
            };
    
    class Ofstream :
    #if defined(_WIN32) || defined(_WIN64)
        public FileStream::WindowsStreamBuffer
    #else 
        public FileStream::LinuxStreamBuffer
    #endif
    {
        private:
            void seekp(os::Cursor pos_off) noexcept;
            void flush() noexcept;

        public:
            Ofstream() = default;
            Ofstream(const char* path, os::OpenMode mode = os::OpenMode::write | os::OpenMode::binary | os::OpenMode::new_file);
            #if defined(_WIN32) || defined(_WIN64)
                Ofstream(const wchar_t* path, os::OpenMode mode = os::OpenMode::write | os::OpenMode::binary | os::OpenMode::new_file);
            #endif
            ~Ofstream();

            Ofstream(const Ofstream&) = delete;
            Ofstream(Ofstream&&) noexcept = delete;
            Ofstream& operator=(const Ofstream&) = delete;
            Ofstream& operator=(Ofstream&&) noexcept = delete;
            explicit operator bool() const noexcept;

            // Essential Write Operations (binary)
            Ofstream& write(const unsigned char* container, std::size_t bytes) noexcept;
            Ofstream& write(const char* container, std::size_t bytes) noexcept;



            void close();
            
            Ofstream& operator<<(const unsigned char* buffer);
            Ofstream& operator<<(long double buffer);
            Ofstream& operator<<(const char* buffer);
            Ofstream& operator<<(long long buffer);
            Ofstream& operator<<(double buffer);
            Ofstream& operator<<(short buffer);
            Ofstream& operator<<(float buffer);
            Ofstream& operator<<(long buffer);
            Ofstream& operator<<(int buffer);
            Ofstream& operator<<(unsigned int buffer);
            Ofstream& operator<<(unsigned long buffer);
            Ofstream& operator<<(unsigned short buffer);
            Ofstream& operator<<(unsigned long long buffer);

            // Stream Positioning
            void seek(os::Cursor position);
            os::Cursor tell() const;

            // Public Vars
            static constexpr unsigned char EndOfFileEOF = -1;
            };
    
    }

// version two functions

// Private Data
namespace os::PRIVATE_DETAILS{
    struct ListDirs{
        std::size_t*    content_size;
        wchar_t**       content;
        std::size_t     len;

        ListDirs();
        ~ListDirs() noexcept;

        ListDirs(const ListDirs& source)                = delete;
        ListDirs(ListDirs&& source) noexcept;
        ListDirs& operator=(const ListDirs& source)     = delete;
        ListDirs& operator=(ListDirs&& source) noexcept = delete;

        void add(const wchar_t* data, std::size_t size);

        friend std::ostream& operator<<(std::ostream& printer_txt, const ListDirs& source){
        for(std::size_t i{}; i<source.len; ++i) printer_txt << os::GlobalFunctions::wide_to_ascii((source.content[i])).data <<
         "--------------------" << source.content_size[i] << '\n';
        return printer_txt;
    }

        private:
            void refil();
    };
}


namespace os{
    os::Wchar_t getcwd() noexcept;
    bool chwdir() noexcept;
    os::PRIVATE_DETAILS::ListDirs listdir(const wchar_t* path = os::getcwd().data);
}



    
#endif // OS_IMPLEMENTATION_HPP
