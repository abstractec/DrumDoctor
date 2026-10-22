#include "ViolationListComponent.h"
#include "Palette.h"

ViolationListComponent::ViolationListComponent()
{
    header.setText ("Problems", juce::dontSendNotification);
    header.setFont (drumdoctor::palette::font (12.0f, true));
    header.setColour (juce::Label::textColourId, drumdoctor::palette::muted());
    header.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (header);

    emptyLabel.setText ("They will be listed here.", juce::dontSendNotification);
    emptyLabel.setFont (drumdoctor::palette::font (12.0f));
    emptyLabel.setColour (juce::Label::textColourId, juce::Colour (0xff6f6558));
    emptyLabel.setJustificationType (juce::Justification::centred);
    emptyLabel.setInterceptsMouseClicks (false, false);
    emptyLabel.setVisible (false);
    addAndMakeVisible (emptyLabel);

    list.setModel (this);
    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, drumdoctor::palette::panel());
    list.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (list);
}

void ViolationListComponent::setPluginStyle (bool shouldUsePluginStyle)
{
    pluginStyle = shouldUsePluginStyle;
    header.setText (pluginStyle ? "CAN'T BE PLAYED" : "Problems", juce::dontSendNotification);
    header.setColour (juce::Label::textColourId, pluginStyle ? juce::Colour (0xff8d8272) : drumdoctor::palette::muted());
    list.setColour (juce::ListBox::backgroundColourId, pluginStyle ? juce::Colour (0xff12100e) : drumdoctor::palette::panel());
    list.setRowHeight (pluginStyle ? 26 : 24);
    emptyLabel.setVisible (pluginStyle && violations.empty());
    if (emptyLabel.isVisible())
        emptyLabel.toFront (false);
    repaint();
}

ViolationListComponent::~ViolationListComponent()
{
    list.setModel (nullptr);
}

void ViolationListComponent::setEmptyMessage (const juce::String& text)
{
    if (emptyLabel.getText() == text)
        return;

    emptyLabel.setText (text, juce::dontSendNotification);
}

void ViolationListComponent::setViolations (const std::vector<drumdoctor::Violation>& next)
{
    violations = next;
    list.updateContent();
    list.deselectAllRows();
    emptyLabel.setVisible (pluginStyle && violations.empty());
    if (emptyLabel.isVisible())
        emptyLabel.toFront (false);
    repaint();
}

int ViolationListComponent::getNumRows()
{
    return static_cast<int> (violations.size());
}

void ViolationListComponent::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (row < 0 || row >= getNumRows())
        return;

    auto textColour = selected ? drumdoctor::palette::ink() : drumdoctor::palette::muted();
    auto textLeft = 8;

    if (pluginStyle)
    {
        g.setColour (selected ? juce::Colour (0xff3a2a16) : juce::Colour (0xff12100e));
        g.fillRect (0, 0, width, height);
        g.setColour (juce::Colour (0xff2a241c));
        g.drawHorizontalLine (height - 1, 0.0f, static_cast<float> (width));

        const auto led = juce::Rectangle<float> (8.0f, static_cast<float> (height) * 0.5f - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colour (0xffe23b2f));
        g.fillEllipse (led);
        textColour = selected ? juce::Colour (0xfff3ead8) : juce::Colour (0xffc7bba8);
        textLeft = 22;
    }
    else if (selected)
    {
        g.setColour (drumdoctor::palette::amber().withAlpha (0.22f));
        g.fillRect (0, 0, width, height);
    }

    g.setColour (textColour);
    g.setFont (drumdoctor::palette::font (pluginStyle ? 12.0f : 12.5f));
    g.drawText (drumdoctor::violationLabel (violations[(size_t) row]),
                textLeft, 0, width - textLeft - 8, height,
                juce::Justification::centredLeft, true);
}

void ViolationListComponent::selectedRowsChanged (int lastRowSelected)
{
    if (onSelect != nullptr && lastRowSelected >= 0)
        onSelect (lastRowSelected);
}

void ViolationListComponent::paint (juce::Graphics& g)
{
    if (! pluginStyle)
        return;

    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff2c2823));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff161310));
    g.fillRoundedRectangle (bounds.reduced (1.5f), 7.0f);
}

void ViolationListComponent::resized()
{
    auto area = getLocalBounds().reduced (pluginStyle ? 12 : 10, pluginStyle ? 10 : 8);
    auto titleRow = area.removeFromTop (pluginStyle ? 22 : 18);
    header.setBounds (titleRow);
    list.setBounds (area);
    emptyLabel.setBounds (area.reduced (16, 8));
}
