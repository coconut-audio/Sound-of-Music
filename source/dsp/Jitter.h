#pragma once

#include <JuceHeader.h>

class Jitter
{
public:
    void prepare(float jitterParameter)
    {
        noiseAmount = jitterParameter / 100.0f;
        crackleProbability = static_cast<int>(jitterParameter);
    }

    float process(float inputSample, float drySample)
    {
        float outputSample = inputSample + static_cast<float>(randomGenerator.nextInt(3) - 1) * noiseAmount * drySample;

        if (crackleProbability > 0 && randomGenerator.nextInt(100 - crackleProbability + 2) == 0 && randomGenerator.nextInt(10) != 0)
            outputSample = 0.0f;

        return outputSample;
    }

    bool isActive() const { return noiseAmount > 0.0f; }

private:
    Random randomGenerator;
    float noiseAmount = 0.0f;
    int crackleProbability = 0;
};
