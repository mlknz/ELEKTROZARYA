#pragma once

#include <iostream>
#include <sstream>
#include <assert.h>

// https://stackoverflow.com/questions/19415845/a-better-log-macro-using-template-metaprogramming
#define EZLOG(...) LogWrapper(__FILE__, __LINE__, __VA_ARGS__)

#define EZASSERT(expr, ...) \
if (!expr) \
{\
    EZLOG("ASSERT:", __VA_ARGS__); \
    assert(false); \
}\
static_assert(true, "") // for requiring semicolong after EZASSERT

// Log_Recursive wrapper that creates the ostringstream
template<typename... Args>
void LogWrapper(const char* file, int line, const Args&... args)
{
    std::ostringstream msg;
    Log_Recursive(file, line, msg, args...);
}

// "Recursive" variadic function
template<typename T, typename... Args>
void Log_Recursive(const char* file, int line, std::ostringstream& msg,
                   T value, const Args&... args)
{
    msg << value << " ";
    Log_Recursive(file, line, msg, args...);
}

// Terminator
inline void Log_Recursive(const char* file, int line, std::ostringstream& msg)
{
    std::cout << file << "(" << line << "): " << msg.str() << std::endl;
}
