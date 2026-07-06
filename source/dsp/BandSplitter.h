#pragma once

#include <JuceHeader.h>
#include "../common/Parameters.h"

class BandSplitter
{
public:
    void prepare(double sampleRate, const std::vector<float>& dividerFrequencies)
    {
        numberOfDividers = jmin(static_cast<int>(dividerFrequencies.size()), maximumDividers);

        for (int divider = 0; divider < numberOfDividers; divider++)
        {
            double frequency = static_cast<double>(dividerFrequencies[static_cast<size_t>(divider)]);

            IIRCoefficients lowPass = IIRCoefficients::makeLowPass(sampleRate, frequency);
            IIRCoefficients highPass = IIRCoefficients::makeHighPass(sampleRate, frequency);

            for (int pass = 0; pass < 2; pass++)
            {
                splitLowPass[divider][0][pass].setCoefficients(lowPass);
                splitLowPass[divider][1][pass].setCoefficients(lowPass);
                splitHighPass[divider][0][pass].setCoefficients(highPass);
                splitHighPass[divider][1][pass].setCoefficients(highPass);

                postLowPass[divider][0][pass].setCoefficients(lowPass);
                postLowPass[divider][1][pass].setCoefficients(lowPass);
                postHighPass[divider][0][pass].setCoefficients(highPass);
                postHighPass[divider][1][pass].setCoefficients(highPass);
            }
        }
    }

    void process(AudioSampleBuffer& input, std::vector<AudioSampleBuffer>& bands, int numberOfSamples)
    {
        if (numberOfDividers == 0)
        {
            bands[0].makeCopyOf(input);
            return;
        }

        AudioSampleBuffer tempBuffer(2, numberOfSamples);
        tempBuffer.makeCopyOf(input);

        for (int divider = 0; divider < numberOfDividers; divider++)
        {
            auto index = static_cast<size_t>(divider);

            bands[index].makeCopyOf(tempBuffer);

            for (int pass = 0; pass < 2; pass++)
            {
                splitLowPass[divider][0][pass].processSamples(bands[index].getWritePointer(0), numberOfSamples);
                splitLowPass[divider][1][pass].processSamples(bands[index].getWritePointer(1), numberOfSamples);
                splitHighPass[divider][0][pass].processSamples(tempBuffer.getWritePointer(0), numberOfSamples);
                splitHighPass[divider][1][pass].processSamples(tempBuffer.getWritePointer(1), numberOfSamples);
            }
        }

        bands[static_cast<size_t>(numberOfDividers)].makeCopyOf(tempBuffer);
    }

    void filterBand(AudioSampleBuffer& band, int bandIndex, int numberOfSamples)
    {
        if (numberOfDividers == 0)
            return;

        for (int divider = 0; divider < numberOfDividers; divider++)
        {
            if (bandIndex <= divider)
            {
                for (int pass = 0; pass < 2; pass++)
                {
                    postLowPass[divider][0][pass].processSamples(band.getWritePointer(0), numberOfSamples);
                    postLowPass[divider][1][pass].processSamples(band.getWritePointer(1), numberOfSamples);
                }
            }
            if (bandIndex >= divider + 1)
            {
                for (int pass = 0; pass < 2; pass++)
                {
                    postHighPass[divider][0][pass].processSamples(band.getWritePointer(0), numberOfSamples);
                    postHighPass[divider][1][pass].processSamples(band.getWritePointer(1), numberOfSamples);
                }
            }
        }
    }

    int getNumberOfBands() const { return numberOfDividers + 1; }

private:
    static constexpr int maximumDividers = maxBands - 1;
    int numberOfDividers = 0;

    IIRFilter splitLowPass[maximumDividers][2][2], splitHighPass[maximumDividers][2][2];
    IIRFilter postLowPass[maximumDividers][2][2], postHighPass[maximumDividers][2][2];
};
