#pragma once

static constexpr float pi = 3.14159265359f;
static constexpr float tau = pi * 2.f;

namespace param {
	enum class ID { Squash, Gain };

	// PARAMETER ID STUFF
	static juce::String getName(ID i) {
		switch (i) {
		case ID::Squash: return "Squash";
		case ID::Gain: return "Gain";
		default: return "";
		}
	}
	static juce::String getName(int i) { getName(static_cast<ID>(i)); }
	static juce::String getID(const ID i) { return getName(i).toLowerCase().removeCharacters(" "); }
	static juce::String getID(const int i) { return getName(i).toLowerCase().removeCharacters(" "); }

	namespace makeRange
	{
		inline juce::NormalisableRange<float> biased(float start, float end, float bias)
		{
			if (bias > 0.f)
				return {
					start, end,
					[b = 1.f - bias, range = end - start](float min, float max, float normalized) {
						return min + range * std::pow(normalized, b);
					},
					[b = 1.f / (1.f - bias), rangeInv = 1.f / (end - start)](float min, float max, float denormalized) {
						return std::pow((denormalized - min) * rangeInv, b);
					},
					nullptr
			};
			else
				return {
					start, end,
					[b = 1.f / (bias + 1.f), range = end - start](float min, float max, float normalized) {
						return min + range * std::pow(normalized, b);
					},
					[b = bias + 1, rangeInv = 1.f / (end - start)](float min, float max, float denormalized) {
						return std::pow((denormalized - min) * rangeInv, b);
					},
					nullptr
			};
		}
	}

	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue,
		std::function<juce::String(float value, int maxLen)> stringFromValue,
		const juce::NormalisableRange<float>& range) {
		return std::make_unique<juce::AudioParameterFloat>(
			getID(i), getName(i), range, defaultValue, getName(i), juce::AudioProcessorParameter::Category::genericParameter,
			stringFromValue
			);
	}
	static std::unique_ptr<juce::AudioParameterFloat> createParameter(ID i, float defaultValue = 1.f,
		std::function<juce::String(float value, int maxLen)> stringFromValue = nullptr,
		const float min = 0.f, const float max = 1.f, const float interval = -1.f) {
		if (interval != -1.f)
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max, interval));
		else
			return createParameter(i, defaultValue, stringFromValue, juce::NormalisableRange<float>(min, max));
	}

	
	static juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
		std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

		const auto percStr = [](float v,int) { return juce::String(std::floor(v * 100.f)) + " %"; };
		const auto dbStr = [](float v,int) { return juce::String(std::floor(v * 100.f) * .01f) + " db"; };

		parameters.push_back(createParameter(ID::Squash, 100.f, percStr, makeRange::biased(0.f, 100.f, -.6f)));
		parameters.push_back(createParameter(ID::Gain,   0.f,   dbStr,   makeRange::biased(-40.f, 0.f, 0.f)));
		
		return { parameters.begin(), parameters.end() };
	}
};

inline juce::Rectangle<float> maxQuadIn(const juce::Rectangle<float>& b) noexcept {
    const auto minDimen = std::min(b.getWidth(), b.getHeight());
    const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
    const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
    return { x, y, minDimen, minDimen };
}
inline void repaintWithChildren(juce::Component* comp) {
    comp->repaint();
    for (auto c = 0; c < comp->getNumChildComponents(); ++c)
        comp->getChildComponent(c)->repaint();
}

struct Comp :
    public juce::Component
{
    Comp()
    {}
};

class Knob :
    public Comp
{
    static constexpr float StartAngle = -pi * .25f * 3.f;
    static constexpr float EndAngle = pi * .25f * 3.f;
    static constexpr float SensitiveDrag = .2f;
    static constexpr float WheelDefaultSpeed = .02f;
    static constexpr float WheelInertia = .9f;
    static constexpr float thicc = 2.f, thicc2 = thicc * 2.f;

    struct Dial :
        public Comp
    {
        Dial(Knob& _knob) :
            Comp(),
            knob(_knob)
        {
            setInterceptsMouseClicks(false, false);
        }
    protected:
        const Knob& knob;
        void paint(juce::Graphics& g) override
        {
            const auto width = static_cast<float>(getWidth());
            const auto height = static_cast<float>(getHeight());
            const auto value = knob.rap.getValue();
            juce::PathStrokeType strokeType(thicc, juce::PathStrokeType::JointStyle::curved, juce::PathStrokeType::EndCapStyle::rounded);
            const juce::Point<float> centre(width * .5f, height * .5f);
            const auto radius = std::min(centre.x, centre.y) - thicc;
            const auto angleRange = EndAngle - StartAngle;
            const auto valueAngle = StartAngle + angleRange * value;

            g.setColour(knob.mainCol);
            juce::Path pathNorm;
            pathNorm.addCentredArc(centre.x, centre.y, radius, radius,
                0.f, StartAngle, EndAngle,
                true
            );
            const auto innerRad = radius - thicc2;
            pathNorm.addCentredArc(centre.x, centre.y, innerRad, innerRad,
                0.f, StartAngle, EndAngle,
                true
            );
            g.strokePath(pathNorm, strokeType);

            g.setColour(knob.mainCol);
            for (auto r = 0; r <= 24.f; ++r) {
                auto rr = 1.f * r / 24.f;
                const auto vLine = juce::Line<float>::fromStartAndAngle(centre, radius + 1.f, StartAngle + angleRange * value * rr);
                auto x = vLine.getStartX();
                auto y = vLine.getStartX();
                auto endX = vLine.getEndX();
                auto endY = vLine.getEndY();
                if (x > endX) std::swap(x, endX);
                if (y > endY) std::swap(y, endY);
                auto w = endX - x;
                auto h = endY - y;
                g.drawRect(x,y,w,h, 3.f);
            }
        }
    };

    struct Label :
        public Comp
    {
        Label(const juce::String& _name) :
            Comp()
        {
            setName(_name);
        }
    protected:
        void paint(juce::Graphics& g) override {
            g.setColour(juce::Colours::white);
            g.setFont(juce::Typeface::createSystemTypefaceFor(BinaryData::nel19_ttf, BinaryData::nel19_ttfSize));
            g.drawFittedText(getName(), getLocalBounds(), juce::Justification::centred, 1);
        }
    };
public:
    Knob(juce::AudioProcessorValueTreeState& apvts, param::ID _pID, const juce::String& _name) :
        rap(*apvts.getParameter(param::getID(_pID))),
        attach(rap, [this](float) { repaint(); }, nullptr),
        dial(*this),
        label(_name),
        dragY(0.f),
        scrollSpeed(0.f)
    {
        addAndMakeVisible(dial);
        addAndMakeVisible(label);
        attach.sendInitialUpdate();
    }
    juce::Colour mainCol;
protected:
    juce::RangedAudioParameter& rap;
    juce::ParameterAttachment attach;
    Dial dial;
    Label label;
    float dragY, scrollSpeed;

    void mouseDown(const juce::MouseEvent& evt) override {
        attach.beginGesture();
        dragY = evt.position.y;
    }
    void mouseDrag(const juce::MouseEvent& evt) override {
        const auto dragOffset = evt.position.y - dragY;
        auto dragOffNorm = dragOffset / static_cast<float>(getHeight());
        if (evt.mods.isShiftDown())
            dragOffNorm *= SensitiveDrag;
        const auto newValue = juce::jlimit(0.f, 1.f, rap.getValue() - dragOffNorm);
        attach.setValueAsPartOfGesture(rap.convertFrom0to1(newValue));
        dragY = evt.position.y;
    }
    void mouseUp(const juce::MouseEvent& evt) override {
        if (!evt.mouseWasDraggedSinceMouseDown()) {
            if (evt.mods.isCtrlDown())
                this->attach.setValueAsPartOfGesture(rap.getDefaultValue());
            else {
                juce::Point<float>centre(
                    static_cast<float>(dial.getWidth()) * .5f,
                    static_cast<float>(dial.getHeight()) * .5f
                );
                const juce::Line<float> fromCentre(centre, evt.position);
                const auto angle = fromCentre.getAngle();
                const auto angleRange = EndAngle - StartAngle;

                const auto newValue = juce::jlimit(0.f, 1.f, (angle - StartAngle) / angleRange);
                this->attach.setValueAsPartOfGesture(rap.convertFrom0to1(newValue));
            }
        }
        attach.endGesture();
    }
    void mouseWheelMove(const juce::MouseEvent& evt, const juce::MouseWheelDetails& wheel) override {
        if (evt.mods.isAnyMouseButtonDown()) return;
        const bool reversed = wheel.isReversed ? -1.f : 1.f;
        const bool isTrackPad = wheel.deltaY * wheel.deltaY < .0549316f;
        if (isTrackPad)
            dragY = reversed * wheel.deltaY;
        else {
            const auto deltaYPos = wheel.deltaY > 0.f ? 1.f : -1.f;
            dragY = reversed * WheelDefaultSpeed * deltaYPos;
        }
        if (evt.mods.isShiftDown())
            dragY *= SensitiveDrag;
        const auto newValue = juce::jlimit(0.f, 1.f, rap.getValue() + dragY);
        attach.setValueAsCompleteGesture(rap.convertFrom0to1(newValue));
    }

private:
    void resized() override {
        auto bounds = getLocalBounds().toFloat();
        {
            const auto x = 0.f;
            const auto y = 0.f;
            const auto w = bounds.getWidth();
            const auto h = bounds.getHeight() * .2f;
            const juce::Rectangle<float> labelBounds(x, y, w, h);
            label.setBounds(labelBounds.toNearestInt());
            bounds.setY(h);
            bounds.setHeight(bounds.getHeight() - h);
        }
        const auto dialBounds = maxQuadIn(bounds);
        dial.setBounds(dialBounds.toNearestInt());
        /*
        if (modulatable) {
            const auto width = static_cast<float>(dialBounds.getWidth());
            const auto height = static_cast<float>(dialBounds.getHeight());
            const auto w = width / nelG::Pi - nelG::Thicc;
            const auto h = height / nelG::Pi - nelG::Thicc;
            const auto x = dialBounds.getX() + (width - w) * .5f;
            const auto y = dialBounds.getY() + (height - h);
            const juce::Rectangle<float> dialArea(x, y, w, h);
            modulatorDial.setBounds(dialArea.toNearestInt());
        }
        */
    }

    std::function<void(float)> onParamChange(bool _modulatable) {
        if (_modulatable)
            return [](float) {};
        return [this](float) { repaint(); };
    }
};