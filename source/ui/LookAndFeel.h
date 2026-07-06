#pragma once

#include <JuceHeader.h>
#include "../common/Colours.h"

class SliderLookAndFeel final : public LookAndFeel_V4
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPosition, float rotaryStartAngle, float rotaryEndAngle,
                          Slider&) override
    {
        float diameter = jmin(width, height);
        float radius = diameter / 2.0f;
        float centerX = x + width / 2.0f;
        float centerY = y + height / 2.0f;
        float arcWidth = 3.0f;
        float currentAngle = rotaryStartAngle + sliderPosition * (rotaryEndAngle - rotaryStartAngle);

        Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, radius - arcWidth, radius - arcWidth, 0.0f,
                                    rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(Theme::separator);
        g.strokePath(backgroundArc, PathStrokeType(arcWidth, PathStrokeType::curved, PathStrokeType::rounded));

        if (sliderPosition > 0.001f)
        {
            Path filledArc;
            filledArc.addCentredArc(centerX, centerY, radius - arcWidth, radius - arcWidth, 0.0f,
                                    rotaryStartAngle, currentAngle, true);
            g.setColour(Theme::accent);
            g.strokePath(filledArc, PathStrokeType(arcWidth, PathStrokeType::curved, PathStrokeType::rounded));
        }

        float needleLength = radius - arcWidth - 4.0f;
        Path needle;
        needle.addRectangle(-1.0f, -needleLength, 2.0f, needleLength);
        needle.addEllipse(-3.0f, -3.0f, 6.0f, 6.0f);
        g.setColour(Theme::text);
        g.fillPath(needle, AffineTransform::rotation(currentAngle).translated(centerX, centerY));
    }
};
