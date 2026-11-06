#pragma once

#include "PluginProcessor.h"

namespace vox
{
    // The factory preset library (categories become submenus in the preset menu)
    std::vector<PresetManager::Factory> makeFactoryPresets();
}
