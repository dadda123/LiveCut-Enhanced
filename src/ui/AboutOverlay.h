/*
 This file is part of LiveCut Enhanced.
 Based on Livecut, Copyright 2004 by Remy Muller.

 Livecut can be redistributed and/or modified under the terms of the
 GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 Livecut is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */

#pragma once

#include "LiveCutLookAndFeel.h"

#if !defined(JucePlugin_VersionString)
#define JucePlugin_VersionString "1.0.0"
#endif

//==============================================================================
class AboutOverlay : public juce::Component
{
public:
    AboutOverlay()
    {
        setWantsKeyboardFocus(true);

        viewport.setViewedComponent(&content, false);
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(10);
        addAndMakeVisible(viewport);

        closeButton.onClick = [this]
        { dismiss(); };
        addAndMakeVisible(closeButton);
    }

    void show()
    {
        setVisible(true);
        toFront(true);
        grabKeyboardFocus();
    }

    void dismiss() { setVisible(false); }

    void setHelpMode (bool enabled)
    {
        closeButton.setTooltip (enabled ? "Close the About panel." : juce::String());
    }

    void paint(juce::Graphics &g) override
    {
        g.fillAll(Colors::background.withAlpha(0.72f));

        auto r = panelBounds().toFloat().reduced(0.5f);
        g.setColour(Colors::panel);
        g.fillRoundedRectangle(r, 10.f);
        g.setColour(Colors::panelOutline);
        g.drawRoundedRectangle(r, 10.f, 1.f);

        auto header = panelBounds().reduced(14, 0).removeFromTop(34);
        g.setColour(Colors::accent);
        g.fillRoundedRectangle(header.removeFromLeft(16).withSizeKeepingCentre(8, 8).toFloat(),
                               2.f);
        header.removeFromLeft(6);
        g.setColour(Colors::textDim);
        g.setFont(juce::Font(juce::FontOptions(12.f, juce::Font::bold)));
        g.drawText("ABOUT", header, juce::Justification::centredLeft, false);
    }

    void resized() override
    {
        auto p = panelBounds();
        closeButton.setBounds(p.removeFromTop(34).removeFromRight(34).reduced(5));
        viewport.setBounds(p.reduced(16, 0).withTrimmedBottom(14));
        content.setSize(viewport.getWidth() - viewport.getScrollBarThickness() - 4,
                        content.getIdealHeight());
    }

    void mouseDown(const juce::MouseEvent &e) override
    {
        if (!panelBounds().contains(e.getPosition()))
            dismiss();
    }

    bool keyPressed(const juce::KeyPress &key) override
    {
        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }
        return false;
    }

private:
    juce::Rectangle<int> panelBounds() const
    {
        return getLocalBounds().withSizeKeepingCentre(juce::jmin(600, getWidth() - 64),
                                                      juce::jmin(500, getHeight() - 64));
    }

    //==========================================================================
    class LinkRow : public juce::Component
    {
    public:
        LinkRow(const juce::String &prefixText, const juce::String &url)
            : prefix(prefixText), link(url, juce::URL(url))
        {
            styleLink(link);
            addAndMakeVisible(link);
        }

        void paint(juce::Graphics &g) override
        {
            g.setColour(Colors::textDim);
            g.setFont(juce::Font(juce::FontOptions(13.5f)));
            g.drawText(prefix, getLocalBounds().withWidth(prefixWidth),
                       juce::Justification::centredLeft, false);
        }

        void resized() override { link.setBounds(getLocalBounds().withTrimmedLeft(prefixWidth)); }

        static void styleLink(juce::HyperlinkButton &b)
        {
            b.setFont(juce::Font(juce::FontOptions(13.5f)), false,
                      juce::Justification::centredLeft);
            b.setColour(juce::HyperlinkButton::textColourId, Colors::accent);
        }

    private:
        static constexpr int prefixWidth = 100;
        juce::String prefix;
        juce::HyperlinkButton link;
    };

    //==========================================================================
    /** The scrollable page. Static text is painted directly (the LookAndFeel
        pins all Label fonts to 17.0f, so Labels can't carry custom sizes);
        only the clickable links are child components. */
    class Content : public juce::Component
    {
    public:
        Content()
        {
            addText("Livecut Enhanced", 18.f, true, Colors::text, 26);
            addText(juce::String("Version ") + JucePlugin_VersionString, 13.5f, false,
                    Colors::textDim);
            addSpace(16);

            addText("A modern 64-bit Windows build of Livecut, a real-time beat-slicer");
            addText("based on the BBCut algorithm (Nick Collins), with a new GUI built");
            addText("in JUCE 8.");
            addSpace(10);

            addText("This build adds a modern GUI with help mode and fixes.", 13.5f, false,
                    Colors::textDim);
            addText("Tweaks made to mitigate issues where audio could stop during looped playback.", 13.5f,
                    false, Colors::textDim);
            addSpace(20);

            addHeading("Credits");
            addText("Original Livecut: mdsp @ smartelectronix, GPL v2+");
            addLink("https://github.com/mdsp/Livecut");
            addSpace(10);
            addText("GUI redesign & enhancements (this build): Dadda");
            addLink("https://daddasounds.com");
            addSpace(20);

            addHeading("License");
            addText("Licensed under GPL v2 or later, same as the original.");
            addSpace(10);
            addText("VST 3 SDK (c) Steinberg Media Technologies GmbH, MIT License.", 13.5f,
                    false, Colors::textDim);
            addText("VST is a trademark of Steinberg Media Technologies GmbH.", 13.5f, false,
                    Colors::textDim);
            addSpace(20);

            addHeading("Links");
            addPair("Website:", "https://daddasounds.com");
            addPair("Source code:", "https://github.com/dadda123/livecut-enhanced");
            addSpace(8);
        }

        int getIdealHeight() const { return totalHeight; }

        void paint(juce::Graphics &g) override
        {
            for (const auto &t : texts)
            {
                g.setColour(t.colour);
                g.setFont(juce::Font(juce::FontOptions(t.size, t.bold ? juce::Font::bold
                                                                      : juce::Font::plain)));
                g.drawText(t.text, 0, t.y, getWidth(), t.height,
                           juce::Justification::centredLeft, false);
            }

            g.setColour(Colors::panelOutline);
            for (auto y : ruleYs)
                g.fillRect(0, y, getWidth(), 1);
        }

        void resized() override
        {
            for (const auto &c : comps)
                c.comp->setBounds(0, c.y, getWidth(), c.height);
        }

    private:
        struct TextRow
        {
            juce::String text;
            float size;
            bool bold;
            juce::Colour colour;
            int y, height;
        };

        struct CompRow
        {
            std::unique_ptr<juce::Component> comp;
            int y, height;
        };

        void addText(const juce::String &text, float size = 13.5f, bool bold = false,
                     juce::Colour colour = Colors::text, int height = 19)
        {
            texts.push_back({text, size, bold, colour, totalHeight, height});
            totalHeight += height;
        }

        void addHeading(const juce::String &text)
        {
            addText(text.toUpperCase(), 12.f, true, Colors::textDim, 20);
            ruleYs.push_back(totalHeight + 2);
            addSpace(12);
        }

        void addLink(const juce::String &url)
        {
            auto b = std::make_unique<juce::HyperlinkButton>(url, juce::URL(url));
            LinkRow::styleLink(*b);
            addAndMakeVisible(*b);
            comps.push_back({std::move(b), totalHeight, 19});
            totalHeight += 19;
        }

        void addPair(const juce::String &prefix, const juce::String &url)
        {
            auto row = std::make_unique<LinkRow>(prefix, url);
            addAndMakeVisible(*row);
            comps.push_back({std::move(row), totalHeight, 19});
            totalHeight += 19;
        }

        void addSpace(int px) { totalHeight += px; }

        std::vector<TextRow> texts;
        std::vector<CompRow> comps;
        std::vector<int> ruleYs;
        int totalHeight = 0;
    };

    //==========================================================================
    juce::TextButton closeButton{"X"};
    juce::Viewport viewport;
    Content content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutOverlay)
};
