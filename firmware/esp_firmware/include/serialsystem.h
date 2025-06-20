#pragma once

#include <SoftwareSerial.h>
#include <cstddef>

namespace SerialSystem
{
enum class LoggingLevel
{
    Info = 0,
    Debug,
    Warning,
    Critical,
};

void initSerialSystem(size_t communicationBaud, size_t loggingBaud,
                      LoggingLevel allowedLevel = LoggingLevel::Info);

extern EspSoftwareSerial::UART ProtocolCommunicator; //for communication with PC

template <typename ShitToPrint>
void logMessageLn(ShitToPrint msg, LoggingLevel level = LoggingLevel::Info);

template <typename ShitToPrint>
void logMessage(ShitToPrint msg, LoggingLevel level = LoggingLevel::Info);

extern template void logMessageLn<const char *>(const char *msg, LoggingLevel level);
extern template void logMessageLn<int>(int msg, LoggingLevel level);
extern template void logMessageLn<const String &>(const String &msg, LoggingLevel level);

extern template void logMessage<const char *>(const char *msg, LoggingLevel level);
extern template void logMessage<int>(int msg, LoggingLevel level);
extern template void logMessage<const String &>(const String &msg, LoggingLevel level);

} // namespace SerialSystem
