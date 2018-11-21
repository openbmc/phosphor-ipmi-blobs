#include "fs.hpp"

#include <experimental/filesystem>
#include <string>
#include <vector>

namespace blobs
{
namespace fs = std::experimental::filesystem;

std::vector<std::string> getLibraryList(const std::string& path,
                                        PathMatcher check)
{
    std::vector<std::string> output;

    for (const auto& p : fs::recursive_directory_iterator(path))
    {
        auto ps = p.path().string();

        if (check(ps))
        {
            output.push_back(ps);
        }
    }

    /* Algorithm ends up being two-pass, but expectation is a list under 10. */
    return output;
}

} // namespace blobs
