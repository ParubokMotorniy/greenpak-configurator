#include "commandreception.h"
#include "commanddispatchers.h"
#include "protocol.h"
#include "serialsystem.h"

#include <Arduino.h>

#define RECEIVE_QUEUE_SIZE 30

static uint8_t receiveQueueStorage[RECEIVE_QUEUE_SIZE * sizeof(Protocol::CommandRepresentation)];
static StaticQueue_t receiveQueue;
static QueueHandle_t receiveQueueHandle;

void masterCommunicator(void *pvParameters)
{
    constexpr uint16_t receptionLoopDelay = 5;
    constexpr uint8_t commandLength = sizeof(Protocol::CommandRepresentation);
    while (true)
    {
        Protocol::CommandRepresentation commandBuffer{ 0 };
        while (SerialSystem::ProtocolCommunicator.available() < commandLength
               || SerialSystem::ProtocolCommunicator.readBytes(reinterpret_cast<uint8_t *>(
                                                                   &commandBuffer),
                                                               commandLength)
                      != commandLength)
        {
            vTaskDelay(receptionLoopDelay / portTICK_PERIOD_MS);
        } // reads next command

        if (Protocol::determineCommand(commandBuffer) == Protocol::CommandType::Invalid)
        {
            SerialSystem::logMessageLn("Invalid command received from master!",
                                       SerialSystem::LoggingLevel::Debug);
            while (SerialSystem::ProtocolCommunicator.write(Protocol::FlowControl::NACK) == 0)
            {
            }
            continue;
        }

        SerialSystem::logMessageLn<const String &>(String("Successfully received a ")
                                                       + String(commandBuffer)
                                                       + String(" command from master!"),
                                                   SerialSystem::LoggingLevel::Debug);

        while (xQueueSend(receiveQueueHandle, &commandBuffer, 0) == errQUEUE_FULL)
        {
            while (SerialSystem::ProtocolCommunicator.write(Protocol::FlowControl::FULL) == 0)
            {
            }
            vTaskDelay(receptionLoopDelay / portTICK_PERIOD_MS);
        } // sends the command for processing

        while (SerialSystem::ProtocolCommunicator.write(Protocol::FlowControl::ACK) == 0)
        {
        } // sends ack -> communicator is ready for more data
    }
}

void slaveCommunicator(void *pvParameters)
{
    constexpr uint16_t postLoopDelay = 5;
    while (true)
    {
        Protocol::CommandRepresentation nextCommand{ 0 };
        while (xQueueReceive(receiveQueueHandle, &nextCommand, 0) == errQUEUE_EMPTY)
        {
            vTaskDelay(postLoopDelay / portTICK_PERIOD_MS);
        }

        SerialSystem::logMessageLn("Posting a command to dispatch queue!",
                                   SerialSystem::LoggingLevel::Debug);

        DispatchSystem::postCommandToDispatchQueue(nextCommand);
    }
}

bool CommandReception::initCommandReceptionSystem()
{
    receiveQueueHandle = xQueueCreateStatic(RECEIVE_QUEUE_SIZE,
                                            sizeof(Protocol::CommandRepresentation),
                                            receiveQueueStorage, &receiveQueue);

    if (receiveQueueHandle == nullptr)
        return false;

    const auto masterTaskCreationRes = xTaskCreate(masterCommunicator, "pc-to-proc", 1024, nullptr,
                                                   1, nullptr);

    const auto slaveTaskCreationRes = xTaskCreate(slaveCommunicator, "proc-to-chip", 1024, nullptr,
                                                  1, nullptr);

    return slaveTaskCreationRes == pdPASS && masterTaskCreationRes == pdPASS;
}
