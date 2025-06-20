#pragma once

#include <cstdint>

// TODO: make the protocol a shared lib between firmware and cli ...
namespace Protocol
{
using CommandRepresentation = uint16_t;

// for now -> let's reserve 4 most significant bits for the opcode
// the rest 12 - for the payload
enum CommandType : CommandRepresentation
{
    Invalid = 0x0000,
    Color = 0x1000,
    Delay = 0x2000,
    Clear = 0x3000
};

enum FlowControl : uint8_t
{
    ACK = 0x0Fu,
    NACK = 0xF0u,
    FULL = 0xFFu,
};

inline bool payloadIsValid(uint16_t payload, CommandType targetType)
{
    switch (targetType)
    {
    case Invalid:
        return false;
    case Color:
        return payload <= 4095;
    case Delay:
        return payload <= 4095;
    case Clear:
        return payload == 255;
    default:
        return false;
    }
}

inline CommandRepresentation makeColorCommand(uint16_t payload)
{
    return static_cast<CommandRepresentation>(payload) | Color;
}

inline CommandRepresentation makeDelayCommand(uint16_t payload)
{
    return static_cast<CommandRepresentation>(payload) | Delay;
}

inline CommandRepresentation makeClearCommand() { return 0x00FFu | Clear; }

inline uint8_t extractPayload(CommandRepresentation command) { return 0x0FFFu & command; }

inline CommandType determineCommand(CommandRepresentation command)
{
    constexpr CommandRepresentation opCodemask = 0xF000;
    constexpr uint16_t payloadMask = 0x0FFF;

    const CommandRepresentation opCode = command & opCodemask;

    if (opCode > Clear)
        return Invalid;

    const auto estimatedType = static_cast<CommandType>(opCode);

    return payloadIsValid(command & payloadMask, estimatedType) ? estimatedType : Invalid;
}
} // namespace Protocol
