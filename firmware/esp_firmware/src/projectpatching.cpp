#include "projectpatching.h"
#include "serialsystem.h"

#include <algorithm>

namespace ProjectPatching
{

ProjectPatching::EpilepticPatchFactory::EpilepticPatchFactory()
{
    _projectPatches.resize(12);
    _setColorsValues.resize(12);

    const auto patchAdder = [this](uint16_t bitAddress, std::vector<bool> &&patch,
                                   size_t patchIdx) {
        _projectPatches[patchIdx].patchStartBitAddress = bitAddress;
        _projectPatches[patchIdx].patch.resize(6);

        _setColorsValues[patchIdx] = std::move(patch);
    };

    patchAdder(420, { true, false, true, true, true, false }, 0);
    patchAdder(426, { true, false, false, false, false, true }, 1);
    patchAdder(432, { false, true, false, false, false, true }, 2);

    patchAdder(444, { true, false, true, true, true, false }, 3);
    patchAdder(456, { true, false, false, false, false, true }, 4);
    patchAdder(462, { false, true, false, false, false, true }, 5);

    patchAdder(468, { true, false, true, true, true, false }, 6);
    patchAdder(480, { true, false, false, false, false, true }, 7);
    patchAdder(492, { false, true, false, false, false, true }, 8);

    patchAdder(504, { true, false, true, true, true, false }, 9);
    patchAdder(516, { true, false, false, false, false, true }, 10);
    patchAdder(528, { false, true, false, false, false, true }, 11);
}

PatchFactory *EpilepticPatchFactory::buildColorPatches(uint16_t colorCombination)
{
    for (size_t c = 0; c < 12; ++c)
    {
        if (((colorCombination >> c) & 1) == 1)
        {
            // could've used memcpy, but vector of bools may not be an actual vector of bools
            for (size_t i = 0; i < 6; ++i)
            {
                _projectPatches[c].patch[i] = _setColorsValues[c][i];
            }
        }
        else
        {
            // could've used memset, but vector of bools may not be an actual vector of bools
            for (size_t i = 0; i < 6; ++i)
            {
                _projectPatches[c].patch[i] = false;
            }
        }
    }

    return this;
}

PatchFactory *EpilepticPatchFactory::buildClearPatches()
{
    std::for_each(_projectPatches.begin(), _projectPatches.end(),
                  [](ProjectPatching::ProjectPatch &patch) {
                      for (size_t i = 0; i < 6; ++i)
                      {
                          patch.patch[i] = false;
                      }
                  });

    return this;
}

RGBPatchFactory::RGBPatchFactory(uint8_t pwmTickPeriod)
    : _pwmTickPeriod(pwmTickPeriod), _colorChangeQuant(pwmTickPeriod / 16)
{
    assert(pwmTickPeriod >= 16);

    _projectPatches.resize(3);

    const auto patchAdder = [this](uint16_t bitAddress, size_t patchIdx) {
        _projectPatches[patchIdx].patchStartBitAddress = bitAddress;
        _projectPatches[patchIdx].patch.resize(8);
    };

    patchAdder(1472, 0); // red
    patchAdder(1504, 1); // green
    patchAdder(1544, 2); // blue
}
PatchFactory *RGBPatchFactory::buildColorPatches(uint16_t colorCombination)
{
    for (size_t m = 0; m < 3; ++m)
    {
        const uint8_t segmentColorValue = (colorCombination >> (4 * m)) & 0x0F;
        setColorToPatch(segmentColorValue, _projectPatches[m]);
    }
    return this;
}

PatchFactory *RGBPatchFactory::buildClearPatches()
{
    buildColorPatches(0);
    return this;
}

void RGBPatchFactory::setColorToPatch(uint8_t colorCombination, ProjectPatch &targetPatch)
{
    const uint8_t dutyCycle
        = colorCombination == 15 /*to avoid rounding issues when setting duty cycle to 100%*/
              ? _pwmTickPeriod
              : (colorCombination == 0 /*setting less than one is not possible*/
                     ? 1
                     : colorCombination * _colorChangeQuant);

    for (size_t h = 0; h < 8; ++h)
    {
        targetPatch.patch[h] = ((dutyCycle >> h) & 1u) == 1u;
    }
}

void applyPatchToData(std::vector<uint8_t> &dataRange, uint16_t firstAddressInRange,
                      const ProjectPatch &patch)
{
    // addresses increase left-to-right, but the bytes are imagined right-to-left
    for (size_t b = 0, bitSize = patch.patch.size(); b < bitSize; ++b)
    {
        const size_t absoluteBitIdx = b + patch.patchStartBitAddress;
        size_t byteIdx = (absoluteBitIdx / 8)
                         - firstAddressInRange; // obtains the idx of byte in 'data' to modify
        size_t bitIdx = absoluteBitIdx % 8;     // obtains the idx of bit in the byte to change

        dataRange[byteIdx] = patch.patch[b] == 0 ? dataRange[byteIdx] & ~(1 << bitIdx)
                                                 : dataRange[byteIdx] | (1 << bitIdx);
    }
}

} // namespace ProjectPatching