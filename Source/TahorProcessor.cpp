#include "TahorProcessor.h"
#include "TahorEditor.h"

using namespace tahor;

//==============================================================================
// Factory presets: each lists only what it changes from the defaults (the defaults are "Vocal · Air & Body")
const std::vector<Preset>& tahor::factoryPresets()
{
    // V11: the factory library - 69 presets in ten folders (the browser's tags)
    static const std::vector<Preset> list {
        { "Init \xc2\xb7 Flat", {  }, "Utility" },
        { "Clean Slate \xc2\xb7 Everything Off", { { "resoOn", 0.0f }, { "tahorOn", 1.0f } }, "Utility" },
        { "Pop Vocal \xc2\xb7 Bright & Forward", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 90.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 250.0f }, { "t1Gain", -2.5f }, { "t1Q", 1.2f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3000.0f }, { "t2Gain", 2.0f }, { "t2Q", 0.9f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 10000.0f }, { "t3Gain", 3.0f }, { "t3Q", 0.7f }, { "rsDepth", 3.5f }, { "chain0", 1.0f }, { "u0v3", 0.6667f }, { "u0v4", 0.35f }, { "u0v5", 0.6f }, { "charMode", 1.0f } }, "Vocals" },
        { "Pop Vocal \xc2\xb7 Silky Air", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 100.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1500.0f }, { "t2Gain", -1.5f }, { "t2Q", 1.0f }, { "rsDepth", 3.0f }, { "chain0", 9.0f }, { "u8v2", 0.5f }, { "u8v3", 0.45f } }, "Vocals" },
        { "Rap Vocal \xc2\xb7 Upfront", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 110.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 300.0f }, { "t1Gain", -3.0f }, { "t1Q", 1.4f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 4500.0f }, { "t2Gain", 3.0f }, { "t2Q", 1.0f }, { "rsDepth", 4.0f }, { "rsDetail", 60.0f }, { "chain0", 4.0f }, { "u3v0", 0.5f }, { "u3v3", 0.5833f }, { "u3v4", 0.6f }, { "charMode", 2.0f }, { "charThd", 60.0f } }, "Vocals" },
        { "Rap Vocal \xc2\xb7 Dark & Heavy", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 80.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 200.0f }, { "t1Gain", 2.0f }, { "t1Q", 0.7f }, { "t3On", 1.0f }, { "t3Type", 4.0f }, { "t3Hz", 12000.0f }, { "t3Gain", 0.0f }, { "t3Q", 1.0f }, { "t3Slope", 1.0f }, { "chain0", 8.0f }, { "u7v0", 0.6f }, { "u7v3", 0.45f }, { "charMode", 2.0f } }, "Vocals" },
        { "R&B Vocal \xc2\xb7 Smooth", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 90.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 350.0f }, { "t1Gain", -2.0f }, { "t1Q", 1.0f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 8000.0f }, { "t3Gain", 2.0f }, { "t3Q", 0.7f }, { "rsDepth", 3.0f }, { "chain0", 2.0f }, { "chain1", 11.0f }, { "u1v4", 0.4f }, { "u1v5", 0.3f }, { "u10v5", 0.5625f }, { "u10v4", 0.8333f } }, "Vocals" },
        { "Worship Vocal \xc2\xb7 Warm & Open", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 80.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 2500.0f }, { "t2Gain", 1.5f }, { "t2Q", 0.8f }, { "chain0", 1.0f }, { "chain1", 16.0f }, { "u0v1", 0.2f }, { "u0v4", 0.25f } }, "Vocals" },
        { "Gospel Lead \xc2\xb7 Power", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 100.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 400.0f }, { "t1Gain", -2.0f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3500.0f }, { "t2Gain", 2.5f }, { "t2Q", 1.0f }, { "rsDepth", 4.0f }, { "chain0", 6.0f }, { "u5v4", 0.6667f }, { "u5v5", 0.3333f }, { "charMode", 2.0f }, { "charThd", 60.0f } }, "Vocals" },
        { "Vocal \xc2\xb7 Air & Body", { { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 180.0f }, { "t1Gain", 1.5f }, { "t1Q", 0.7f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 12000.0f }, { "t3Gain", 2.5f }, { "t3Q", 0.7f }, { "chain0", 1.0f }, { "u0v4", 0.3f } }, "Vocals" },
        { "Vocal \xc2\xb7 De-harsh (Dynamic)", { { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3200.0f }, { "t2Gain", 0.0f }, { "t2Q", 2.0f }, { "t2Dyn", 1.0f }, { "t2Thr", -30.0f }, { "t2Rng", -6.0f }, { "rsDepth", 6.0f }, { "rsDetail", 65.0f } }, "Vocals" },
        { "Backing Vocals \xc2\xb7 Tuck Behind", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 200.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 400.0f }, { "t1Gain", -2.0f }, { "t1Q", 1.0f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 9000.0f }, { "t3Gain", 2.0f }, { "t3Q", 0.7f }, { "t4On", 1.0f }, { "t4Type", 4.0f }, { "t4Hz", 15000.0f }, { "t4Gain", 0.0f }, { "t4Q", 1.0f }, { "t4Slope", 1.0f } }, "Vocals" },
        { "Podcast \xc2\xb7 Broadcast Voice", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 80.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 150.0f }, { "t1Gain", 2.0f }, { "t1Q", 0.7f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 300.0f }, { "t2Gain", -2.5f }, { "t2Q", 1.0f }, { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 5000.0f }, { "t3Gain", 2.0f }, { "t3Q", 1.0f }, { "rsDepth", 5.0f }, { "chain0", 5.0f }, { "u4v1", 0.5625f } }, "Vocals" },
        { "Spoken Word \xc2\xb7 Clarity", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 100.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 250.0f }, { "t1Gain", -3.0f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3000.0f }, { "t2Gain", 2.0f }, { "t2Q", 1.0f }, { "rsDepth", 3.0f } }, "Vocals" },
        { "Kick \xc2\xb7 Punch", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 30.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 60.0f }, { "t1Gain", 3.0f }, { "t1Q", 1.5f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 350.0f }, { "t2Gain", -4.0f }, { "t2Q", 1.5f }, { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 4000.0f }, { "t3Gain", 3.0f }, { "t3Q", 1.2f }, { "chain0", 6.0f }, { "u5v0", 0.6667f }, { "u5v1", 0.3333f } }, "Drums" },
        { "Kick \xc2\xb7 Sub Weight", { { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 55.0f }, { "t1Gain", 3.0f }, { "t1Q", 0.7f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 250.0f }, { "t2Gain", -3.0f }, { "t2Q", 1.2f }, { "chain0", 15.0f }, { "u14v0", 0.5f }, { "u14v1", 0.4f } }, "Drums" },
        { "Snare \xc2\xb7 Crack", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 80.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 200.0f }, { "t1Gain", 2.0f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 900.0f }, { "t2Gain", -2.5f }, { "t2Q", 1.2f }, { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 5000.0f }, { "t3Gain", 3.0f }, { "t3Q", 1.0f }, { "chain0", 3.0f }, { "u2v3", 0.6f } }, "Drums" },
        { "Snare \xc2\xb7 Fat", { { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 180.0f }, { "t1Gain", 3.0f }, { "t1Q", 1.0f }, { "t3On", 1.0f }, { "t3Type", 4.0f }, { "t3Hz", 12000.0f }, { "t3Gain", 0.0f }, { "t3Q", 1.0f }, { "t3Slope", 1.0f }, { "chain0", 4.0f }, { "u3v1", 0.5938f }, { "u3v2", 0.6667f } }, "Drums" },
        { "Drum Bus \xc2\xb7 Glue & Weight", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 25.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 80.0f }, { "t1Gain", 1.5f }, { "t1Q", 0.7f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 10000.0f }, { "t3Gain", 1.5f }, { "t3Q", 0.7f }, { "rsDepth", 2.0f }, { "chain0", 4.0f }, { "chain1", 10.0f }, { "u9v5", 0.25f }, { "charMode", 2.0f } }, "Drums" },
        { "Drum Bus \xc2\xb7 Smack", { { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 400.0f }, { "t2Gain", -2.0f }, { "t2Q", 1.0f }, { "chain0", 6.0f }, { "chain1", 12.0f }, { "charMode", 2.0f }, { "charThd", 70.0f } }, "Drums" },
        { "808 \xc2\xb7 Clean Sub", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 25.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 300.0f }, { "t1Gain", -4.0f }, { "t1Q", 1.2f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 120.0f }, { "t2Gain", 2.0f }, { "t2Q", 0.8f } }, "Drums" },
        { "Overheads \xc2\xb7 Shimmer", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 200.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 10000.0f }, { "t3Gain", 2.5f }, { "t3Q", 0.7f }, { "chain0", 9.0f }, { "u8v3", 0.4f } }, "Drums" },
        { "Master \xc2\xb7 Polish", { { "rsDepth", 2.0f }, { "rsDetail", 35.0f }, { "chain0", 11.0f }, { "chain1", 10.0f }, { "phase", 1.0f }, { "u10v1", 0.5417f }, { "u10v5", 0.6f }, { "u9v5", 0.15f } }, "Mastering" },
        { "Master \xc2\xb7 Gentle Air", { { "chain0", 13.0f }, { "u12v5", 0.8067f }, { "u12v6", 0.5469f } }, "Mastering" },
        { "Master \xc2\xb7 Warm Low End", { { "chain0", 15.0f }, { "chain1", 16.0f }, { "u14v0", 0.3333f }, { "u14v1", 0.25f }, { "u15v1", 0.55f } }, "Mastering" },
        { "Master \xc2\xb7 Loud & Clear", { { "rsDepth", 2.0f }, { "chain0", 14.0f }, { "u13v1", 0.5312f }, { "u13v7", 0.5312f }, { "u13v9", 0.5469f } }, "Mastering" },
        { "Master \xc2\xb7 Tube Glue", { { "chain0", 16.0f }, { "chain1", 12.0f }, { "charMode", 2.0f }, { "charThd", 40.0f } }, "Mastering" },
        { "Master \xc2\xb7 Tame Harshness", { { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3000.0f }, { "t2Gain", 0.0f }, { "t2Q", 1.5f }, { "t2Dyn", 1.0f }, { "t2Thr", -35.0f }, { "t2Rng", -3.0f }, { "rsDepth", 2.5f }, { "rsDetail", 40.0f } }, "Mastering" },
        { "Master \xc2\xb7 Clean Low Cut", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 25.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "phase", 1.0f } }, "Mastering" },
        { "Mix Bus \xc2\xb7 Glow", { { "rsDepth", 2.5f }, { "chain0", 10.0f }, { "chain1", 11.0f }, { "charMode", 2.0f }, { "u9v5", 0.2f }, { "u9v1", 0.55f }, { "u10v5", 0.6f } }, "Mix Bus" },
        { "Mix Bus \xc2\xb7 Punch", { { "chain0", 6.0f }, { "chain1", 4.0f }, { "u5v0", 0.5833f }, { "u5v4", 0.5833f } }, "Mix Bus" },
        { "Mix Bus \xc2\xb7 Wide Air", { { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 12000.0f }, { "t3Gain", 1.5f }, { "t3Q", 0.7f }, { "chain0", 9.0f }, { "u8v3", 0.3f } }, "Mix Bus" },
        { "Mix Bus \xc2\xb7 Tight Low End", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 30.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 200.0f }, { "t1Gain", -1.5f }, { "t1Q", 1.0f } }, "Mix Bus" },
        { "Mix Bus \xc2\xb7 Vintage Console", { { "chain0", 3.0f }, { "chain1", 5.0f }, { "charMode", 2.0f }, { "charThd", 60.0f } }, "Mix Bus" },
        { "Bass \xc2\xb7 Defined", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 35.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 80.0f }, { "t1Gain", 2.0f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 250.0f }, { "t2Gain", -2.5f }, { "t2Q", 1.2f }, { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 800.0f }, { "t3Gain", 2.0f }, { "t3Q", 1.0f }, { "chain0", 4.0f } }, "Instruments" },
        { "Bass \xc2\xb7 Round", { { "chain0", 15.0f }, { "chain1", 8.0f }, { "u14v1", 0.3f }, { "u7v0", 0.6f } }, "Instruments" },
        { "Electric Guitar \xc2\xb7 Bite", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 90.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 300.0f }, { "t1Gain", -2.0f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 2500.0f }, { "t2Gain", 2.5f }, { "t2Q", 1.0f }, { "t4On", 1.0f }, { "t4Type", 4.0f }, { "t4Hz", 9000.0f }, { "t4Gain", 0.0f }, { "t4Q", 1.0f }, { "t4Slope", 1.0f }, { "chain0", 3.0f } }, "Instruments" },
        { "Electric Guitar \xc2\xb7 Smooth", { { "t3On", 1.0f }, { "t3Type", 4.0f }, { "t3Hz", 8000.0f }, { "t3Gain", 0.0f }, { "t3Q", 1.0f }, { "t3Slope", 1.0f }, { "chain0", 8.0f }, { "charMode", 2.0f } }, "Instruments" },
        { "Acoustic \xc2\xb7 Sparkle", { { "rsDepth", 4.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 280.0f }, { "t1Gain", -2.5f }, { "t1Q", 1.4f }, { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 9000.0f }, { "t3Gain", 3.0f }, { "t3Q", 1.0f }, { "chain0", 9.0f }, { "u8v3", 0.35f } }, "Instruments" },
        { "Acoustic \xc2\xb7 Warm Body", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 70.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 200.0f }, { "t1Gain", 1.5f }, { "t1Q", 1.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 800.0f }, { "t2Gain", -1.5f }, { "t2Q", 1.0f }, { "chain0", 1.0f } }, "Instruments" },
        { "Keys \xc2\xb7 Clear Piano", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 60.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 300.0f }, { "t1Gain", -2.0f }, { "t1Q", 1.0f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 8000.0f }, { "t3Gain", 2.0f }, { "t3Q", 0.7f }, { "chain0", 13.0f } }, "Instruments" },
        { "Synth Pad \xc2\xb7 Silk", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 120.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1000.0f }, { "t2Gain", -1.5f }, { "t2Q", 1.0f }, { "chain0", 9.0f } }, "Instruments" },
        { "Strings \xc2\xb7 Air", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 80.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t3On", 1.0f }, { "t3Type", 2.0f }, { "t3Hz", 9000.0f }, { "t3Gain", 2.0f }, { "t3Q", 0.7f } }, "Instruments" },
        { "Surgical \xc2\xb7 De-mud", { { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 300.0f }, { "t1Gain", -4.0f }, { "t1Q", 1.4f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 De-box", { { "t1On", 1.0f }, { "t1Type", 0.0f }, { "t1Hz", 500.0f }, { "t1Gain", -4.0f }, { "t1Q", 2.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Harshness Cut", { { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 3000.0f }, { "t2Gain", -5.0f }, { "t2Q", 4.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Ring Notch 1 kHz", { { "t2On", 1.0f }, { "t2Type", 5.0f }, { "t2Hz", 1000.0f }, { "t2Gain", 0.0f }, { "t2Q", 8.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Sibilance (Dynamic)", { { "t3On", 1.0f }, { "t3Type", 0.0f }, { "t3Hz", 7000.0f }, { "t3Gain", 0.0f }, { "t3Q", 3.0f }, { "t3Dyn", 1.0f }, { "t3Thr", -35.0f }, { "t3Rng", -8.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Rumble Cut", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 60.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 5.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Clean Highs", { { "t4On", 1.0f }, { "t4Type", 4.0f }, { "t4Hz", 16000.0f }, { "t4Gain", 0.0f }, { "t4Q", 1.0f }, { "t4Slope", 3.0f } }, "Tahor Surgical" },
        { "Surgical \xc2\xb7 Hum Remover 60 Hz", { { "t0On", 1.0f }, { "t0Type", 5.0f }, { "t0Hz", 60.0f }, { "t0Gain", 0.0f }, { "t0Q", 12.0f }, { "t1On", 1.0f }, { "t1Type", 5.0f }, { "t1Hz", 120.0f }, { "t1Gain", 0.0f }, { "t1Q", 12.0f }, { "t2On", 1.0f }, { "t2Type", 5.0f }, { "t2Hz", 180.0f }, { "t2Gain", 0.0f }, { "t2Q", 12.0f }, { "t3On", 1.0f }, { "t3Type", 5.0f }, { "t3Hz", 240.0f }, { "t3Gain", 0.0f }, { "t3Q", 12.0f } }, "Tahor Surgical" },
        { "Reso \xc2\xb7 De-ess", { { "rsDepth", 6.0f }, { "rsLow", 4000.0f }, { "rsHigh", 12000.0f }, { "rsMode", 1.0f }, { "rn0On", 0.0f }, { "rn1On", 0.0f }, { "rn2On", 0.0f }, { "rn3On", 1.0f }, { "rn3Shape", 0.0f }, { "rn3Hz", 7000.0f }, { "rn3Amt", 3.0f }, { "rn3Q", 1.0f } }, "Reso Clean-up" },
        { "Reso \xc2\xb7 Harsh Vocal", { { "rsDepth", 5.0f }, { "rsDetail", 60.0f }, { "rsLow", 1500.0f }, { "rsHigh", 8000.0f }, { "rn1On", 1.0f }, { "rn1Shape", 0.0f }, { "rn1Hz", 3000.0f }, { "rn1Amt", 3.0f }, { "rn1Q", 1.2f } }, "Reso Clean-up" },
        { "Reso \xc2\xb7 Boomy Room", { { "rsDepth", 5.0f }, { "rsLow", 80.0f }, { "rsHigh", 600.0f }, { "rn0On", 0.0f }, { "rn1On", 0.0f }, { "rn2On", 0.0f }, { "rn3On", 1.0f }, { "rn3Shape", 0.0f }, { "rn3Hz", 200.0f }, { "rn3Amt", 3.0f }, { "rn3Q", 1.0f } }, "Reso Clean-up" },
        { "Reso \xc2\xb7 Gentle Polish", { { "rsDepth", 3.0f }, { "rsDetail", 40.0f } }, "Reso Clean-up" },
        { "Reso \xc2\xb7 Guitar Fizz", { { "rsDepth", 5.0f }, { "rsLow", 3000.0f }, { "rsHigh", 10000.0f } }, "Reso Clean-up" },
        { "Reso \xc2\xb7 Sharp Resonances", { { "rsDepth", 6.0f }, { "rsDetail", 75.0f }, { "rsMode", 1.0f } }, "Reso Clean-up" },
        { "Character \xc2\xb7 Pultec Lift", { { "chain0", 1.0f }, { "u0v0", 0.6667f }, { "u0v1", 0.5f }, { "u0v2", 0.35f }, { "u0v3", 0.6667f }, { "u0v4", 0.4f }, { "u0v5", 0.5f } }, "Character" },
        { "Character \xc2\xb7 British Console", { { "chain0", 3.0f }, { "chain1", 5.0f }, { "charMode", 2.0f }, { "u2v4", 0.5667f }, { "u4v1", 0.5625f } }, "Character" },
        { "Character \xc2\xb7 Fire & Silk", { { "chain0", 12.0f }, { "chain1", 13.0f }, { "u11v1", 0.3f }, { "u11v6", 0.3f }, { "u12v6", 0.5625f }, { "charMode", 2.0f }, { "charThd", 60.0f } }, "Character" },
        { "Character \xc2\xb7 Passive Weight", { { "chain0", 15.0f }, { "chain1", 16.0f }, { "u14v1", 0.35f }, { "u15v9", 0.575f } }, "Character" },
        { "Character \xc2\xb7 Helios Grit", { { "chain0", 8.0f }, { "u7v2", 0.65f }, { "charMode", 2.0f }, { "charThd", 80.0f } }, "Character" },
        { "Character \xc2\xb7 Tube Stack", { { "chain0", 1.0f }, { "chain1", 2.0f }, { "chain2", 11.0f }, { "charMode", 2.0f }, { "charThd", 55.0f }, { "u0v4", 0.3f }, { "u1v3", 0.3f } }, "Character" },
        { "FX \xc2\xb7 Telephone", { { "resoOn", 0.0f }, { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 400.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 4.0f }, { "t1Hz", 3400.0f }, { "t1Gain", 0.0f }, { "t1Q", 1.0f }, { "t1Slope", 3.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1500.0f }, { "t2Gain", 4.0f }, { "t2Q", 1.0f }, { "chain0", 3.0f }, { "charMode", 2.0f }, { "charThd", 80.0f } }, "Effects" },
        { "FX \xc2\xb7 AM Radio", { { "resoOn", 0.0f }, { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 200.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 3.0f }, { "t1On", 1.0f }, { "t1Type", 4.0f }, { "t1Hz", 4500.0f }, { "t1Gain", 0.0f }, { "t1Q", 1.0f }, { "t1Slope", 3.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1200.0f }, { "t2Gain", 3.0f }, { "t2Q", 1.0f }, { "chain0", 4.0f }, { "charMode", 2.0f }, { "charThd", 70.0f } }, "Effects" },
        { "FX \xc2\xb7 Walkie-Talkie", { { "resoOn", 0.0f }, { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 600.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 4.0f }, { "t1On", 1.0f }, { "t1Type", 4.0f }, { "t1Hz", 2800.0f }, { "t1Gain", 0.0f }, { "t1Q", 1.0f }, { "t1Slope", 4.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1800.0f }, { "t2Gain", 6.0f }, { "t2Q", 1.5f }, { "chain0", 6.0f }, { "charMode", 2.0f }, { "charThd", 100.0f } }, "Effects" },
        { "FX \xc2\xb7 Lo-Fi", { { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 120.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 2.0f }, { "t1On", 1.0f }, { "t1Type", 4.0f }, { "t1Hz", 6000.0f }, { "t1Gain", 0.0f }, { "t1Q", 1.0f }, { "t1Slope", 2.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 900.0f }, { "t2Gain", 2.0f }, { "t2Q", 1.0f }, { "chain0", 8.0f }, { "charMode", 2.0f }, { "charThd", 90.0f } }, "Effects" },
        { "FX \xc2\xb7 Megaphone", { { "resoOn", 0.0f }, { "t0On", 1.0f }, { "t0Type", 3.0f }, { "t0Hz", 500.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 4.0f }, { "t1On", 1.0f }, { "t1Type", 4.0f }, { "t1Hz", 3500.0f }, { "t1Gain", 0.0f }, { "t1Q", 1.0f }, { "t1Slope", 4.0f }, { "t2On", 1.0f }, { "t2Type", 0.0f }, { "t2Hz", 1500.0f }, { "t2Gain", 8.0f }, { "t2Q", 2.0f }, { "chain0", 4.0f }, { "chain1", 6.0f }, { "charMode", 2.0f }, { "charThd", 100.0f } }, "Effects" },
        { "FX \xc2\xb7 Underwater", { { "resoOn", 0.0f }, { "t0On", 1.0f }, { "t0Type", 4.0f }, { "t0Hz", 500.0f }, { "t0Gain", 0.0f }, { "t0Q", 1.0f }, { "t0Slope", 5.0f }, { "t1On", 1.0f }, { "t1Type", 1.0f }, { "t1Hz", 150.0f }, { "t1Gain", 4.0f }, { "t1Q", 0.7f } }, "Effects" },
    };
    return list;
}

//==============================================================================
TahorProcessor::TahorProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "TAHOR", createLayout())
{
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p)) apvts.addParameterListener (wp->paramID, &dirtyListener);
    startTimerHz (20);
}

TahorProcessor::~TahorProcessor()
{
    for (auto* p : getParameters())
        if (auto* wp = dynamic_cast<juce::AudioProcessorParameterWithID*> (p)) apvts.removeParameterListener (wp->paramID, &dirtyListener);
}

juce::AudioProcessorValueTreeState::ParameterLayout TahorProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout L;
    using R = juce::NormalisableRange<float>;
    auto F = [&L] (const juce::String& id, const juce::String& name, R r, float def, const juce::String& unit = {})
    { L.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def, juce::AudioParameterFloatAttributes().withLabel (unit))); };
    auto B = [&L] (const juce::String& id, const juce::String& name, bool def)
    { L.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    auto C = [&L] (const juce::String& id, const juce::String& name, juce::StringArray items, int def)
    { L.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, items, def)); };
    auto hz = [] (float lo, float hi) { R r (lo, hi); r.setSkewForCentre (std::sqrt (lo * hi)); return r; };

    // global
    B ("bypass", "Bypass", false);
    B ("match", "Match", false);
    C ("phase", "Processing", { "Natural Phase", "Linear Phase", "Linear Phase HQ" }, 0);
    C ("os", "Oversampling", { "Off", "2x", "4x", "8x", "16x", "32x" }, 1);
    C ("osRender", "Render Oversampling", { "Same", "2x", "4x", "8x", "16x", "32x" }, 5);
    F ("inGain", "Input", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    F ("outGain", "Output", R (-24.0f, 24.0f, 0.1f), 0.0f, "dB");
    B ("scExt", "Sidechain External", true);
    C ("analyzer", "Analyzer", { "Pre + Post", "Post", "Pre", "Off" }, 0);
    C ("range", "Range", { juce::String::fromUTF8 ("\xc2\xb1" "3 dB"), juce::String::fromUTF8 ("\xc2\xb1" "6 dB"),
                           juce::String::fromUTF8 ("\xc2\xb1" "12 dB"), juce::String::fromUTF8 ("\xc2\xb1" "30 dB") }, 2);
    B ("resoOn", "Reso On", true);
    B ("tahorOn", "Tahor On", true);
    B ("charOn", "Character On", true);

    // RESO
    F ("rsDepth", "Reso Depth", R (0.0f, 10.0f, 0.1f), 2.0f);   // V5: starts at nine o'clock
    F ("rsDetail", "Reso Detail", R (0.0f, 100.0f, 1.0f), 50.0f, "%");
    { R r (0.5f, 50.0f, 0.1f); r.setSkewForCentre (5.0f); F ("rsAttack", "Reso Attack", r, 5.0f, "ms"); }
    { R r (5.0f, 500.0f, 1.0f); r.setSkewForCentre (80.0f); F ("rsRelease", "Reso Release", r, 80.0f, "ms"); }
    F ("rsMaxCut", "Reso Max Cut", R (0.0f, 24.0f, 0.5f), 12.0f, "dB");
    F ("rsMix", "Reso Mix", R (0.0f, 100.0f, 1.0f), 100.0f, "%");
    F ("rsTrim", "Reso Trim", R (-12.0f, 12.0f, 0.1f), 0.0f, "dB");
    C ("rsMode", "Reso Soft/Hard", { "Soft", "Hard" }, 0);
    B ("rsLowLat", "Reso Low Latency", false);
    C ("rsMs", "Reso Stereo", { "Stereo", "Mid", "Side" }, 0);
    B ("rsDelta", "Reso Delta", false);
    B ("rsKey", "Reso Sidechain", false);
    F ("rsLink", "Reso Stereo Link", R (0.0f, 100.0f, 1.0f), 100.0f, "%");
    F ("rsDTilt", "Reso Detail Tilt", R (-100.0f, 100.0f, 1.0f), 10.0f, "%");
    F ("rsATilt", "Reso Attack Tilt", R (-100.0f, 100.0f, 1.0f), 0.0f, "%");
    F ("rsRTilt", "Reso Release Tilt", R (-100.0f, 100.0f, 1.0f), -15.0f, "%");
    C ("rsPhase", "Reso Phase", { "Minimum", "Linear" }, 0);
    F ("rsLow", "Reso Focus Low", hz (20.0f, 20000.0f), 47.5f, "Hz");
    F ("rsHigh", "Reso Focus High", hz (20.0f, 20000.0f), 12000.0f, "Hz");
    struct ND { bool on; int shape; float hz, amt, q; };
    const ND nd[maxResoNodes] { { true, 1, 167.0f, 2.0f, 0.8f }, { true, 0, 970.0f, 3.0f, 1.2f }, { true, 4, 4500.0f, 1.5f, 0.5f },
                                { false, 0, 300.0f, 0.0f, 1.0f }, { false, 0, 2000.0f, 0.0f, 1.0f }, { false, 0, 8000.0f, 0.0f, 1.0f } };
    const juce::StringArray shapes { "Bell", "Bandpass", "Low shelf", "High shelf", "Tilt", "Notch", "Low cut", "High cut" };
    for (int i = 0; i < maxResoNodes; ++i)
    {
        const auto n = "Reso Band " + juce::String (i + 1) + " ";
        B (nid (i, "On"), n + "On", nd[i].on);
        C (nid (i, "Shape"), n + "Shape", shapes, nd[i].shape);
        F (nid (i, "Hz"), n + "Freq", hz (20.0f, 20000.0f), nd[i].hz, "Hz");
        F (nid (i, "Amt"), n + "Amount", R (-6.0f, 6.0f, 0.1f), nd[i].amt, "dB");
        { R r (0.2f, 6.0f, 0.01f); r.setSkewForCentre (1.0f); F (nid (i, "Q"), n + "Width", r, nd[i].q); }
        C (nid (i, "Place"), n + "Side", { "ST", "L", "R" }, 0);                                   // V9
        C (nid (i, "Pair"), n + "Partner", { "-", "1", "2", "3", "4", "5", "6" }, 0);              // V9: its other side
    }

    // TAHOR
    struct TB { bool on; int type; float hz, db, q; int slope, place; bool dyn, sc; };
    // V3: Tahor starts flat - every band off (they keep their places for when you switch one on)
    const TB td[numBands] { { false, 3, 40.0f, 0.0f, 0.71f, 1, 0, false, false },
                            { false, 0, 280.0f, 0.0f, 1.4f, 1, 0, false, false },
                            { false, 0, 2800.0f, 0.0f, 1.0f, 1, 0, false, false },
                            { false, 0, 3800.0f, 0.0f, 1.0f, 1, 0, false, false },
                            { false, 4, 17000.0f, 0.0f, 0.71f, 2, 0, false, false },
                            { false, 0, 600.0f, 0.0f, 1.0f, 1, 0, false, false },
                            { false, 0, 1200.0f, 0.0f, 1.0f, 1, 0, false, false },
                            { false, 0, 6000.0f, 0.0f, 1.0f, 1, 0, false, false } };
    for (int b = 0; b < numBands; ++b)
    {
        const auto n = "Band " + juce::String (b + 1) + " ";
        B (bid (b, "On"), n + "On", td[b].on);
        C (bid (b, "Type"), n + "Shape", { "Bell", "Low shelf", "High shelf", "Low cut", "High cut", "Notch" }, td[b].type);
        F (bid (b, "Hz"), n + "Freq", hz (10.0f, 30000.0f), td[b].hz, "Hz");
        F (bid (b, "Gain"), n + "Gain", R (-30.0f, 30.0f, 0.1f), td[b].db, "dB");
        { R r (0.1f, 30.0f, 0.01f); r.setSkewForCentre (1.0f); F (bid (b, "Q"), n + "Q", r, td[b].q); }
        C (bid (b, "Slope"), n + "Slope", { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, td[b].slope);
        C (bid (b, "Place"), n + "Stereo", { "ST", "M", "S", "L", "R" }, td[b].place);
        B (bid (b, "Dyn"), n + "Dynamic", td[b].dyn);
        F (bid (b, "Thr"), n + "Threshold", R (-60.0f, 0.0f, 0.1f), -30.0f, "dB");
        F (bid (b, "Rng"), n + "Range", R (-24.0f, 24.0f, 0.1f), -6.0f, "dB");
        B (bid (b, "Spec"), n + "Spectral", false);
        B (bid (b, "Sc"), n + "Sidechain", td[b].sc);
        B (bid (b, "Solo"), n + "Solo", false);
        { R r (0.1f, 200.0f, 0.1f); r.setSkewForCentre (10.0f); F (bid (b, "Att"), n + "Attack", r, 5.0f, "ms"); }      // V9
        { R r (5.0f, 2000.0f, 1.0f); r.setSkewForCentre (120.0f); F (bid (b, "Rel"), n + "Release", r, 120.0f, "ms"); }  // V9
        F (bid (b, "Sel"), n + "Selectivity", R (1.0f, 8.0f, 0.1f), 3.0f, "x");                                         // V9: spectral
        C (bid (b, "Pair"), n + "Partner", { "-", "1", "2", "3", "4", "5", "6", "7", "8" }, 0);                         // V9: its other side
    }

    // CHARACTER
    C ("charMode", "Character Colour", { "Clean", "Subtle", "Warm" }, 1);
    F ("charThd", "Character THD", R (0.0f, 100.0f, 0.1f), 50.0f, "%");   // V7: how much colour (50% = as before)
    juce::StringArray unitNames { "Empty" };
    for (auto& u : units()) unitNames.add (u.name);
    const int cdef[maxChain] { 0, 0, 0, 0 };   // V11: Character starts empty
    for (int s = 0; s < maxChain; ++s) C ("chain" + juce::String (s), "Character Slot " + juce::String (s + 1), unitNames, cdef[s]);
    for (int u = 0; u < numUnits; ++u)
        for (int v = 0; v < unitValues; ++v)
            F (uid (u, v), juce::String (units()[(size_t) u].name) + " " + juce::String (v + 1), R (0.0f, 1.0f, 0.0001f), units()[(size_t) u].defaults[(size_t) v]);
    // V9: each unit can split into left and right, the right side with its own values
    for (int u = 0; u < numUnits; ++u)
    {
        B ("u" + juce::String (u) + "split", juce::String (units()[(size_t) u].name) + " L/R Split", false);
        for (int v = 0; v < unitValues; ++v)
            F ("u" + juce::String (u) + "r" + juce::String (v), juce::String (units()[(size_t) u].name) + " R " + juce::String (v + 1), R (0.0f, 1.0f, 0.0001f), units()[(size_t) u].defaults[(size_t) v]);
    }
    B ("autoGain", "Auto Gain", false);   // V9
    return L;
}

//==============================================================================
vox::CleanBands TahorProcessor::tahorBands() const
{
    vox::CleanBands bands;
    for (int b = 0; b < numBands; ++b)
    {
        auto& c = bands[(size_t) b];
        c.on = param (bid (b, "On")) > 0.5f;
        c.type = (vox::BandType) (int) param (bid (b, "Type"));
        c.freq = param (bid (b, "Hz")); c.gain = param (bid (b, "Gain")); c.q = param (bid (b, "Q"));
        c.slope = (int) param (bid (b, "Slope"));
        c.place = (int) param (bid (b, "Place"));
        c.dyn = param (bid (b, "Dyn")) > 0.5f || param (bid (b, "Spec")) > 0.5f;   // V10: an old Spectral band plays as Dynamic
        c.dynThresh = param (bid (b, "Thr")); c.dynRange = param (bid (b, "Rng"));
        c.spectral = false;   // V10: Spectral is gone from Tahor (Reso does that job)
        c.sc = param (bid (b, "Sc")) > 0.5f;
        c.dynAttMs = param (bid (b, "Att")); c.dynRelMs = param (bid (b, "Rel")); c.specSel = param (bid (b, "Sel"));   // V9
    }
    return bands;
}

ResoProSettings TahorProcessor::resoSettings (int side) const
{
    ResoProSettings s;
    s.on = param ("resoOn") > 0.5f;
    s.depth = param ("rsDepth"); s.detail = param ("rsDetail") / 100.0f;
    s.attackMs = param ("rsAttack"); s.releaseMs = param ("rsRelease");
    s.maxCutDb = param ("rsMaxCut"); s.mix = param ("rsMix") / 100.0f; s.trimDb = param ("rsTrim");
    s.hard = param ("rsMode") > 0.5f; s.msMode = (int) param ("rsMs");
    s.delta = param ("rsDelta") > 0.5f; s.sidechain = param ("rsKey") > 0.5f;
    s.link = param ("rsLink") / 100.0f;
    s.detailTilt = param ("rsDTilt") / 100.0f; s.attackTilt = param ("rsATilt") / 100.0f; s.releaseTilt = param ("rsRTilt") / 100.0f;
    s.minPhase = param ("rsPhase") < 0.5f;
    s.lowHz = param ("rsLow"); s.highHz = param ("rsHigh");
    for (int i = 0; i < maxResoNodes; ++i)
    {
        auto& n = s.nodes[(size_t) i];
        n.on = param (nid (i, "On")) > 0.5f; n.shape = (int) param (nid (i, "Shape"));
        n.hz = param (nid (i, "Hz")); n.amount = param (nid (i, "Amt")); n.q = param (nid (i, "Q"));
        const int pl = (int) param (nid (i, "Place"));   // V9: a split node only works on its own side
        if (side >= 0 && pl != 0 && pl != side + 1) n.on = false;
    }
    return s;
}

void TahorProcessor::getUnitValues (int unit, float* v, int side) const
{
    for (int j = 0; j < unitValues; ++j) v[j] = param (side == 1 ? "u" + juce::String (unit) + "r" + juce::String (j) : uid (unit, j));
}

std::vector<int> TahorProcessor::chain() const
{
    std::vector<int> c;
    for (int s = 0; s < maxChain; ++s) { const int u = (int) param ("chain" + juce::String (s)) - 1; if (u >= 0) c.push_back (u); }
    return c;
}

void TahorProcessor::setChain (const std::vector<int>& c)
{
    for (int s = 0; s < maxChain; ++s) setParam ("chain" + juce::String (s), s < (int) c.size() ? (float) (c[(size_t) s] + 1) : 0.0f);
}

bool TahorProcessor::hasSidechain() const
{
    if (auto* bus = getBus (true, 1)) return bus->isEnabled() && bus->getNumberOfChannels() > 0;
    return false;
}

//==============================================================================
void TahorProcessor::prepareOs (int mb)
{
    for (int k = 0; k < 5; ++k)
    {
        os[(size_t) k] = std::make_unique<juce::dsp::Oversampling<float>> (2, (size_t) (k + 1),
                            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
        os[(size_t) k]->initProcessing ((size_t) juce::jmax (32, mb));
    }
}

void TahorProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    preparedRate = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);
    reso.prepare (sampleRate, false);
    resoLow.prepare (sampleRate, true);
    for (int li = 0; li < 2; ++li) for (auto& rr : resoSplit[(size_t) li]) rr.prepare (sampleRate, li == 1);
    tahorStage.prepare (sampleRate, maxBlock);
    tahorStage.msEnabled = true;
    lrEq.prepare (sampleRate);
    for (auto& e : charEq) { e.prepare (sampleRate, maxBlock); e.externalColour = true; }
    for (auto& e : charEqR) { e.prepare (sampleRate, maxBlock); e.externalColour = true; }
    for (auto& s : charSat) s.prepare (sampleRate);
    charWas.fill (-1);
    prepareOs (maxBlock);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) maxBlock, 1 };
    for (auto& st : soloC) for (auto& f : st) { f.coefficients = new juce::dsp::IIR::Coefficients<float> (1, 0, 0, 1, 0, 0); f.prepare (spec); }
    for (auto& st : soloF) for (auto& f : st) { f.coefficients = new juce::dsp::IIR::Coefficients<float> (1, 0, 0, 1, 0, 0); f.prepare (spec); }
    dry.setSize (2, maxBlock);
    keyBuf.setSize (2, maxBlock);
    for (auto& d : dryDelay) d.assign (1 << 17, 0.0f);
    dryPos = 0;
    reportedLatency = -1;
    setLatencySamples (latencyNow());
}

bool TahorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (layouts.getMainInputChannelSet() != out) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::stereo() && sc != juce::AudioChannelSet::mono()) return false;
    }
    return true;
}

int TahorProcessor::latencyNow() const
{
    const int osIdx = isNonRealtime() && (int) param ("osRender") > 0 ? (int) param ("osRender") : (int) param ("os");
    int lat = (param ("rsLowLat") > 0.5f ? resoLow : reso).getLatencySamples();
    lat += tahorStage.latencyFor ((int) param ("phase"), juce::jmin (2, osIdx));
    if (osIdx > 0 && os[(size_t) osIdx - 1]) lat += (int) std::round (os[(size_t) osIdx - 1]->getLatencyInSamples());
    return lat;
}

void TahorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto main = getBusBuffer (buffer, true, 0);
    const int n = main.getNumSamples();
    const int nc = juce::jmin (2, main.getNumChannels());
    if (n == 0 || nc == 0) return;
    if (n > maxBlock) { prepareToPlay (preparedRate, n); }

    // the sidechain (key) input
    const bool scOn = param ("scExt") > 0.5f && hasSidechain();
    if (scOn)
    {
        auto key = getBusBuffer (buffer, true, 1);
        keyBuf.setSize (2, n, false, false, true);
        for (int c = 0; c < 2; ++c) keyBuf.copyFrom (c, 0, key, juce::jmin (c, key.getNumChannels() - 1), 0, n);
    }

    // the untouched signal, delayed by the latency, for BYPASS
    const int lat = latencyNow();
    const int dlen = (int) dryDelay[0].size();
    for (int c = 0; c < 2; ++c)
    {
        dry.setSize (2, n, false, false, true);
        auto* x = main.getReadPointer (juce::jmin (c, nc - 1));
        auto* d = dry.getWritePointer (c);
        auto& line = dryDelay[c];
        for (int i = 0; i < n; ++i)
        {
            const int w = (dryPos + i) & (dlen - 1);
            line[(size_t) w] = x[i];
            d[i] = line[(size_t) ((w - juce::jmin (lat, dlen - 1)) & (dlen - 1))];
        }
    }
    dryPos = (dryPos + n) & (dlen - 1);

    // input level (and AUTO learning) + MATCH's reference
    double msIn = 0.0;
    for (int c = 0; c < nc; ++c) { auto* x = main.getReadPointer (c); for (int i = 0; i < n; ++i) msIn += (double) x[i] * x[i]; }
    msIn /= (double) (n * nc);
    if (autoLearning.load()) { learnSum += msIn * n; learnCount += n; }
    const float inG = juce::Decibels::decibelsToGain (param ("inGain"));
    main.applyGainRamp (0, n, inSmooth, inG); inSmooth = inG;
    preTap.push (main);
    inMeterL.store (main.getMagnitude (0, 0, n));
    inMeterR.store (main.getMagnitude (nc - 1, 0, n));
    { float r2 = 0; for (int c = 0; c < nc; ++c) r2 = juce::jmax (r2, main.getRMSLevel (c, 0, n)); inRms.store (r2); }   // V10: for the readouts

    // RESO
    {
        bool split = false;   // V9: a node on one side only - left and right each get their own Reso
        if (nc == 2) for (int i = 0; i < maxResoNodes; ++i) if (param (nid (i, "On")) > 0.5f && (int) param (nid (i, "Place")) != 0) split = true;
        resoIsSplit.store (split);
        if (! split)
        {
            auto rs = resoSettings();
            auto& r = param ("rsLowLat") > 0.5f ? resoLow : reso;
            r.process (main, rs, scOn ? &keyBuf : nullptr);
        }
        else
        {
            const int li = param ("rsLowLat") > 0.5f ? 1 : 0;
            for (int side = 0; side < 2; ++side)
            {
                juce::AudioBuffer<float> one (main.getArrayOfWritePointers() + side, 1, n);
                juce::AudioBuffer<float> key1 (keyBuf.getArrayOfWritePointers() + side, 1, n);
                auto rs = resoSettings (side);
                rs.msMode = 0;
                resoSplit[(size_t) li][(size_t) side].process (one, rs, scOn ? &key1 : nullptr);
            }
        }
    }

    // TAHOR (L / R bands run in their own Natural EQ)
    const int osIdx = isNonRealtime() && (int) param ("osRender") > 0 ? (int) param ("osRender") : (int) param ("os");
    auto bands = tahorBands();
    vox::CleanBands lr = bands;
    bool anyLr = false, anySolo = false; int soloBand = -1;
    for (int b = 0; b < numBands; ++b)
    {
        const bool isLr = bands[(size_t) b].place >= 3;
        if (isLr && nc > 1) { bands[(size_t) b].on = false; anyLr = anyLr || lr[(size_t) b].on; }
        else lr[(size_t) b].on = false;
        if (bands[(size_t) b].place >= 3 && nc < 2) bands[(size_t) b].place = 0;
        if (param (bid (b, "Solo")) > 0.5f && param (bid (b, "On")) > 0.5f && ! anySolo) { anySolo = true; soloBand = b; }
    }
    const bool tahorOn = param ("tahorOn") > 0.5f;
    tahorStage.keyL = scOn ? keyBuf.getReadPointer (0) : nullptr;
    tahorStage.keyR = scOn ? keyBuf.getReadPointer (1) : nullptr;
    tahorStage.keyLen = n;
    lrEq.keyL = tahorStage.keyL; lrEq.keyR = tahorStage.keyR; lrEq.keyLen = n;
    tahorStage.process (main, bands, tahorOn, (int) param ("phase"), juce::jmin (2, osIdx));
    if (tahorOn && anyLr) lrEq.process (main, lr);
    for (int b = 0; b < numBands; ++b)
        dynDb[(size_t) b].store (bands[(size_t) b].on ? tahorStage.dynOffsetDb[(size_t) b].load() : lrEq.dynOffsetDb[(size_t) b].load());

    // SOLO: listen to just the band
    if (anySolo)
    {
        const float f = juce::jlimit (20.0f, (float) (preparedRate * 0.45), param (bid (soloBand, "Hz")));
        const float q = juce::jlimit (0.3f, 12.0f, param (bid (soloBand, "Q")));
        const auto bp = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (preparedRate, f, q);
        for (int c = 0; c < nc; ++c)
            for (int st = 0; st < 2; ++st)
            {
                *soloF[(size_t) c][(size_t) st].coefficients = bp;
                auto* x = main.getWritePointer (c);
                for (int i = 0; i < n; ++i) x[i] = soloF[(size_t) c][(size_t) st].processSample (x[i]);
            }
    }

    // CHARACTER: the units' EQs, then their colour (oversampled)
    const bool charOn = param ("charOn") > 0.5f && ! anySolo;
    const int mode = (int) param ("charMode");
    const auto ch = chain();
    std::array<int, maxChain> active { -1, -1, -1, -1 };
    if (charOn)
        for (int s = 0; s < (int) ch.size(); ++s)
        {
            const int u = ch[(size_t) s];
            float v[unitValues]; getUnitValues (u, v);
            if (u == 5 && v[10] < 0.5f) continue;   // the 550B's IN switch
            auto& e = charEq[(size_t) s];
            auto& er = charEqR[(size_t) s];
            if (charWas[(size_t) s] != u) { e.reset(); er.reset(); charSat[(size_t) s].reset(); charWas[(size_t) s] = u; }
            e.update (unitSettings (u, v), units()[(size_t) u].type);
            e.place = 0;
            if (nc == 2 && unitSplit (u))   // V9: left and right with their own settings
            {
                float vr[unitValues]; getUnitValues (u, vr, 1);
                er.update (unitSettings (u, vr), units()[(size_t) u].type);
                er.place = 0;
                juce::AudioBuffer<float> l (main.getArrayOfWritePointers(), 1, n), rr (main.getArrayOfWritePointers() + 1, 1, n);
                e.process (l); er.process (rr);
            }
            else e.process (main);
            active[(size_t) s] = u;
        }
    if (osIdx != osWas) { for (auto& o : os) if (o) o->reset(); osWas = osIdx; }
    auto colour = [&] (juce::dsp::AudioBlock<float> blk)
    {
        if (! charOn || mode == 0) return;
        const float thd = param ("charThd") / 50.0f;   // V7: 0 .. 2, 1 = as before
        const float mul = (mode == 2 ? 2.2f : 1.0f) * thd, dmul = (mode == 2 ? 1.35f : 1.0f) * (0.6f + 0.4f * thd);
        for (int s = 0; s < maxChain; ++s)
        {
            if (active[(size_t) s] < 0) continue;
            float drive, bias, mix;
            vox::VocalEQ::colourFor (units()[(size_t) active[(size_t) s]].type, drive, bias, mix);
            mix = juce::jmin (1.0f, mix * mul); drive *= dmul;
            if (mix <= 0.0f) continue;
            for (size_t c = 0; c < blk.getNumChannels(); ++c)
            {
                auto* x = blk.getChannelPointer (c);
                for (size_t i = 0; i < blk.getNumSamples(); ++i) x[i] = charSat[(size_t) s].processSample ((int) c, x[i], drive, bias, mix);
            }
        }
    };
    {
        juce::dsp::AudioBlock<float> block (main.getArrayOfWritePointers(), (size_t) nc, (size_t) n);
        if (osIdx > 0 && os[(size_t) osIdx - 1])
        {
            auto up = os[(size_t) osIdx - 1]->processSamplesUp (block);
            colour (up);
            os[(size_t) osIdx - 1]->processSamplesDown (block);
        }
        else colour (block);
    }

    // V8: SOLO a Character node - just its range, after the units' EQ and colour
    if (const float sh = charSoloHz.load(); sh > 0.0f && ! anySolo)
    {
        const float f = juce::jlimit (20.0f, (float) (preparedRate * 0.45), sh);
        const float q = juce::jlimit (0.3f, 12.0f, charSoloQ.load());
        const auto bp = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (preparedRate, f, q);
        for (int c = 0; c < nc; ++c)
            for (int st = 0; st < 2; ++st)
            {
                *soloC[(size_t) c][(size_t) st].coefficients = bp;
                auto* x = main.getWritePointer (c);
                for (int i = 0; i < n; ++i) x[i] = soloC[(size_t) c][(size_t) st].processSample (x[i]);
            }
    }

    // output: MATCH (equal loudness with the input), OUT, BYPASS
    double msOut = 0.0;
    for (int c = 0; c < nc; ++c) { auto* x = main.getReadPointer (c); for (int i = 0; i < n; ++i) msOut += (double) x[i] * x[i]; }
    msOut /= (double) (n * nc);
    const float k = std::exp (-(float) n / (1.5f * (float) preparedRate));
    matchIn = k * matchIn + (1.0f - k) * (float) msIn;
    matchOut = k * matchOut + (1.0f - k) * (float) msOut;
    float target = 1.0f;
    if (param ("match") > 0.5f && matchOut > 1.0e-9f && matchIn > 1.0e-9f) target = juce::jlimit (0.125f, 8.0f, std::sqrt (matchIn / matchOut));
    matchGain += (target - matchGain) * (1.0f - k);
    const float outG = juce::Decibels::decibelsToGain (param ("outGain") + (param ("autoGain") > 0.5f ? autoGainDb.load() : 0.0f)) * matchGain;   // V9: + auto gain
    main.applyGainRamp (0, n, outSmooth, outG); outSmooth = outG;

    if (param ("bypass") > 0.5f)
        for (int c = 0; c < nc; ++c) main.copyFrom (c, 0, dry, c, 0, n);

    postTap.push (main);
    meterL.store (main.getMagnitude (0, 0, n));
    meterR.store (main.getMagnitude (nc - 1, 0, n));
    { float r2 = 0; for (int c = 0; c < nc; ++c) r2 = juce::jmax (r2, main.getRMSLevel (c, 0, n)); outRms.store (r2); }   // V10
}

juce::AudioProcessorEditor* TahorProcessor::createEditor() { return new TahorEditor (*this); }

//==============================================================================
void TahorProcessor::setParam (const juce::String& pid, float value)
{
    if (auto* p = apvts.getParameter (pid))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}
void TahorProcessor::beginGesture (const juce::String& pid) { if (auto* p = apvts.getParameter (pid)) p->beginChangeGesture(); }
void TahorProcessor::dragParam (const juce::String& pid, float value) { if (auto* p = apvts.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (value)); }
void TahorProcessor::endGesture (const juce::String& pid) { if (auto* p = apvts.getParameter (pid)) p->endChangeGesture(); }

int TahorProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String TahorProcessor::getProgramName (int index)
{
    const auto& l = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) l.size()) ? juce::String::fromUTF8 (l[(size_t) index].name) : juce::String();
}
void TahorProcessor::setCurrentProgram (int index) { loadFactoryPreset (index); }

void TahorProcessor::loadFactoryPreset (int index)
{
    const auto& l = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) l.size())) return;
    loadingPreset = true;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [pid, v] : l[(size_t) index].values)
        if (auto* p = apvts.getParameter (pid)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    loadingPreset = false;
    currentProgram = index;
    presetName = juce::String::fromUTF8 (l[(size_t) index].name);
    presetDirty = false;
}

juce::String TahorProcessor::factoryTag (int index)
{
    return factoryPresets()[(size_t) juce::jlimit (0, (int) factoryPresets().size() - 1, index)].tag;   // V11: each preset carries its folder
}

juce::String TahorProcessor::presetTagOf (const juce::File& f) const
{
    if (auto xml = juce::XmlDocument::parse (f)) return xml->getStringAttribute ("presetTag", "Mine");
    return "Mine";
}

bool TahorProcessor::saveUserPreset (const juce::String& name, const juce::String& tag)
{
    apvts.state.setProperty ("presetTag", tag, nullptr);
    return saveUserPreset (name);
}

juce::File TahorProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Presets/Mishkal Audio/Tahor");
}

juce::Array<juce::File> TahorProcessor::userPresets() const
{
    auto files = userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml");
    files.sort();
    return files;
}

bool TahorProcessor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto folder = userPresetFolder();
    folder.createDirectory();
    auto state = apvts.copyState();
    state.setProperty ("presetName", name.trim(), nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (folder.getChildFile (clean + ".xml"))) { presetName = name.trim(); presetDirty = false; return true; }
    return false;
}

bool TahorProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loadingPreset = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadingPreset = false;
            presetName = xml->getStringAttribute ("presetName", f.getFileNameWithoutExtension());
            presetDirty = false;
            return true;
        }
    return false;
}

void TahorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetDirty", presetDirty, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TahorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            loadingPreset = true;
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loadingPreset = false;
            presetName = xml->getStringAttribute ("presetName", presetName);
            presetDirty = xml->getBoolAttribute ("presetDirty", false);
        }
}

//==============================================================================
std::vector<float> TahorProcessor::snapshot() const
{
    std::vector<float> v;
    for (auto* p : getParameters()) v.push_back (p->getValue());
    return v;
}

void TahorProcessor::applySnapshot (const std::vector<float>& v)
{
    const auto& params = getParameters();
    for (int i = 0; i < juce::jmin ((int) v.size(), params.size()); ++i)
        if (std::abs (params[i]->getValue() - v[(size_t) i]) > 1.0e-6f)
        {
            params[i]->beginChangeGesture(); params[i]->setValueNotifyingHost (v[(size_t) i]); params[i]->endChangeGesture();
        }
    lastSeen = v; stableTicks = 0;
}

void TahorProcessor::recordHistory()
{
    auto now = snapshot();
    if (now != lastSeen) { lastSeen = now; stableTicks = 0; return; }
    if (++stableTicks != 7) return;
    if (historyPos >= 0 && history[(size_t) historyPos] == now) return;
    // V9: name the step - what changed since the last one
    juce::String label = "Start";
    if (historyPos >= 0)
    {
        const auto& prev = history[(size_t) historyPos];
        const auto& params = getParameters();
        int changed = 0, first = -1;
        for (int i = 0; i < juce::jmin ((int) prev.size(), (int) now.size()); ++i)
            if (std::abs (prev[(size_t) i] - now[(size_t) i]) > 1.0e-6f) { if (first < 0) first = i; ++changed; }
        if (changed > 8) label = "Preset / many changes";
        else if (first >= 0)
        {
            auto* p = params[first];
            label = p->getName (40) + "  " + p->getCurrentValueAsText() + (p->getLabel().isNotEmpty() ? " " + p->getLabel() : juce::String());
            if (changed > 1) label << "  (+" << (changed - 1) << ")";
        }
    }
    history.resize ((size_t) (historyPos + 1));
    historyLabel.removeRange (historyPos + 1, historyLabel.size());
    history.push_back (now);
    historyLabel.add (label);
    if (history.size() > 200) { history.erase (history.begin()); historyLabel.remove (0); }
    historyPos = (int) history.size() - 1;
}

void TahorProcessor::jumpHistory (int pos)
{
    if (pos < 0 || pos >= (int) history.size() || pos == historyPos) return;
    historyPos = pos;
    applySnapshot (history[(size_t) pos]);
    presetDirty = true;
}

void TahorProcessor::undo() { if (canUndo()) { applySnapshot (history[(size_t) --historyPos]); presetDirty = true; } }
void TahorProcessor::redo() { if (canRedo()) { applySnapshot (history[(size_t) ++historyPos]); presetDirty = true; } }

void TahorProcessor::switchAB (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abSlot) return;
    slots[abSlot] = apvts.copyState();
    slotNames[abSlot] = presetName + (presetDirty ? " *" : "");
    abSlot = slot;
    if (slots[slot].isValid())
    {
        loadingPreset = true;
        apvts.replaceState (slots[slot].createCopy());
        loadingPreset = false;
        presetName = slotNames[slot].trimCharactersAtEnd (" *");
        presetDirty = slotNames[slot].endsWith (" *");
    }
}

void TahorProcessor::timerCallback()
{
    if (! loadingPreset) recordHistory();
    // V9: AUTO GAIN - how much louder or quieter the EQs make things (weighted to where the ear lives), undone at the output
    {
        const double sr = juce::jmax (44100.0, preparedRate);
        const auto bands = tahorBands();
        const bool tOn = param ("tahorOn") > 0.5f, cOn = param ("charOn") > 0.5f;
        std::vector<vox::VocalEQ::Design> designs;
        if (cOn)
            for (int u : chain())
            {
                float v[unitValues]; getUnitValues (u, v);
                designs.push_back (vox::VocalEQ::design (unitSettings (u, v), units()[(size_t) u].type, sr));
            }
        double sum = 0.0, wsum = 0.0;
        for (int i = 0; i < 40; ++i)
        {
            const double hz = 30.0 * std::pow (14000.0 / 30.0, i / 39.0);
            double db = 0.0;
            if (tOn)
                for (auto& b : bands)
                    if (b.on && ! b.isCut()) db += (b.place >= 3 ? 0.5 : 1.0) * vox::tahorAnalogBandDb (b, b.freq, b.gain, b.q, hz);
            for (auto& d : designs) db += vox::VocalEQ::magnitudeDb (d, hz, sr);
            const double w = (hz >= 80.0 && hz <= 6000.0) ? 1.0 : 0.4;
            sum += w * db; wsum += w;
        }
        autoGainDb.store (juce::jlimit (-24.0f, 24.0f, (float) (-sum / juce::jmax (1.0e-9, wsum))));
    }
    const int lat = latencyNow();
    if (lat != reportedLatency) { reportedLatency = lat; setLatencySamples (lat); }
    // AUTO: listen for ~3 seconds, then set IN so the level sits around -18 dBFS RMS
    if (autoLearning.load() && learnCount > 0 && learnCount > (int) (3.0 * preparedRate))
    {
        const double ms = learnSum / learnCount;
        autoLearning.store (false);
        if (ms > 1.0e-9)
            setParam ("inGain", juce::jlimit (-24.0f, 24.0f, -18.0f - (float) (10.0 * std::log10 (ms))));
        learnSum = 0.0; learnCount = 0;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TahorProcessor(); }
