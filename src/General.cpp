#include "os.hpp"

os::OpenMode operator|(os::OpenMode lhs, os::OpenMode rhs){
    return static_cast<os::OpenMode>(static_cast<std::size_t>(lhs)|static_cast<std::size_t>(rhs));
}

os::OpenMode operator&(os::OpenMode lhs, os::OpenMode rhs){
    return static_cast<os::OpenMode>(static_cast<std::size_t>(lhs)&static_cast<std::size_t>(rhs));
}

os::Cursor operator+(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)+static_cast<std::size_t>(rhs));
}

os::Cursor operator-(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)-static_cast<std::size_t>(rhs));
}

StreamBuffer::StreamBuffer(){
    buffer = static_cast<unsigned char*>(::operator new(buffer_size));
    current_cursor = end_cursor = buffer;
    offset = static_cast<os::Cursor>(0);
}

void StreamBuffer::seek(os::Cursor position){
    if(position >= offset && position <= offset+static_cast<os::Cursor>(buffer_size))
}


os::Wchar_t::Wchar_t(std::size_t len){
    data = static_cast<wchar_t*>(::operator new(sizeof(wchar_t)*len));
}

os::Wchar_t::~Wchar_t() noexcept{
    ::operator delete(data);
}

os::Char::Char(std::size_t len){
    data = static_cast<char*>(::operator new(sizeof(char)*len));
}

os::Char::~Char() noexcept{
    ::operator delete(data);
}

os::Wchar_t os::GlobalFunctions::ascii_to_wide(const char* ascii_str){
    if(!ascii_str) return Wchar_t{};
    std::size_t len{};
    while(ascii_str[len] != '\0'){
        if(ascii_str[len] < 32 || ascii_str[len] >= 127) throw os::exceptions::PathInvalid{};
        if(len == PATH_MAX_SIZE) throw os::exceptions::PathTooLong{};
        ++len;
    }
    os::Wchar_t path{len+1};
    for(std::size_t i{}; i<len; ++i) path.data[i] = static_cast<wchar_t>(ascii_str[i]);
    path.data[len] = L'\0';
    return path;
}

os::Char os::GlobalFunctions::wide_to_ascii(const wchar_t* wide_str){
    if(!wide_str) return os::Char{};
    std::size_t len{};
    while(wide_str != L'\0'){
        if(wide_str[len] < 32 || wide_str[len] >= 127) throw os::exceptions::PathInvalid{};
        if(len == PATH_MAX_SIZE) throw os::exceptions::PathTooLong{};
        ++len;
    }
    os::Char path{len+1};
    for(std::size_t i{}; i<len; ++i) path.data[i] = static_cast<char>(wide_str[i]);
    return path;
}

int main(){
    return 0;
}