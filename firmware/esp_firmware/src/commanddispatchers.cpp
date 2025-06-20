#include "commanddispatchers.h"
#include "i2ccommunication.h"
#include "serialsystem.h"

#include <algorithm>
#include <memory>

namespace DispatchSystem
{

class CommandPayload
{
public:
    virtual ~CommandPayload() = default;
};

class ColorCommandPayload : public CommandPayload
{
public:
    ~ColorCommandPayload() override = default;
    ColorCommandPayload(std::vector<ProjectPatching::ProjectPatch> &&patches)
        : _payloadPatches(patches)
    {
    }
    const std::vector<ProjectPatching::ProjectPatch> &payloadPatches() { return _payloadPatches; }

private:
    std::vector<ProjectPatching::ProjectPatch> _payloadPatches;
};

class DelayCommandPayload : public CommandPayload
{
public:
    ~DelayCommandPayload() override = default;
    DelayCommandPayload(uint16_t delay) : _delay(delay) {}
    uint16_t delay() { return _delay; }

private:
    uint16_t _delay;
};

// TODO: forcing each command, however small, to dynamically allocate memory is overkill. Consider
// other options of providing uniform interface while allowing arbitrary data to be embedded
struct PackagedCommand
{
    Protocol::CommandType _commandType; // for proper casts in another thread
    std::unique_ptr<CommandPayload> _payload;
};

#define DISPATCH_QUEUE_SIZE 50
static uint8_t dispatchQueueStorage[DISPATCH_QUEUE_SIZE * sizeof(PackagedCommand)];
static StaticQueue_t dispatchQueue;
static QueueHandle_t dispatchQueueHandle;
static ProjectPatching::PatchFactory *patchFactory;
static uint8_t targetSlaveAddress;

void patchChip(const std::vector<ProjectPatching::ProjectPatch> &patchesToApply)
{
    constexpr uint16_t pageSize = 16;

    const auto &[minPatch, maxPatch]
        = std::minmax_element(patchesToApply.begin(), patchesToApply.end(),
                              [](const ProjectPatching::ProjectPatch &p1,
                                 const ProjectPatching::ProjectPatch &p2) {
                                  return p1.patchStartBitAddress < p2.patchStartBitAddress;
                              });

    // logic below aligns the data operations to 16-byte page boundaries
    // inclusive
    const uint16_t startByteAddress = ((minPatch->patchStartBitAddress / 8 /*byte address*/)
                                       / pageSize /*page floor*/)
                                      * pageSize /*back to page-aligned byte address*/;
    // exclusive
    const uint16_t endByteAddress = ((((maxPatch->patchStartBitAddress + maxPatch->patch.size())
                                       / 8 /*byte address*/)
                                      / pageSize)
                                     + 1 /*page ceiling*/)
                                    * pageSize /*back to page-aligned byte address*/;
    const uint16_t dataSize = endByteAddress - startByteAddress;

    std::vector<uint8_t> chipData;
    if (!I2cSystem::readBytes(chipData, dataSize,
                              I2cSystem::OperationDescriptor{ .slaveAddress = targetSlaveAddress,
                                                              .byteAddress = startByteAddress }))
    {
        SerialSystem::logMessageLn<const String &>(String("[~] Failed to read #") + String(dataSize)
                                                       + String(" bytes from slave #")
                                                       + String(targetSlaveAddress, 2)
                                                       + String(" starting at 0x")
                                                       + String(startByteAddress, 16),
                                                   SerialSystem::LoggingLevel::Critical);
        return;
    }

    SerialSystem::logMessageLn<const String &>(String("[~] Read #") + String(dataSize)
                                                   + String(" bytes from slave #")
                                                   + String(targetSlaveAddress, 2)
                                                   + String(" starting at 0x")
                                                   + String(startByteAddress, 16),
                                               SerialSystem::LoggingLevel::Warning);

    // applies individual patches
    std::for_each(patchesToApply.begin(), patchesToApply.end(),
                  [&chipData, startByteAddress](const ProjectPatching::ProjectPatch &patch) {
                      ProjectPatching::applyPatchToData(chipData, startByteAddress, patch);
                  });

    bool writeSuccess = I2cSystem::writeBytes(chipData.data(), dataSize,
                                              I2cSystem::OperationDescriptor{
                                                  .slaveAddress = targetSlaveAddress,
                                                  .byteAddress = startByteAddress,
                                                  .memType = I2cSystem::RegConfig });

    // for paged nvm programming:
    // bool writeSuccess = true;
    // for (uint16_t o = 0, inPageSize = dataSize / pageSize; o < inPageSize; ++o)
    // {
    //     writeSuccess &= I2cSystem::writeBytes(chipData.data() + o * pageSize, pageSize,
    //                                           I2cSystem::OperationDescriptor{
    //                                               .slaveAddress = targetSlaveAddress,
    //                                               .byteAddress = startByteAddress + o * pageSize
    //                                               });
    //     vTaskDelay(20 / portTICK_PERIOD_MS);
    // }

    if (writeSuccess)
    {
        SerialSystem::logMessageLn<const String &>(String("[~] Wrote #") + String(dataSize)
                                                       + String(" bytes back to slave #")
                                                       + String(targetSlaveAddress, 2)
                                                       + String(" starting at 0x")
                                                       + String(startByteAddress, 16),
                                                   SerialSystem::LoggingLevel::Warning);
    }
    else
    {
        SerialSystem::logMessageLn<const String &>(String("[~] Failed to write #")
                                                       + String(dataSize)
                                                       + String(" bytes back to slave #")
                                                       + String(targetSlaveAddress, 2)
                                                       + String(" starting at 0x")
                                                       + String(startByteAddress, 16),
                                                   SerialSystem::LoggingLevel::Critical);
    }
}

void commandDispatchThread(void *pvParameters)
{
    while (true)
    {
        PackagedCommand buffer;
        while (xQueueReceive(dispatchQueueHandle, &buffer, 10 / portTICK_PERIOD_MS)
               == errQUEUE_EMPTY)
        {
        }

        switch (buffer._commandType)
        {
        case Protocol::CommandType::Clear:
        case Protocol::CommandType::Color:
        {
            patchChip(static_cast<ColorCommandPayload *>(buffer._payload.get())->payloadPatches());
        }
        break;
        case Protocol::CommandType::Delay:
        {
            const uint16_t delay = static_cast<DelayCommandPayload *>(buffer._payload.get())
                                       ->delay();
            SerialSystem::logMessageLn<const String &>(String("Delaying for: ") + String(delay)
                                                       + String(" ms"));
            vTaskDelay(delay / portTICK_PERIOD_MS);
        }
        break;
        default:
            assert(false);
            break;
        }

        // the dynamic memory allocated for the command is freed here
    }
}

bool initDispatchSystem(ProjectPatching::PatchFactory *patchFactoryToUse, uint8_t slaveAddress)
{
    if (patchFactoryToUse == nullptr)
        return false;

    patchFactory = patchFactoryToUse;
    targetSlaveAddress = slaveAddress;

    dispatchQueueHandle = xQueueCreateStatic(DISPATCH_QUEUE_SIZE, sizeof(PackagedCommand),
                                             dispatchQueueStorage, &dispatchQueue);

    if (dispatchQueueHandle == nullptr)
        return false;

    auto dispatchTaskCreateRes = xTaskCreate(commandDispatchThread, "Command dispatcher", 2048,
                                             nullptr, 0, nullptr);

    return dispatchTaskCreateRes == pdPASS;
}

void postCommandToDispatchQueue(Protocol::CommandRepresentation command)
{
    constexpr auto commandSubmitter = [](PackagedCommand *commandToSubmit) {
        while (xQueueSend(dispatchQueueHandle, commandToSubmit, 10 / portTICK_PERIOD_MS)
               == errQUEUE_FULL)
        {
        }

        commandToSubmit->_payload.release(); // that same unique pointer is in the queue now, so we
                                             // need to release it here
    };

    const auto opCode = Protocol::determineCommand(command);
    const uint16_t payload = Protocol::extractPayload(command);

    switch (opCode)
    {
    case Protocol::CommandType::Color:
    {
        PackagedCommand colorCommand{
            ._commandType = Protocol::CommandType::Color,
            ._payload = std::make_unique<ColorCommandPayload>(
                std::vector<ProjectPatching::ProjectPatch>(
                    patchFactory->buildColorPatches(payload)->patches()))
        };
        commandSubmitter(&colorCommand);
        SerialSystem::logMessageLn("Dispatching Color command!", SerialSystem::LoggingLevel::Debug);
    }
    break;
    case Protocol::CommandType::Delay:
    {
        PackagedCommand delayCommand{ ._commandType = Protocol::CommandType::Delay,
                                      ._payload = std::make_unique<DelayCommandPayload>(payload) };
        commandSubmitter(&delayCommand);
        SerialSystem::logMessageLn("Dispatching Delay command!", SerialSystem::LoggingLevel::Debug);
    }
    break;
    case Protocol::CommandType::Clear:
    {
        PackagedCommand clearCommand{ ._commandType = Protocol::CommandType::Color,
                                      ._payload = std::make_unique<ColorCommandPayload>(
                                          std::vector<ProjectPatching::ProjectPatch>(
                                              patchFactory->buildClearPatches()->patches())) };
        commandSubmitter(&clearCommand);
        SerialSystem::logMessageLn("Dispatching Clear command!", SerialSystem::LoggingLevel::Debug);
    }
    break;
    default:
        SerialSystem::logMessageLn("Dispatch of unknown command requested!",
                                   SerialSystem::LoggingLevel::Critical);
        break;
    }
}

} // namespace DispatchSystem
