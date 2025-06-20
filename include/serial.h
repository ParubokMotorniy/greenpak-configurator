#pragma once

#include "protocol.h"

#include <vector>
#include <string>

using CommandBuffer = std::vector<Protocol::CommandRepresentation>;

bool sendBuffer(const CommandBuffer &buffer, const std::string &port, size_t baudRate);
