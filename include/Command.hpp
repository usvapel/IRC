#include <string>
#include <vector>

struct Command {
    std::string              command;
    std::string              prefix;
    std::vector<std::string> params;
};
