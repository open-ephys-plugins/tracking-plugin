/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2022 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "TrackingNodeEditor.h"
#include "TrackingNode.h"
#include "TrackingStimulatorCanvas.h"
#include <vector>

TrackingNodeEditor::TrackingNodeEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "Tracking"),
      selectedSource (-1)
{
    desiredWidth = 250;

    sourceLabel = std::make_unique<Label> ("Source Label", "Source");
    sourceLabel->setFont (FontOptions ("Inter", "Regular", 12.0f));
    sourceLabel->setBounds (35, 24, 60, 20);
    addAndMakeVisible (sourceLabel.get());

    trackingSourceSelector = std::make_unique<ComboBox> ("Tracking Sources");
    trackingSourceSelector->setBounds (35, 45, 90, 20);
    trackingSourceSelector->addListener (this);
    addAndMakeVisible (trackingSourceSelector.get());

    plusButton = std::make_unique<UtilityButton> ("+");
    plusButton->addListener (this);
    plusButton->setRadius (3.0f);
    plusButton->setBounds (130, 45, 20, 20);
    addAndMakeVisible (plusButton.get());

    minusButton = std::make_unique<UtilityButton> ("-");
    minusButton->addListener (this);
    minusButton->setRadius (3.0f);
    minusButton->setBounds (10, 45, 20, 20);
    addAndMakeVisible (minusButton.get());

    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "Port", 165, 25);
    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "Color", 70, 75);
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "Address", 165, 75);
    addToggleParameterEditor (Parameter::PROCESSOR_SCOPE, "StimOn", 15, 75);

    for (auto ed : parameterEditors)
    {
        ed->setLayout (ParameterEditor::Layout::nameOnTop);

        if (ed->getParameterName() == "StimOn")
            ed->setSize (40, 36);
        else
            ed->setSize (80, 36);
    }
}

Visualizer* TrackingNodeEditor::createNewCanvas()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
    return new TrackingStimulatorCanvas (processor);
}

void TrackingNodeEditor::buttonClicked (Button* btn)
{
    TrackingNode* processor = (TrackingNode*) getProcessor();

    if (btn == plusButton.get())
    {
        // add a tracking source
        int newId = 1;
        if (trackingSourceSelector->getNumItems() > 0)
        {
            newId = trackingSourceSelector->getItemId (trackingSourceSelector->getNumItems() - 1) + 1;
        }

        String txt = "Tracking source " + String (newId);

        if (processor->addSource (txt)) // check if adding the source was successfull
        {
            trackingSourceSelector->addItem (txt, newId);
            trackingSourceSelector->setSelectedId (newId, dontSendNotification);
            selectedSource = newId - 1;

            updateCustomView();

            if (canvas)
                canvas->update();
        }
    }
    else if (btn == minusButton.get())
    {
        processor->removeSource (selectedSource);

        if (selectedSource >= processor->getNumSources())
            selectedSource = processor->getNumSources() - 1;

        trackingSourceSelector->clear();
        for (int i = 0; i < processor->getNumSources(); i++)
            trackingSourceSelector->addItem ("Tracking source " + String (i + 1), i + 1);

        updateCustomView();

        if (canvas)
            canvas->update();
    }
}

void TrackingNodeEditor::comboBoxChanged (ComboBox* c)
{
    if (c == trackingSourceSelector.get())
    {
        selectedSource = c->getSelectedId() - 1;
        updateCustomView();
    }
}

void TrackingNodeEditor::saveVisualizerEditorParameters (XmlElement* xml)
{
    XmlElement* mainNode = xml->createNewChildElement ("TRACKING_SOURCES");
    mainNode->setAttribute ("selectedID", selectedSource);

    TrackingNode* processor = (TrackingNode*) getProcessor();

    for (int i = 0; i < trackingSourceSelector->getNumItems(); i++)
    {
        XmlElement* source = new XmlElement ("Source" + String (i + 1));
        source->setAttribute ("port", processor->getPort (i));
        source->setAttribute ("address", processor->getAddress (i));
        source->setAttribute ("color", processor->getColor (i));
        mainNode->addChildElement (source);
    }
}

void TrackingNodeEditor::loadVisualizerEditorParameters (XmlElement* xml)
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
    auto* mainNode = xml->getChildByName ("TRACKING_SOURCES");

    if (mainNode != nullptr)
    {
        for (auto* source : mainNode->getChildIterator())
        {
            // add a tracking source
            int newId = 1;
            if (trackingSourceSelector->getNumItems() > 0)
            {
                newId = trackingSourceSelector->getItemId (trackingSourceSelector->getNumItems() - 1) + 1;
            }

            String srcName = "Tracking source " + String (newId);

            processor->addSource (srcName,
                                  source->getIntAttribute ("port"),
                                  source->getStringAttribute ("address"),
                                  source->getStringAttribute ("color"));

            trackingSourceSelector->addItem (srcName, newId);
        }

        selectedSource = mainNode->getIntAttribute ("selectedID");
        trackingSourceSelector->setSelectedId (selectedSource + 1, dontSendNotification);

        updateCustomView();

        if (canvas)
            canvas->update();
    }
}

void TrackingNodeEditor::updateCustomView()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();

    auto portParam = processor->getParameter ("Port");
    int port = processor->getPort (selectedSource);

    if (port == 0)
        port = DEF_PORT;

    portParam->currentValue = port;

    auto addressParam = processor->getParameter ("Address");
    String addr = processor->getAddress (selectedSource);

    if (addr.isEmpty())
        addr = DEF_ADDRESS;

    addressParam->currentValue = addr;

    auto colorParam = processor->getParameter ("Color");
    String color = processor->getColor (selectedSource);

    if (color.isEmpty())
        color = DEF_COLOR;

    colorParam->currentValue = processor->colors.indexOf (color);

    for (auto ed : parameterEditors)
    {
        ed->updateView();
    }
}