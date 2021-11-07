#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::Colour randCol(juce::Random& rand) {
    return juce::Colour(0xffff0000).withRotatedHue(rand.nextFloat());
}

juce::Colour randCol(juce::Colour last, juce::Random& rand, float t) {
    return last.withRotatedHue((rand.nextFloat() - .5f) * t);
}

juce::Colour getDominantCol(const juce::Image& img) {
    float r = 0.f, g = 0.f, b = 0.f;
    for(auto y = 0; y < img.getHeight(); ++y)
        for (auto x = 0; x < img.getWidth(); ++x) {
            auto pxl = img.getPixelAt(x, y);
            r += pxl.getFloatRed();
            g += pxl.getFloatGreen();
            b += pxl.getFloatBlue();
        }
    auto gain = 1.f / float(img.getWidth() * img.getHeight());
    r *= gain;
    g *= gain;
    b *= gain;
    return juce::Colour(
        juce::uint8(r * 128.f),
        juce::uint8(g * 128.f),
        juce::uint8(b * 128.f)
    );
}


SusquashAudioProcessorEditor::SusquashAudioProcessorEditor (SusquashAudioProcessor& _p) :
    AudioProcessorEditor(&_p),
    p(_p),
    bg(),
    mainCol(),
    subTitle("~ inspired by dan worrall ~", "~ inspired by dan worrall ~"),
    squash(p.apvts, param::ID::Squash, "SUSQUASH"),
    gain(p.apvts, param::ID::Gain, "GAIN")
{
    auto state = p.apvts.state;
    
    const auto width = static_cast<int>(state.getProperty("width", 339));
    const auto height = static_cast<int>(state.getProperty("height", 431));

    bg = juce::Image(juce::Image::ARGB, width, height, true);
    {
        juce::Graphics g{ bg };
        juce::Random rand;
        const auto numFlames = 12.f + rand.nextFloat() * 12.f;
        const auto lineLen = 2.f + rand.nextFloat() * 12.f;
        auto col = randCol(rand);
        for (auto i = 0; i < numFlames; ++i) {
            auto x = rand.nextFloat() * width;
            auto y = 1.f * height;
            col = randCol(col, rand, .1f);
            while (x > 0 && x < width && y > 0) {
                auto yRel = y / height;
                auto angle = rand.nextFloat() * pi - pi * .5f;
                auto line = juce::Line<float>::fromStartAndAngle({ x,y }, lineLen, angle);
                col = randCol(col, rand, .1f);
                g.setColour(col);
                g.drawRect(line.getEndX(), line.getEndY(), x, y);
                x = line.getEndX();
                y = line.getEndY();
            }
        }
        auto tmpImg = bg.createCopy();
        for (auto y = 0; y < height; ++y) {
            const auto yRel = 1.f * y / height;
            const auto ySide = std::sqrt(std::sqrt(std::abs(2.f * yRel - 1.f)));
            for (auto x = 0; x < width; ++x) {
                const auto xRel = 1.f * x / width;
                const auto xSide = std::sqrt(std::sqrt(std::abs(2.f * xRel - 1.f)));
                tmpImg.setPixelAt(x, y, bg.getPixelAt(xSide * width, ySide * height).darker(ySide * xSide * .4f).withMultipliedAlpha(.8f));
            }
        }
        bg = tmpImg;
        mainCol = getDominantCol(bg);
        squash.mainCol = mainCol;
        gain.mainCol = mainCol;
    }

    addAndMakeVisible(subTitle);
    subTitle.setFont(juce::Typeface::createSystemTypefaceFor(BinaryData::nel19_ttf, BinaryData::nel19_ttfSize));
    subTitle.setColour(juce::Label::ColourIds::textColourId, mainCol.contrasting());
    subTitle.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(squash);
    addAndMakeVisible(gain);

    setResizable(true, true);
    setOpaque(true);
    setSize(width, height);
}

void SusquashAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(mainCol.contrasting(.5f));
    g.drawImageAt(bg, 0, 0, false);
}

void SusquashAudioProcessorEditor::resized()
{
    bg = bg.rescaled(getWidth(), getHeight(), juce::Graphics::ResamplingQuality::lowResamplingQuality);

    squash.setBounds(getLocalBounds());
    auto gainY = (int)(getHeight() * .8f);
    gain.setBounds(0, gainY, getWidth(), getHeight() - gainY);

    subTitle.setBounds(0, gain.getHeight() * 3 / 9, getWidth(), gain.getHeight());

    auto state = p.apvts.state;
    state.setProperty("width", getWidth(), nullptr);
    state.setProperty("height", getHeight(), nullptr);
}
