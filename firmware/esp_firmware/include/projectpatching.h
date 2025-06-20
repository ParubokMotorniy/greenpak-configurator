#pragma once

#include "i2ccommunication.h"
#include "protocol.h"

#include <array>
#include <vector>

namespace ProjectPatching
{

struct ProjectPatch
{
    uint16_t patchStartBitAddress; // where the patch begins
    std::vector<bool>
        patch; // just an array of data to write to project. Addresses increase left-to-right
};

void applyPatchToData(std::vector<uint8_t> &dataRange, uint16_t firstAddressInRange,
                      const ProjectPatch &patch);

// interprets the color code and builds the project patches needed to achieve the color
class PatchFactory
{
public:
    virtual PatchFactory *buildColorPatches(uint16_t colorCombination) = 0;
    virtual PatchFactory *buildClearPatches() = 0;
    const std::vector<ProjectPatch> &patches() { return _projectPatches; }

protected:
    std::vector<ProjectPatch> _projectPatches;
};

// for project with checksum 0x50304251 (simple_project.gp6)
// adjusts colors by (dis)connecting RGB lines to impulse generators constructed from counters
class EpilepticPatchFactory : public PatchFactory
{
public:
    EpilepticPatchFactory();
    PatchFactory *buildColorPatches(uint16_t colorCombination) override;
    PatchFactory *buildClearPatches() override;

private:
    std::vector<std::vector<bool>> _setColorsValues;
};

// for project with checksum 0x91D4AD57 (rgb_project.gp6)
// adjusts colors by changing the duty cycle of PWMs powering the LEDs
class RGBPatchFactory : public PatchFactory
{
public:
    RGBPatchFactory(uint8_t pwmTickPeriod);
    PatchFactory *buildColorPatches(uint16_t colorCombination) override;
    PatchFactory *buildClearPatches() override;

private:
    void setColorToPatch(uint8_t colorCombination, ProjectPatch &targetPatch);

private:
    uint8_t _pwmTickPeriod{ 0 };
    uint8_t _colorChangeQuant{ 0 };
};

} // namespace ProjectPatching
