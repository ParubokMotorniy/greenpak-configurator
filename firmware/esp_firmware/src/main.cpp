#include <Arduino.h>

#include "commanddispatchers.h"
#include "commandreception.h"
#include "i2ccommunication.h"
#include "projectpatching.h"
#include "protocol.h"
#include "serialsystem.h"

static constexpr uint8_t slaveAddress = 0b1010;
static constexpr size_t i2cClockSpeed = 300'000u;
static constexpr size_t protocolBaud = 57600;
static constexpr size_t loggingBaud = 57600;
static ProjectPatching::RGBPatchFactory currentPatchFactory{ 255 };

void setup()
{
    SerialSystem::initSerialSystem(protocolBaud, loggingBaud, SerialSystem::LoggingLevel::Debug);

    I2cSystem::initI2cSystem(i2cClockSpeed);

    if (!DispatchSystem::initDispatchSystem(&currentPatchFactory, slaveAddress)
        || !CommandReception::initCommandReceptionSystem())
    {
        SerialSystem::logMessageLn("[-] Initialization failed!",
                                   SerialSystem::LoggingLevel::Critical);
        exit(-1);
    }

    SerialSystem::logMessageLn("[+] Initialization succeeded!", SerialSystem::LoggingLevel::Critical);
}

void loop() {}
