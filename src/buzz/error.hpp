#pragma once

#include <exception>
#include <ostream>
#include <sstream>

#ifdef __clang__
    #include <experimental/source_location>
    namespace std
    {
        using experimental::source_location;
    }
#else
    #include <source_location>
#endif

template <class T>
concept Streamable = requires (std::ostream& s, T x) { s << x; };

class Error : public std::exception {
public:
    Error(std::source_location location = std::source_location::current());

    template <Streamable T>
    Error& operator<<(T&& value) &
    {
        updateMessage(std::forward<T>(value));
        return *this;
    }

    template <Streamable T>
    Error&& operator<<(T&& value) &&
    {
        updateMessage(std::forward<T>(value));
        return std::move(*this);
    }

    const char* what() const noexcept override
    {
        return _message.c_str();
    }

private:
    template <Streamable T>
    void updateMessage(T&& value)
    {
        auto stream = std::ostringstream{std::move(_message), std::ios::app};
        stream << std::forward<T>(value);
        _message = std::move(stream).str();
    }

    std::string _message;
};

void check(bool condition, std::source_location location = std::source_location::current());
