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
    file_handle = -1;
}

void FileStream::LinuxStreamBuffer::open(const char* path, os::OpenMode mode){
    if(file_handle != -1) return ;
    file_handle = ::open(
        path,
        // Read_Write Conditions
        ((mode & os::OpenMode::read_write) == os::OpenMode::read_write) ?
            (static_cast<bool>(mode & os::OpenMode::new_file)) ? // Cond True (read_write + new_file)
                (static_cast<bool>(mode & os::OpenMode::append)) ?
                    O_RDWR | O_DIRECT | O_DSYNC | O_CREAT : O_RDWR | O_DIRECT | O_DSYNC | O_CREAT | O_TRUNC
                : // Cond False (read_write + existing file)
                (static_cast<bool>(mode & os::OpenMode::append)) ?
                    O_RDWR | O_DIRECT | O_DSYNC : O_RDWR | O_DIRECT | O_DSYNC | O_TRUNC
        :
        // Cond False (ReadWrite) -> Write Only Conditions
        (static_cast<bool>(mode & os::OpenMode::write)) ?
            (static_cast<bool>(mode & os::OpenMode::new_file)) ? // Cond True (write + new_file)
                (static_cast<bool>(mode & os::OpenMode::append)) ?
                    O_RDWR | O_DIRECT | O_DSYNC | O_CREAT : O_WRONLY | O_DIRECT | O_DSYNC | O_CREAT | O_TRUNC
                : // Cond False (write + existing file)
                (static_cast<bool>(mode & os::OpenMode::append)) ?
                    O_RDWR | O_DIRECT | O_DSYNC : O_WRONLY | O_DIRECT | O_DSYNC | O_TRUNC
        :
        // Cond False (Write) -> Read Only by default
        (static_cast<bool>(mode & os::OpenMode::new_file)) ?
            O_RDONLY | O_DIRECT | O_CREAT : O_RDONLY | O_DIRECT
        ,
        0666
    );
    if (file_handle == -1) {
        switch (errno) {
            case ENOENT:
                throw os::exceptions::FileNotFound();
            case ENOTDIR:
                throw os::exceptions::PathNotFound();
            case EACCES:
            case EPERM:
            case EROFS:
            case ETXTBSY:
            case EBUSY: // Equivalent to ERROR_SHARING_VIOLATION if flock/FUSE denies access
                throw os::exceptions::FileAccessDenied();
            case EEXIST:
                throw os::exceptions::FileAlreadyExists();
            case ENAMETOOLONG:
                throw os::exceptions::PathTooLong();
            case EISDIR:
            case ELOOP:
            case EFAULT:
                throw os::exceptions::PathInvalid();
            default:
                throw os::exceptions::FileUnknownError();
        }
    }

    if (static_cast<bool>(mode & os::OpenMode::write) && static_cast<bool>(mode & os::OpenMode::append)) {
        struct stat st {};
        if (::fstat(file_handle, &st) == -1) throw os::exceptions::FileReadError();
        const off_t file_size = st.st_size;
        const off_t setoffset = file_size - (file_size % buffer_size);
        offset = static_cast<os::Cursor>(file_size / buffer_size);
        if (::lseek(file_handle, setoffset, SEEK_SET) == -1) throw os::exceptions::FileSeekError();
        if ((file_size % buffer_size) > 0) refil(false);

        current_cursor = buffer + (file_size % buffer_size);
        end_cursor = buffer + buffer_size;

        if (::lseek(file_handle, setoffset, SEEK_SET) == -1) throw os::exceptions::FileSeekError();
    }
    flags = mode;

}

void FileStream::LinuxStreamBuffer::open(const wchar_t* path, os::OpenMode method){
    return this->open(os::GlobalFunctions::wide_to_ascii(path).data, method);
}

void FileStream::LinuxStreamBuffer::close(){
    if(file_handle == -1) return ;
    if(::close(file_handle) == 0) return ;
    switch (errno)
    {
    case EBADF:
        throw os::exceptions::FileUnknownError{};
    case EINTR:
        throw os::exceptions::SystemFailureError{};
    case EIO:
        throw os::exceptions::SystemFailureError{};
    case ENOSPC:
    case EDQUOT:
        throw os::exceptions::SystemFailureError{};    
    default:
        throw os::exceptions::FileUnknownError{};
    }
}

#endif 

#include <iostream>

int main(){
    std::cout << "Hello Word";
    return 0;
}