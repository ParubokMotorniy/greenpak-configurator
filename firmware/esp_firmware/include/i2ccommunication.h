#pragma once

#include <cstdint>
#include <vector>

namespace I2cSystem
{

enum OperationType : uint8_t
{
    Write = 0x00u,
    Read = 0x01u,
};

enum MemorySpaceType : uint8_t
{
    RegConfig,
    NVMConfig,
    EEPROM,
};

struct OperationDescriptor
{
    uint8_t slaveAddress{ 0 }; //: 4 = 0;
    uint16_t byteAddress{ 0 }; //: 11 = 0;
    MemorySpaceType memType = RegConfig;

    uint8_t buildControlByte(OperationType opType)
    {
        uint8_t blockAddress{ 0 };
        switch (memType)
        {
        case RegConfig:
            blockAddress = (byteAddress >> 9) & 1;
            break;
        case NVMConfig:
            blockAddress = 0b010;
            break;
        case EEPROM:
            blockAddress = 0b011;
            break;
        default:
            break;
        }

        return (slaveAddress << 4) | (blockAddress << 1) | opType;
    }

    uint8_t buildWordAddress() { return static_cast<uint8_t>(byteAddress); }
};

bool writeBytes(uint8_t *data, size_t count, OperationDescriptor opDescriptor);
bool writeByte(uint8_t data, OperationDescriptor opDescriptor);

bool readBytes(std::vector<uint8_t> &target, size_t numberToRead, OperationDescriptor opDescriptor);
bool readByte(uint8_t *target, OperationDescriptor opDescriptor);

void initI2cSystem(size_t clockSpeed);
} // namespace I2cSystem
