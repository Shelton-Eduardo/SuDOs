#include "os.hpp"

#ifndef MAX_GET_RANGE
    #define MAX_GET_RANGE 30
#endif

os::OpenMode operator|(os::OpenMode lhs, os::OpenMode rhs){
    return static_cast<os::OpenMode>(static_cast<std::size_t>(lhs)|static_cast<std::size_t>(rhs));
}

os::OpenMode operator&(os::OpenMode lhs, os::OpenMode rhs){
    return static_cast<os::OpenMode>(static_cast<std::size_t>(lhs)&static_cast<std::size_t>(rhs));
}

os::OpenMode operator>>(os::OpenMode lhs, int bitshift) noexcept{
    return static_cast<os::OpenMode>(static_cast<std::uint8_t>(lhs) >> bitshift);
}

os::Cursor operator+(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)+static_cast<std::size_t>(rhs));
}

os::Cursor operator-(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)-static_cast<std::size_t>(rhs));
}

os::Cursor operator+(os::Cursor lhs, int rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)-static_cast<std::size_t>(rhs));
}

os::Cursor operator-(os::Cursor lhs, int rhs){
    if (lhs == os::Cursor::start) return static_cast<os::Cursor>(0);
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs) - static_cast<std::size_t>(rhs));
}

os::Cursor operator/(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)/static_cast<std::size_t>(rhs));
}

os::Cursor operator*(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs)*static_cast<std::size_t>(rhs));
}

os::Cursor operator%(os::Cursor lhs, os::Cursor rhs){
    return static_cast<os::Cursor>(static_cast<std::size_t>(lhs) % static_cast<std::size_t>(rhs));
}

os::Wchar_t::Wchar_t(std::size_t len){
    data = static_cast<wchar_t*>(::operator new(sizeof(wchar_t)*len));
}

os::Wchar_t::~Wchar_t() noexcept{
    ::operator delete(data);
}

os::Wchar_t::Wchar_t(Wchar_t&& source) noexcept : data(source.data){
    source.data = nullptr;
}

os::Wchar_t& os::Wchar_t::operator=(Wchar_t&& source) noexcept{
    if(this == &source) return *this;
    data = source.data;
    source.data = nullptr;
    return *this;
}

os::Char::Char(std::size_t len){
    data = static_cast<char*>(::operator new(sizeof(char)*len));
}

os::Char::~Char() noexcept{
    ::operator delete(data);
}

os::Char::Char(Char&& source) noexcept : data(source.data){
    source.data = nullptr;
}

os::Char& os::Char::operator=(Char&& source) noexcept{
    if(this == &source) return *this;
    data = source.data;
    source.data = nullptr;
    return *this;
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
    while(wide_str[len] != L'\0'){
        if(wide_str[len] < 32 || wide_str[len] >= 127) throw os::exceptions::PathInvalid{};
        if(len == PATH_MAX_SIZE) throw os::exceptions::PathTooLong{};
        ++len;
    }
    os::Char path{len+1};
    for(std::size_t i{}; i<len; ++i) path.data[i] = static_cast<char>(wide_str[i]);
    path.data[len] = '\0';
    return path;
}

std::size_t os::Ifstream::read(unsigned char* container, std::size_t bytes) noexcept{
    if(!is_open() || bytes == 0) return 0;
    std::size_t bytes_read{};
    if(static_cast<bool>(flags & os::OpenMode::binary)){
        while(bytes_read < bytes){
            if(current_cursor == end_cursor) if(!refil()) break;
            container[bytes_read++] = *current_cursor++;
            }
    }else{
        while(bytes_read < bytes){
            if(current_cursor == end_cursor) if(!refil()) break;
            if(*current_cursor == '\r'){
                if((current_cursor+1) == end_cursor){
                    if(!refil()){
                        container[bytes_read++] = '\n';
                        break;
                    }if(*(current_cursor+1) == '\n'){
                        container[bytes_read++] = '\n';
                        current_cursor+=2;
                        continue;
                    }container[bytes_read++] = *current_cursor++;
                    continue;
                }
            }
            container[bytes_read++] = *current_cursor++;
        }
    }
    return bytes_read;
}

unsigned char os::Ifstream::get(){
    if(!is_open()) throw os::exceptions::FileNotOpenError();
    if(static_cast<bool>(flags & os::OpenMode::binary)){
        return *current_cursor;
    }
    for(std::size_t i{}; i < MAX_GET_RANGE; ++i){
        if((current_cursor+i) >= end_cursor) return '\n';
        if(*(current_cursor+i) == '\r' || *(current_cursor+i) == '\t') continue;
        if(*(current_cursor+i) == '\n') return '\n'; 
    }return '\n';
}

os::Ofstream& os::Ofstream::write(const unsigned char* container, std::size_t bytes) noexcept{
    #if !defined(__linux__)
        if (file_handle == INVALID_HANDLE_VALUE) return *this;
    #else
        if (file_handle == -1) return *this;
    #endif

    auto write_byte = [&](unsigned char C){
            if(current_cursor == end_cursor) flush();
            *(current_cursor++) = C;
        };

    if (static_cast<bool>(flags & os::OpenMode::binary)){
        for(std::size_t i{}; i < bytes; ++i) write_byte(container[i]);
    }else{
        std::size_t i{};
        while(i < bytes){
            switch (container[i]) {
                case '\n':
                    #if !defined(__linux__)
                        write_byte('\r');
                    #endif
                    write_byte('\n');
                    ++i;
                    break;
                case '\r':
                    if (i + 1 < bytes && container[i + 1] == '\n') {
                        write_byte('\r');
                        write_byte('\n');
                        i+=2;
                    } else{
                        write_byte('\r');
                    }
                    break;
                default:
                    write_byte(container[i++]);
                    break;
            }
        }
        write_byte('\n');
    }
    return *this;
}

os::Ofstream& os::Ofstream::write(const char* container, std::size_t bytes) noexcept{
    return write(reinterpret_cast<const unsigned char*>(container), bytes);
}

os::Ofstream& os::Ofstream::operator<<(const unsigned char* buffer){
    std::size_t len{};
    while(buffer[len] != '\0' && len <= WRITE_MAX_SIZE) ++len;
    return this->write(buffer, len);    
}

os::Ofstream& os::Ofstream::operator<<(const char* buffer){
    std::size_t len{};
    while(buffer[len] != '\0' && len <= WRITE_MAX_SIZE) ++len;
    return this->write(reinterpret_cast<const unsigned char*>(buffer), len);
}

#define OPERATOR_OVERLOAD_FUNC_DEFINITIONS(type)                                            \
    os::Ofstream& os::Ofstream::operator<<(type){                                           \
        return this->write(reinterpret_cast<const unsigned char*>(&buffer), sizeof(type));  \
    }

OPERATOR_OVERLOAD_FUNC_DEFINITIONS(long double)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(long long)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(double)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(short)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(float)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(long)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(int)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(unsigned int)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(unsigned long)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(unsigned short)
OPERATOR_OVERLOAD_FUNC_DEFINITIONS(unsigned long long)

#undef OPERATOR_OVERLOAD_FUNC_DEFINITIONS