#pragma once

#include <JuceHeader.h>

class BitCrusher
{
public:
    void prepare(float crushParameter)
    {
        int bitCount = static_cast<int>(juce::jlimit(2.0f, 32.0f, crushParameter));
        crushLevel = std::pow(2.0f, static_cast<float>(bitCount));
    }

    float process(float inputSample) const
    {
        return std::round(inputSample * crushLevel) / crushLevel;
    }

    float getCrushLevel() const { return crushLevel; }

private:
    float crushLevel = 4.0f;
};
