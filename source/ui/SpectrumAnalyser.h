#pragma once

#include <JuceHeader.h>
#include "../common/Colours.h"

class SpectrumAnalyser : public Component
{
public:
    SpectrumAnalyser() = default;
    ~SpectrumAnalyser() override = default;

    void paint(Graphics&) override;
    void resized() override;

    void updateSpectra(const float* dryData, const float* wetData, float dataSize,
                       const std::vector<float>& dividerFrequencies, int selectedBandIndex);

    float frequencyToX(float frequency) const;
    float xToFrequency(float xCoordinate) const;

    static constexpr float minimumDecibels = -60.0f;
    static constexpr float maximumDecibels = 36.0f;
    static constexpr float minimumFrequency = 20.0f;
    static constexpr float maximumFrequency = 20000.0f;

private:
    void applySavgolFilter(float* data, int dataSize);
    bool hasSignal(const float* data) const;

    Rectangle<float> componentBounds;
    ColourGradient spectrumGradient;

    static constexpr int scopeSize = 512;
    float dryScopeData[scopeSize] = {};
    float wetScopeData[scopeSize] = {};
    float drySmoothedData[scopeSize] = {};
    float wetSmoothedData[scopeSize] = {};

    static constexpr int savgolWindowSize = 5;
    static constexpr int savgolHalfWindowSize = savgolWindowSize / 2;
    static constexpr float savgolNormalization = 1.0f / 35.0f;
    static constexpr float savgolCoefficient[savgolWindowSize] = {
        -3.0f * savgolNormalization, 12.0f * savgolNormalization, 17.0f * savgolNormalization,
        12.0f * savgolNormalization, -3.0f * savgolNormalization
    };

    std::vector<int> gridFrequencyValues = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    static constexpr float lineWidth = 2.0f;

    std::vector<float> currentDividerFrequencies;
    int currentSelectedBandIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyser)
};
