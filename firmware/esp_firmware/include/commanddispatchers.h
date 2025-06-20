#pragma once

#include <Arduino.h>

#include "protocol.h"
#include "projectpatching.h"

namespace DispatchSystem
{

bool initDispatchSystem(ProjectPatching::PatchFactory *patchFactoryToUse, uint8_t slaveAddress);

void postCommandToDispatchQueue(Protocol::CommandRepresentation command);

} // namespace DispatchSystem
