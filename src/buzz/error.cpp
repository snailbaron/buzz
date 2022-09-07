#include "error.hpp"

#include "build-info.hpp"

#include <filesystem>

namespace {

class FormatLocation {
public:
    FormatLocation(std::source_location location)
        : _location(location)
    { }

    friend std::ostream& operator<<(
        std::ostream& output, const FormatLocation& fl)
    {
        auto readablePath =
            std::filesystem::proximate(fl._location.file_name(), SOURCE_ROOT);
        return output << readablePath.string() << ":" <<
            fl._location.line() << ":" << fl._location.column() << ":" <<
            " \"" << fl._location.function_name() << "\": ";
    }

private:
    std::source_location _location;
};

std::string locationString(std::source_location location)
{
    auto stream = std::ostringstream{};
    stream << FormatLocation{location};
    return stream.str();
}

} // namespace

Error::Error(std::source_location location)
    : _message(locationString(location))
{ }

void check(bool condition, std::source_location location)
{
    if (!condition) {
        throw Error{std::move(location)};
    }
}
