#pragma once

#include "serial.h"

#include <functional>
#include <string_view>
#include <filesystem>

using CommandMatcher = std::function<Protocol::CommandRepresentation(std::string_view)>;

bool parseCommandFile(const std::filesystem::path &filePath, CommandBuffer &destination);
