#include "i2ccommunication.h"
#include "serialsystem.h"

#include <Wire.h>

namespace I2cSystem
{

// TODO: check if NVM is actually programmed here - if so, switch to paged write operation
bool writeBytes(uint8_t *data, size_t count, OperationDescriptor opDescriptor)
{
    Wire.beginTransmission(opDescriptor.buildControlByte(OperationType::Write) >> 1);

    Wire.write(opDescriptor.buildWordAddress());
    Wire.write(data, count);

    return Wire.endTransmission(true) == 0;
}

bool writeByte(uint8_t data, OperationDescriptor opDescriptor)
{
    Wire.beginTransmission(opDescriptor.buildControlByte(OperationType::Write) >> 1);

    Wire.write(opDescriptor.buildWordAddress());
    Wire.write(data);

    return Wire.endTransmission(true) == 0;
}

bool readBytes(std::vector<uint8_t> &target, size_t numberToRead, OperationDescriptor opDescriptor)
{
    if (numberToRead == 0)
        return true;

    target.resize(numberToRead);

    Wire.beginTransmission(opDescriptor.buildControlByte(OperationType::Write) >> 1);

    Wire.write(opDescriptor.buildWordAddress());

    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (const auto count = Wire.requestFrom(opDescriptor.buildControlByte(OperationType::Read) >> 1,
                                            numberToRead);
        count < numberToRead)
    {
        return false;
    }

    for (size_t q = 0; q < numberToRead; ++q)
    {
        target[q] = Wire.read();
    }

    return true;
}

bool readByte(uint8_t *target, OperationDescriptor opDescriptor)
{
    Wire.beginTransmission(opDescriptor.buildControlByte(OperationType::Write) >> 1);

    Wire.write(opDescriptor.buildWordAddress());

    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }

    if (Wire.requestFrom(opDescriptor.buildControlByte(OperationType::Read) >> 1, 1) == 0)
    {
        return false;
    }

    *target = Wire.read();
    return true;
}

void initI2cSystem(size_t clockSpeed)
{
    Wire.begin(8, 9, clockSpeed);
    Wire.setClock(clockSpeed);
    Wire.setTimeout(3000);
}
} // namespace I2cSystem
