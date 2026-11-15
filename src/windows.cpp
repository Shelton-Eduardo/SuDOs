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
    current_cursor = end_cursor = buffer;
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
        (method == os::OpenMode::read_write) ? GENERIC_READ | GENERIC_WRITE : (method == os::OpenMode::read) ? GENERIC_READ : GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        (method & os::OpenMode::write) ?  :,
        [in]           DWORD                 dwFlagsAndAttributes,
        [in, optional] HANDLE                hTemplateFile
    );
}

#endif