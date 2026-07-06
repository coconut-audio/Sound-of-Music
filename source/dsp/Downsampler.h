#pragma once

#include <JuceHeader.h>

class Downsampler
{
public:
    void prepare(double sampleRate, float downsampleParameter)
    {
        int targetRate = jmax(440, static_cast<int>(downsampleParameter));
        decimationStep = jmax(1, static_cast<int>(sampleRate / targetRate));
    }

    float process(float input)
    {
        counter++;
        if (counter >= decimationStep)
        {
            counter = 0;
            heldSample = input;
        }
        return heldSample;
    }

    int getDecimationStep() const { return decimationStep; }

private:
    int decimationStep = 1;
    int counter = 0;
    float heldSample = 0.0f;
};
