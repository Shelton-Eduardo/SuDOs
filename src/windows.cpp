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
    end_cursor = current_cursor = buffer;
    offset = os::Cursor::npos;
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
    file_handle = INVALID_HANDLE_VALUE;
    open(file, method);
}

void FileStream::WindowsStreamBuffer::open(const char* path, os::OpenMode mode){
    return this->open(os::GlobalFunctions::ascii_to_wide(path).data, mode);
}

void FileStream::WindowsStreamBuffer::open(const wchar_t* path, os::OpenMode mode){
    if(file_handle != INVALID_HANDLE_VALUE) return ;
         file_handle = CreateFileW(
            path,
            ((mode & os::OpenMode::read_write) == os::OpenMode::read_write || static_cast<bool>(mode & os::OpenMode::append))
                ? (GENERIC_READ | GENERIC_WRITE) :
            static_cast<bool>(mode & os::OpenMode::read)
                ? GENERIC_READ :
            static_cast<bool>(mode & os::OpenMode::write)
                ? GENERIC_WRITE : 0,
            static_cast<DWORD>((mode >> 4) & static_cast<os::OpenMode>(0b0111)) | FILE_SHARE_READ,
            NULL,
            static_cast<bool>(mode & os::OpenMode::new_file) ? CREATE_ALWAYS :
            static_cast<bool>(mode & (os::OpenMode::write | os::OpenMode::append)) ? OPEN_ALWAYS : OPEN_EXISTING,
            static_cast<bool>(mode & (os::OpenMode::write | os::OpenMode::append))
                ? (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH)
                : (FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING),
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
    }
    
    if( static_cast<bool>(mode & os::OpenMode::write) && static_cast<bool>(mode & os::OpenMode::append)){
        LARGE_INTEGER file_size{};
        LARGE_INTEGER setoffset{};
        if(!GetFileSizeEx(file_handle, &file_size)) throw os::exceptions::FileReadError();
        setoffset.QuadPart = file_size.QuadPart - (file_size.QuadPart % buffer_size);
        if(!SetFilePointerEx(file_handle, setoffset, NULL, FILE_BEGIN)) throw os::exceptions::FileSeekError();
        if((file_size.QuadPart % buffer_size) > 0) refil(false);
        offset = static_cast<os::Cursor>(file_size.QuadPart / buffer_size);
        current_cursor = buffer + (file_size.QuadPart % buffer_size);
        end_cursor = buffer + buffer_size;
        if(!SetFilePointerEx(file_handle, setoffset, NULL, FILE_BEGIN)) throw os::exceptions::FileSeekError();
    }

    flags = mode;
}

void FileStream::WindowsStreamBuffer::close(){
    if(file_handle != INVALID_HANDLE_VALUE) CloseHandle(file_handle);
}

bool FileStream::WindowsStreamBuffer::is_open() const{
    return file_handle != INVALID_HANDLE_VALUE;
}

os::Ifstream::Ifstream(const char* path, os::OpenMode mode) : Ifstream(os::GlobalFunctions::ascii_to_wide(path).data, mode){}

os::Ifstream::Ifstream(const wchar_t* path, os::OpenMode mode) : WindowsStreamBuffer(path, mode){}

bool FileStream::WindowsStreamBuffer::refil(bool update) noexcept{
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

    if(!stat || (read == 0 && !stat)){
        current_cursor = end_cursor = buffer;
        DWORD err = stat ? 0 : GetLastError(); // Debugging Purposes, to remove later on
        return false;
    }
    if(update) offset = offset + static_cast<os::Cursor>(1);
    current_cursor = buffer;
    end_cursor = buffer + read;
    return true;
}

FileStream::WindowsStreamBuffer::~WindowsStreamBuffer(){
    close();
}

void os::Ifstream::seek(os::Cursor position){
    LARGE_INTEGER aligned_pos{};
        aligned_pos.QuadPart = static_cast<LONGLONG>(
            (position / static_cast<os::Cursor>(buffer_size)) * static_cast<os::Cursor>(buffer_size));
    SetFilePointerEx(
        file_handle,
        aligned_pos,
        NULL,
        FILE_BEGIN
        );
    if(!refil(false)) throw os::exceptions::FileSeekError{};
    if((buffer + (static_cast<std::size_t>(position) % buffer_size)) >= end_cursor) throw os::exceptions::FileSeekError{};
    current_cursor = buffer + (static_cast<std::size_t>(position) % buffer_size);
    offset = position / static_cast<os::Cursor>(buffer_size);
}

os::Ofstream::Ofstream(const char* path, os::OpenMode mode) : Ofstream(os::GlobalFunctions::ascii_to_wide(path).data, mode) {}

os::Ofstream::Ofstream(const wchar_t* path, os::OpenMode mode) : WindowsStreamBuffer(path, mode){
    if(offset == os::Cursor::npos){
        offset = os::Cursor::start;
        end_cursor = buffer + buffer_size;
    }
}

void os::Ofstream::seekp(os::Cursor pos_off) noexcept{
    LARGE_INTEGER aligned_pos{};
        aligned_pos.QuadPart = static_cast<LONGLONG>(
            (pos_off * static_cast<os::Cursor>(buffer_size))
        );
    SetFilePointerEx(
        file_handle,
        aligned_pos,
        NULL,
        FILE_BEGIN
    );
}

void os::Ofstream::flush() noexcept{
    if(file_handle == INVALID_HANDLE_VALUE) return ;
    this->seekp(offset); // just in case the filepointer is not at current offset
    DWORD written{};
    // while(current_cursor < end_cursor) *(current_cursor++) = '\0'; // overiding the leftover bytes
    BOOL state = WriteFile(
        file_handle,
        buffer,
        buffer_size,
        &written,
        NULL
    );
    if(!state || written == 0) return ;
    offset = offset + static_cast<os::Cursor>(1ULL);
    current_cursor = buffer;
    end_cursor = buffer + buffer_size;
}

void os::Ofstream::seek(os::Cursor position){
    flush();
    LARGE_INTEGER aligned_pos{};
        aligned_pos.QuadPart = static_cast<LONGLONG>(
            (position / static_cast<os::Cursor>(buffer_size)) * static_cast<os::Cursor>(buffer_size)
        );
    BOOL state = SetFilePointerEx(
        file_handle,
        aligned_pos,
        NULL,
        FILE_BEGIN
    );
    if(!refil(false)) throw os::exceptions::FileSeekError{};
    current_cursor = buffer + (static_cast<std::size_t>(position) % (buffer_size));
    offset = position / static_cast<os::Cursor>(buffer_size);
    end_cursor = buffer + buffer_size;
}

void os::Ofstream::close(){
    if(file_handle == INVALID_HANDLE_VALUE) return ;
    std::size_t total_size = (static_cast<std::size_t>(offset) * buffer_size) + (current_cursor - buffer);
    flush();
    FILE_END_OF_FILE_INFO wflag{};
    wflag.EndOfFile.QuadPart = static_cast<LONGLONG>(total_size);
    SetFileInformationByHandle(file_handle, FileEndOfFileInfo, &wflag, sizeof(wflag));
    CloseHandle(file_handle);
    file_handle = INVALID_HANDLE_VALUE;
    current_cursor = end_cursor = buffer;
    offset = os::Cursor::npos;
}

os::Ofstream::~Ofstream(){
    close();
}

#endif