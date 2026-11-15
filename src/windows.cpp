#include "os.hpp"

#if defined(_WIN32) || defined(_WIN64)

StreamBuffer::StreamBuffer(){
    buffer = static_cast<unsigned char*>(VirtualAlloc(
        NULL,
        buffer_size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    ));
    if(!buffer) throw os::exceptions::BufferFailure{};
    current_cursor = buffer;
    end_cursor = buffer + buffer_size;
    offset = os::Cursor::start;
}

StreamBuffer::~StreamBuffer(){
    if(buffer){
        VirtualFree(buffer, 0, MEM_RELEASE);
        current_cursor = end_cursor = buffer = nullptr;
    }
}


FileStream::WindowsStreamBuffer::WindowsStreamBuffer() : StreamBuffer(), file_handle(INVALID_HANDLE_VALUE){}

FileStream::WindowsStreamBuffer::WindowsStreamBuffer(const char* file, os::OpenMode method) : WindowsStreamBuffer(os::GlobalFunctions::ascii_to_wide(file).data, method) {}

FileStream::WindowsStreamBuffer::WindowsStreamBuffer(const wchar_t* file, os::OpenMode method) : StreamBuffer(){
    file_handle = CreateFileW(
        file,
        ((method & os::OpenMode::read_write) == os::OpenMode::read_write) ? GENERIC_READ | GENERIC_WRITE : (static_cast<bool>(method & os::OpenMode::read)) ? GENERIC_READ : GENERIC_WRITE,
        static_cast<DWORD>((method >> 4) & static_cast<os::OpenMode>(0b0111)),
        NULL,
        (static_cast<bool>(method & os::OpenMode::read) && static_cast<bool>(method & os::OpenMode::new_file)) ? CREATE_ALWAYS :
         (static_cast<bool>(method & os::OpenMode::read)) ? OPEN_EXISTING : 
         (static_cast<bool>(method & os::OpenMode::write) && static_cast<bool>(method & os::OpenMode::new_file)) ? CREATE_ALWAYS : OPEN_ALWAYS,
        (static_cast<bool>(method & os::OpenMode::write)) ? FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
        NULL
    );
    if(file_handle == INVALID_HANDLE_VALUE){
        switch (GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                throw os::exceptions::FileNotFound();
            case ERROR_PATH_NOT_FOUND:
                throw os::exceptions::PathNotFound();
            case ERROR_SHARING_VIOLATION:
            case ERROR_ACCESS_DENIED:
                throw os::exceptions::FileAccessDenied();
            case ERROR_FILE_EXISTS:
            case ERROR_ALREADY_EXISTS:
                throw os::exceptions::FileAlreadyExists();
            default:
                throw os::exceptions::FileUnknownError();
        }
    }flags = method & os::OpenMode::binary;
}

void FileStream::WindowsStreamBuffer::open(const char* path, os::OpenMode mode){
    return this->open(os::GlobalFunctions::ascii_to_wide(path).data, mode);
}

void FileStream::WindowsStreamBuffer::open(const wchar_t* path, os::OpenMode mode){
    if(file_handle != INVALID_HANDLE_VALUE) return ;
    file_handle = CreateFileW(
        path,
        ((mode & os::OpenMode::read_write) == os::OpenMode::read_write) ? GENERIC_READ | GENERIC_WRITE : (static_cast<bool>(mode & os::OpenMode::read)) ? GENERIC_READ : GENERIC_WRITE,
        static_cast<DWORD>((mode >> 4) & static_cast<os::OpenMode>(0b0111)),
        NULL,
        (static_cast<bool>(mode & os::OpenMode::read) && static_cast<bool>(mode & os::OpenMode::new_file)) ? CREATE_ALWAYS :
         (static_cast<bool>(mode & os::OpenMode::read)) ? OPEN_EXISTING : 
         (static_cast<bool>(mode & os::OpenMode::write) && static_cast<bool>(mode & os::OpenMode::new_file)) ? CREATE_ALWAYS : OPEN_ALWAYS,
        (static_cast<bool>(mode & os::OpenMode::write)) ? FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH : FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
        NULL
    );
    if(file_handle == INVALID_HANDLE_VALUE){
        switch (GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
                throw os::exceptions::FileNotFound();
            case ERROR_PATH_NOT_FOUND:
                throw os::exceptions::PathNotFound();
            case ERROR_SHARING_VIOLATION:
            case ERROR_ACCESS_DENIED:
                throw os::exceptions::FileAccessDenied();
            case ERROR_FILE_EXISTS:
            case ERROR_ALREADY_EXISTS:
                throw os::exceptions::FileAlreadyExists();
            default:
                throw os::exceptions::FileUnknownError();
        }
    }flags = mode & os::OpenMode::binary;
}

void FileStream::WindowsStreamBuffer::close(){
    if(file_handle != INVALID_HANDLE_VALUE) CloseHandle(file_handle);
}

bool FileStream::WindowsStreamBuffer::is_open() const{
    return file_handle != INVALID_HANDLE_VALUE;
}

os::Ifstream::Ifstream(const char* path, os::OpenMode mode) : Ifstream(os::GlobalFunctions::ascii_to_wide(path).data, mode){}

os::Ifstream::Ifstream(const wchar_t* path, os::OpenMode mode) : WindowsStreamBuffer(path, mode){}

bool FileStream::WindowsStreamBuffer::refil() noexcept{
    if(file_handle == INVALID_HANDLE_VALUE){
        current_cursor = end_cursor = buffer;
        return false;
    }
    DWORD read{};
    BOOL stat = ReadFile(
        file_handle,
        buffer,
        buffer_size,
        &read,
        NULL
    );

    if(!stat || read == 0){
        current_cursor = end_cursor = buffer;
        return false;
    }
    current_cursor = buffer;
    end_cursor = buffer + read;
    return true;
}


#endif