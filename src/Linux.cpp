#include "os.hpp"

#if defined(__linux__)

StreamBuffer::StreamBuffer(){
    buffer = static_cast<unsigned char*>(mmap(
        nullptr,
        buffer_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0
    ));
    if(buffer == MAP_FAILED) os::exceptions::BufferFailure{};
    end_cursor = current_cursor = buffer;
    offset = os::Cursor::start;
}

StreamBuffer::~StreamBuffer(){
    if(!buffer || buffer == MAP_FAILED) return ;
    if(munmap(buffer, buffer_size) == -1){
        // still to figure out what happens if it fails
    }
    end_cursor = current_cursor = buffer = nullptr;
}


FileStream::LinuxStreamBuffer::LinuxStreamBuffer() : StreamBuffer(), file_handle(-1), flags(os::OpenMode::NONE){}

FileStream::LinuxStreamBuffer::LinuxStreamBuffer(const char* file, os::OpenMode method) : StreamBuffer(){
    open(file, method);
}

FileStream::LinuxStreamBuffer::LinuxStreamBuffer(const wchar_t* file, os::OpenMode method) : LinuxStreamBuffer(os::GlobalFunctions::wide_to_ascii(file).data, method){}

FileStream::LinuxStreamBuffer::~LinuxStreamBuffer() noexcept{
    if(file_handle == -1) return ;
    if(::close(file_handle) != 0) throw os::exceptions::FileUnknownError{};
    file_handle = -1;
}

void FileStream::LinuxStreamBuffer::open(const char* path, os::OpenMode mode) {
    if (file_handle != -1) return;

    int flags = 0;
    if ((mode & os::OpenMode::read_write) == os::OpenMode::read_write || 
        static_cast<bool>(mode & os::OpenMode::append)) {
        flags |= O_RDWR;
    } else if (static_cast<bool>(mode & os::OpenMode::read)) {
        flags |= O_RDONLY;
    } else if (static_cast<bool>(mode & os::OpenMode::write)) {
        flags |= O_WRONLY;
    }
    // Handle structural append mode flag
    if (static_cast<bool>(mode & os::OpenMode::append)) {
        flags |= O_APPEND;
    }
    // Determine Creation Disposition Flags
    if (static_cast<bool>(mode & os::OpenMode::new_file)) {
        flags |= O_CREAT | O_TRUNC;
    } else if (static_cast<bool>(mode & (os::OpenMode::write | os::OpenMode::append))) {
        flags |= O_CREAT;
    }

    flags |= O_DIRECT; 
    
    if (static_cast<bool>(mode & (os::OpenMode::write | os::OpenMode::append))) {
        flags |= O_SYNC; 
    }

    file_handle = ::open(path, flags, 0666);

    if (file_handle != -1 && (mode & os::OpenMode::share_all) != os::OpenMode::share_all) {
        int lock_flags = 0;
        
        if (!static_cast<bool>(mode & os::OpenMode::share_write)) {
            lock_flags = (flags & O_RDONLY) ? LOCK_SH : LOCK_EX;
        }
        if ((mode & os::OpenMode::share_all) == os::OpenMode::share_none) {
            lock_flags = LOCK_EX;
        }

        if (lock_flags != 0 && flock(file_handle, lock_flags | LOCK_NB) == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                ::close(file_handle);
                file_handle = -1;
                throw os::exceptions::FileAccessDenied(); // Sharing Violation
            }
        }
    }

    if (file_handle == -1) {
        switch (errno) {
            case ENOENT:
                throw os::exceptions::FileNotFound();
            case ENOTDIR:
                throw os::exceptions::PathNotFound();
            case EACCES:
            case EPERM:
                throw os::exceptions::FileAccessDenied();
            case EEXIST:
                throw os::exceptions::FileAlreadyExists();
            default:
                throw os::exceptions::FileUnknownError();
        }
    }
}

#endif 