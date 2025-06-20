#include "serialsystem.h"

namespace SerialSystem
{

static EspSoftwareSerial::UART logger = EspSoftwareSerial::UART(-1, 0);
EspSoftwareSerial::UART ProtocolCommunicator = EspSoftwareSerial::UART(3, 4);
static LoggingLevel globalPermittedlevel;

void initSerialSystem(size_t communicationBaud, size_t loggingBaud, LoggingLevel allowedLevel)
{
    globalPermittedlevel = allowedLevel;

    logger.begin(loggingBaud, EspSoftwareSerial::SWSERIAL_8N1, -1, 0);
    while (!logger)
    {
    }
    logger.println("[+] Debug serial initialized!");

    ProtocolCommunicator.begin(communicationBaud, EspSoftwareSerial::SWSERIAL_8N1, 3, 4);
    while (!ProtocolCommunicator)
    {
    }
    ProtocolCommunicator.println("[+] Protocol serial initialized!");
    ProtocolCommunicator.setTimeout(2000);
}

template void logMessageLn<const char *>(const char *msg, LoggingLevel level);
template void logMessageLn<int>(int msg, LoggingLevel level);
template void logMessageLn<const String &>(const String &msg, LoggingLevel level);

template <typename ShitToPrint>
void logMessageLn(ShitToPrint msg, LoggingLevel level)
{
    if (level < globalPermittedlevel)
        return;

    vPortEnterCritical();

    logger.println(msg);

    vPortExitCritical();
}

template void logMessage<const char *>(const char *msg, LoggingLevel level);
template void logMessage<int>(int msg, LoggingLevel level);
template void logMessage<const String &>(const String &msg, LoggingLevel level);

template <typename ShitToPrint>
void logMessage(ShitToPrint msg, LoggingLevel level)
{
    if (level < globalPermittedlevel)
        return;

    vPortEnterCritical();

    logger.print(msg);

    vPortExitCritical();
}

} // namespace SerialSystem
