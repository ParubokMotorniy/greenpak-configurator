#include "scriptprocessing.h"

#include <vector>
#include <charconv>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string_view>

using namespace std::literals;

bool parseCommandFile(const std::filesystem::path &filePath, CommandBuffer &destination)
{
    using CommandMatcher = std::function<Protocol::CommandRepresentation(std::string_view)>;
    static std::vector<CommandMatcher> matchers{[](std::string_view line) -> Protocol::CommandRepresentation
                                                {
                                                    // 'number' parser
                                                    constexpr auto prefix = "color:"sv;
                                                    if (std::string_view(line.data(), std::min(line.size(), prefix.size())) != prefix)
                                                        return 0;

                                                    std::string_view colorCodeStr(std::find(line.cbegin(), line.cend(), ':') + 1, line.cend());

                                                    if (colorCodeStr.empty())
                                                        return 0;

                                                    int code{-1}; // for now - only assume binary color codes
                                                    std::from_chars(colorCodeStr.data(), colorCodeStr.data() + colorCodeStr.size(), code, 2);

                                                    std::cout << "Color code: " << code << std::endl;

                                                    if(!Protocol::payloadIsValid(code, Protocol::Color))
                                                        return 0;

                                                    return Protocol::makeColorCommand(code);
                                                },
                                                [](std::string_view line) -> Protocol::CommandRepresentation
                                                {
                                                    // 'delay' parser
                                                    constexpr auto prefix = "delay:"sv;
                                                    if (std::string_view(line.data(), std::min(line.size(), prefix.size())) != prefix)
                                                        return 0;

                                                    std::string_view delayStr(std::find(line.cbegin(), line.cend(), ':') + 1, line.cend());

                                                    if (delayStr.empty())
                                                        return 0;

                                                    int delay{-1}; // up to 255 ms
                                                    std::from_chars(delayStr.data(), delayStr.data() + delayStr.size(), delay, 10);

                                                    std::cout << "Delay: " << delay << std::endl;

                                                    if(!Protocol::payloadIsValid(delay, Protocol::Delay))
                                                        return 0;

                                                    return Protocol::makeDelayCommand(delay);
                                                },
                                                [](std::string_view line) -> Protocol::CommandRepresentation
                                                {
                                                    // 'clear' parser
                                                    constexpr auto command = "clear"sv;
                                                    std::cout << "Clear" << std::endl;
                                                    return line != command ? 0 : Protocol::makeClearCommand();
                                                }};

    std::ifstream commandFile;

    if (commandFile.open(filePath); !commandFile.is_open())
        return false;

    std::stringstream commandSS;
    commandSS << commandFile.rdbuf();

    //TODO: resize the buffer in advance
    destination.clear();
    for (std::string line; std::getline(commandSS, line, '\n');)
    {
        if (line.empty() || line[0] == '#')
            continue;

        Protocol::CommandRepresentation com = 0;
        for (auto &matcher : matchers)
        {
            if (com = matcher(line); com != '\0')
            {
                break;
            }
        }

        if (com == 0){
            std::cerr << "Invalid command detected!" << std::endl;
            return false;
        }

        destination.emplace_back(com);
    }

    return true;
}
