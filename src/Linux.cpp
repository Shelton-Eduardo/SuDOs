#include "os.hpp"
#include <iostream>

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
    offset = os::Cursor::npos;
}

StreamBuffer::~StreamBuffer(){
    if(!buffer || buffer == MAP_FAILED) return ;
    if(munmap(buffer, buffer_size) == -1){
        // still to figure out what happens if it fails
        1+1;
    }
    end_cursor = current_cursor = buffer = nullptr;
}


FileStream::LinuxStreamBuffer::LinuxStreamBuffer() : StreamBuffer(), file_handle(-1){}

FileStream::LinuxStreamBuffer::LinuxStreamBuffer(const char* file, os::OpenMode method) : StreamBuffer(){
    file_handle = -1;
    open(file, method);
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

FileStream::LinuxStreamBuffer::LinuxStreamBuffer(const wchar_t* file, os::OpenMode method) : LinuxStreamBuffer(os::GlobalFunctions::wide_to_ascii(file).data, method){}

FileStream::LinuxStreamBuffer::~LinuxStreamBuffer() noexcept{
    this->close();
}

void FileStream::LinuxStreamBuffer::close(){
    if(file_handle == -1) return ;
    if(::close(file_handle) == -1){
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
    file_handle = -1;
}


bool FileStream::LinuxStreamBuffer::is_open() const{
    return file_handle != -1;
}


bool FileStream::LinuxStreamBuffer::refil(bool update) noexcept{
    if(file_handle == -1){
        current_cursor = end_cursor = buffer;
        return false;
    }
    ssize_t read = ::read(
        file_handle,
        buffer,
        buffer_size
    );
    if(read == -1){ // the function is no except, so we cant throw but during debugging we need to know whats going
        current_cursor = end_cursor = buffer;
        switch (errno){
            case EAGAIN:
                return false;
            case EBADF:
                return false;
            case EINTR:
                return false;
            case EFAULT:
                return false;
            case EINVAL:
            case EIO:
                return false;
            default:
                return false;
        }
    }
    if (read == 0){
        current_cursor = end_cursor = buffer;
        return false;
    }
    if(update) offset = offset + static_cast<os::Cursor>(1);
    current_cursor = buffer;
    end_cursor = buffer + read;
    return true;
}

os::Ifstream::Ifstream(const char* path, os::OpenMode mode) : LinuxStreamBuffer(path, mode){}

void os::Ifstream::seek(os::Cursor position){
    off_t aligned_pos = 
        static_cast<std::size_t>(position / static_cast<os::Cursor>(buffer_size) * static_cast<os::Cursor>(buffer_size));
    off_t state = lseek( // debugging purposes, remove the var in the end
        file_handle,
        aligned_pos,
        SEEK_SET
    );
    if(!refil(false)) throw os::exceptions::FileSeekError{};
    if((buffer + (static_cast<std::size_t>(position) % buffer_size)) >= end_cursor) throw os::exceptions::FileSeekError{};
    current_cursor = buffer + (static_cast<std::size_t>(position) % buffer_size);
    offset = position / static_cast<os::Cursor>(buffer_size);
}

os::Ofstream::Ofstream(const char* path, os::OpenMode mode) : LinuxStreamBuffer(path, mode){
    if(offset == os::Cursor::npos){
        offset = os::Cursor::start;
        end_cursor = buffer + buffer_size;
    }
}

void os::Ofstream::seekp(os::Cursor pos_off) noexcept{
    off_t aligned_pos = 
        static_cast<std::size_t>(pos_off * static_cast<os::Cursor>(buffer_size));
    lseek(
        file_handle,
        aligned_pos,
        SEEK_SET
    );
}


void os::Ofstream::flush() noexcept{
    if(file_handle == -1) return ;
    this->seekp(offset);
    ssize_t written = ::write(
        file_handle,
        buffer,
        buffer_size
    );
    if(written == -1 || written == 0) return ;
    offset = offset + static_cast<os::Cursor>(1ULL);
    current_cursor = buffer;
    end_cursor = buffer + buffer_size;
}

void os::Ofstream::seek(os::Cursor position){
    flush();
    off_t aligned_pos = 
        static_cast<std::size_t>(position / static_cast<os::Cursor>(buffer_size) * static_cast<os::Cursor>(buffer_size));
    off_t state = lseek( // debugging purposes, remove the var in the end
        file_handle,
        aligned_pos,
        SEEK_SET
    );
    if(!refil(false)) throw os::exceptions::FileSeekError{};
    if((buffer + (static_cast<std::size_t>(position) % buffer_size)) >= end_cursor) throw os::exceptions::FileSeekError{};
    current_cursor = buffer + (static_cast<std::size_t>(position) % buffer_size);
    offset = position / static_cast<os::Cursor>(buffer_size);

}

void os::Ofstream::close(){
    if(file_handle == -1) return ;
    std::size_t total_size = (static_cast<std::size_t>(offset) * buffer_size) + (current_cursor - buffer);
    flush();
    ftruncate64(file_handle, total_size);
    current_cursor = end_cursor = buffer;
    offset = os::Cursor::npos;
}

os::Ofstream::~Ofstream(){
    close();
}

#endif 