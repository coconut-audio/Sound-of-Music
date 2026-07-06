#include "SpectrumAnalyser.h"

float SpectrumAnalyser::frequencyToX(float frequency) const
{
    float proportion = log2(frequency / minimumFrequency) / log2(maximumFrequency / minimumFrequency);
    return jmap<float>(proportion, componentBounds.getX(), componentBounds.getRight());
}

float SpectrumAnalyser::xToFrequency(float xCoordinate) const
{
    float proportion = jmap<float>(xCoordinate, componentBounds.getX(), componentBounds.getRight(), 0.0f, 1.0f);
    return minimumFrequency * pow(maximumFrequency / minimumFrequency, proportion);
}

void SpectrumAnalyser::paint(Graphics& g)
{
    g.setColour(Theme::surface);
    g.fillRect(componentBounds);

    for (int decibelValue = static_cast<int>(minimumDecibels) + 12; decibelValue < static_cast<int>(maximumDecibels); decibelValue += 12)
    {
        float yPosition = jmap<float>(static_cast<float>(decibelValue), minimumDecibels, maximumDecibels, componentBounds.getBottom(), componentBounds.getY());
        g.setColour(decibelValue == 0 ? Theme::gridMajor() : Theme::gridMinor());
        g.drawLine(componentBounds.getX(), yPosition, componentBounds.getRight(), yPosition, lineWidth);

        String decibelLabel = (decibelValue >= 0 ? "+" : "") + String(decibelValue) + " dB";
        g.setColour(Colour(0x80FFFFFF));
        g.setFont(11.0f);
        g.drawText(decibelLabel, Rectangle<int>(static_cast<int>(componentBounds.getX()) + 2, static_cast<int>(yPosition) - 8, 40, 16),
                   Justification::centredLeft, false);
    }

    for (int frequencyValue : gridFrequencyValues)
    {
        float xPosition = frequencyToX(static_cast<float>(frequencyValue));
        g.setColour(Theme::gridMinor());
        g.drawLine(xPosition, componentBounds.getY(), xPosition, componentBounds.getBottom(), lineWidth);
    }

    g.saveState();
    g.reduceClipRegion(componentBounds.toNearestInt());

    if (hasSignal(drySmoothedData))
    {
        Path drySpectrumPath;
        for (int index = 0; index < scopeSize - 1; ++index)
        {
            float proportion = static_cast<float>(index) / static_cast<float>(scopeSize - 1);
            float frequency = minimumFrequency * pow(maximumFrequency / minimumFrequency, proportion);
            float xPosition = frequencyToX(frequency);
            float yPosition = jmap<float>(drySmoothedData[index], minimumDecibels, maximumDecibels, componentBounds.getBottom(), componentBounds.getY());
            index == 0 ? drySpectrumPath.startNewSubPath(xPosition, yPosition) : drySpectrumPath.lineTo(xPosition, yPosition);
        }

        g.setColour(Theme::text);
        g.strokePath(drySpectrumPath, PathStrokeType(lineWidth));

        drySpectrumPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
        drySpectrumPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
        drySpectrumPath.closeSubPath();

        g.setColour(Theme::text.withAlpha(0.15f));
        g.fillPath(drySpectrumPath);
    }

    if (hasSignal(wetSmoothedData))
    {
        Path wetSpectrumPath;
        for (int index = 0; index < scopeSize - 1; ++index)
        {
            float proportion = static_cast<float>(index) / static_cast<float>(scopeSize - 1);
            float frequency = minimumFrequency * pow(maximumFrequency / minimumFrequency, proportion);
            float xPosition = frequencyToX(frequency);
            float yPosition = jmap<float>(wetSmoothedData[index], minimumDecibels, maximumDecibels, componentBounds.getBottom(), componentBounds.getY());
            index == 0 ? wetSpectrumPath.startNewSubPath(xPosition, yPosition) : wetSpectrumPath.lineTo(xPosition, yPosition);
        }

        g.setColour(Theme::accent);
        g.strokePath(wetSpectrumPath, PathStrokeType(lineWidth));

        wetSpectrumPath.lineTo(componentBounds.getRight(), componentBounds.getBottom());
        wetSpectrumPath.lineTo(componentBounds.getX(), componentBounds.getBottom());
        wetSpectrumPath.closeSubPath();

        g.setGradientFill(spectrumGradient);
        g.fillPath(wetSpectrumPath);
    }

    for (int frequencyValue : gridFrequencyValues)
    {
        float xPosition = frequencyToX(static_cast<float>(frequencyValue));
        g.setColour(Colour(0xB0FFFFFF));
        g.setFont(13.0f);

        String frequencyLabel;
        if (frequencyValue >= 1000)
            frequencyLabel = String(frequencyValue / 1000) + "k";
        else
            frequencyLabel = String(frequencyValue);

        g.drawFittedText(frequencyLabel, Rectangle<int>(static_cast<int>(xPosition) - 25, static_cast<int>(componentBounds.getBottom()) - 20, 50, 20),
                         Justification::centred, 1);
    }

    g.restoreState();

    for (size_t divider = 0; divider < currentDividerFrequencies.size(); divider++)
    {
        float centerX = frequencyToX(currentDividerFrequencies[divider]);

        ColourGradient dividerGradient(Theme::accent.withAlpha(0.0f), centerX, componentBounds.getY(),
                                       Theme::accent.withAlpha(0.0f), centerX, componentBounds.getBottom(), false);
        dividerGradient.addColour(0.5f, Theme::accent);

        Path dividerPath;
        dividerPath.startNewSubPath(centerX, componentBounds.getY());
        dividerPath.lineTo(centerX, componentBounds.getBottom());

        g.setGradientFill(dividerGradient);
        g.strokePath(dividerPath, PathStrokeType(2.0f));

        float frequency = currentDividerFrequencies[divider];
        String frequencyLabel;
        if (frequency >= 1000.0f)
            frequencyLabel = String(frequency / 1000.0f, 1) + "k";
        else
            frequencyLabel = String(static_cast<int>(frequency));

        g.setColour(Colour(0xB0FFFFFF));
        g.setFont(13.0f);
        g.drawFittedText(frequencyLabel, Rectangle<int>(static_cast<int>(centerX) - 25, static_cast<int>(componentBounds.getBottom()) + 10, 50, 18),
                         Justification::centred, 1);
    }

    int numberOfBands = static_cast<int>(currentDividerFrequencies.size()) + 1;
    for (int band = 0; band < numberOfBands; band++)
    {
        float leftEdge = (band == 0) ? componentBounds.getX() : frequencyToX(currentDividerFrequencies[static_cast<size_t>(band - 1)]);
        float rightEdge = (band == numberOfBands - 1) ? componentBounds.getRight() : frequencyToX(currentDividerFrequencies[static_cast<size_t>(band)]);

        if (band == currentSelectedBandIndex)
        {
            g.setColour(Theme::bandHighlight);
            g.fillRect(leftEdge, componentBounds.getY(), rightEdge - leftEdge, componentBounds.getHeight());
        }
    }
}

void SpectrumAnalyser::resized()
{
    componentBounds = getLocalBounds().toFloat();

    float zeroLevel = jmap<float>(0.0f, minimumDecibels, maximumDecibels, componentBounds.getBottom(), componentBounds.getY());
    spectrumGradient = ColourGradient(Theme::accent.withAlpha(0.67f), componentBounds.getX(), componentBounds.getBottom(),
                                      Theme::accent.withAlpha(0.27f), componentBounds.getX(), zeroLevel, false);
}

void SpectrumAnalyser::applySavgolFilter(float* data, int dataSize)
{
    for (int index = 0; index < dataSize; ++index)
    {
        float sum = 0.0f;
        for (int offset = -savgolHalfWindowSize; offset <= savgolHalfWindowSize; ++offset)
        {
            int sampleIndex = jlimit(0, dataSize - 1, index + offset);
            sum += data[sampleIndex] * savgolCoefficient[offset + savgolHalfWindowSize];
        }
        data[index] = sum;
    }
}

bool SpectrumAnalyser::hasSignal(const float* data) const
{
    for (int index = 0; index < scopeSize; ++index)
        if (data[index] > minimumDecibels + 3.0f)
            return true;
    return false;
}

void SpectrumAnalyser::updateSpectra(const float* dryFFTData, const float* wetFFTData, float dataSize,
                                     const std::vector<float>& dividerFrequencies, int selectedBandIndex)
{
    currentDividerFrequencies = dividerFrequencies;
    currentSelectedBandIndex = selectedBandIndex;

    for (int index = 0; index < scopeSize; ++index)
    {
        float proportion = static_cast<float>(index) / static_cast<float>(scopeSize - 1);
        float frequency = minimumFrequency * pow(maximumFrequency / minimumFrequency, proportion);
        int fftDataIndex = jlimit(0, static_cast<int>(dataSize / 2.0f - 1.0f),
                                  static_cast<int>(proportion * dataSize / 2.0f));

        float pinkCorrection = 3.0f * std::log2(frequency / minimumFrequency);

        float dryLevel = Decibels::gainToDecibels(dryFFTData[fftDataIndex])
                       - Decibels::gainToDecibels(dataSize)
                       + Decibels::gainToDecibels(512.0f)
                       + pinkCorrection;
        float wetLevel = Decibels::gainToDecibels(wetFFTData[fftDataIndex])
                       - Decibels::gainToDecibels(dataSize)
                       + Decibels::gainToDecibels(512.0f)
                       + pinkCorrection;

        dryScopeData[index] = 0.5f * jlimit(minimumDecibels, maximumDecibels, dryLevel) + 0.5f * dryScopeData[index];
        wetScopeData[index] = 0.5f * jlimit(minimumDecibels, maximumDecibels, wetLevel) + 0.5f * wetScopeData[index];
    }

    memcpy(drySmoothedData, dryScopeData, sizeof(drySmoothedData));
    memcpy(wetSmoothedData, wetScopeData, sizeof(wetSmoothedData));
    applySavgolFilter(drySmoothedData, scopeSize);
    applySavgolFilter(wetSmoothedData, scopeSize);

    repaint();
}
