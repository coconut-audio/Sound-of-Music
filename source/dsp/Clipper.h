#pragma once

#include <JuceHeader.h>

class Clipper
{
public:
    void prepare(float clipParameter)
    {
        clippingThreshold = Decibels::decibelsToGain(clipParameter);
    }

    float process(float inputSample) const
    {
        if (clippingThreshold >= 0.999f)
            return inputSample;

        if (inputSample > clippingThreshold)  return clippingThreshold;
        if (inputSample < -clippingThreshold) return -clippingThreshold;
        return inputSample;
    }

    bool isActive() const { return clippingThreshold < 0.999f; }

private:
    float clippingThreshold = 1.0f;
};
