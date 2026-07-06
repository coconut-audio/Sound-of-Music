#pragma once

#include <JuceHeader.h>

static constexpr int maxBands = 8;

namespace ParamIDs
{
    inline String crush(int band) { return "crush" + String(band); }
    inline String downsample(int band) { return "downsample" + String(band); }
    inline String jitter(int band) { return "jitter" + String(band); }
    inline String clip(int band) { return "clip" + String(band); }
    inline String width(int band) { return "width" + String(band); }
    inline String postFilter(int band) { return "postFilter" + String(band); }
    inline String dividerFrequency(int index) { return "divider" + String(index); }
}

inline AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    for (int band = 0; band < maxBands; band++)
    {
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::crush(band), "Crush" + String(band), NormalisableRange<float>(2.0f, 32.0f, 1.0f, 0.5f), 32.0f));
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::downsample(band), "Downsample" + String(band), NormalisableRange<float>(440.0f, 44100.0f, 1.0f, 0.5f), 44100.0f));
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::jitter(band), "Jitter" + String(band), 0.0f, 100.0f, 0.0f));
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::clip(band), "Clip" + String(band), -15.0f, 0.0f, 0.0f));
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::width(band), "Width" + String(band), 0.0f, 200.0f, 100.0f));
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::postFilter(band), "PostFilter" + String(band), 0.0f, 100.0f, 100.0f));
    }

    for (int divider = 0; divider < maxBands - 1; divider++)
        layout.add(std::make_unique<AudioParameterFloat>(ParamIDs::dividerFrequency(divider), "Divider" + String(divider), 20.0f, 20000.0f, 500.0f));

    return layout;
}
