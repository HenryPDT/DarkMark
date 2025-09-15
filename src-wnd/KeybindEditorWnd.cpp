// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"

dm::KeybindEditorWnd::KeybindEditorWnd() :
	DocumentWindow("DarkMark v" DARKMARK_VERSION " - Keybind Editor", Colours::darkgrey, TitleBarButtons::closeButton),
	keybind_table("KeybindTable"),
	reset_button("Reset to Defaults"),
	save_button("Save"),
	close_button("Close"),
	editing_row(-1)
{
	setContentNonOwned(&canvas, true);
	setUsingNativeTitleBar(true);
	setResizable(true, false);
	setDropShadowEnabled(true);
	setAlwaysOnTop(true);

	canvas.addAndMakeVisible(keybind_table);
	canvas.addAndMakeVisible(reset_button);
	canvas.addAndMakeVisible(save_button);
	canvas.addAndMakeVisible(close_button);

	keybind_table.setModel(this);
	keybind_table.getHeader().addColumn("Action", 1, 200, 100, 400, TableHeaderComponent::notSortable);
	keybind_table.getHeader().addColumn("Current Key", 2, 150, 100, 200, TableHeaderComponent::notSortable);
	keybind_table.getHeader().addColumn("Default Key", 3, 150, 100, 200, TableHeaderComponent::notSortable);
	keybind_table.getHeader().setStretchToFitActive(true);
	keybind_table.setMultipleSelectionEnabled(false);
	keybind_table.setRowHeight(24);
	keybind_table.addKeyListener(this);
	keybind_table.addMouseListener(this, true);

	reset_button.addListener(this);
	save_button.addListener(this);
	close_button.addListener(this);

	setIcon(DarkMarkLogo());
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	populateKeybindRows();
	updateKeybindDisplay();

	if (cfg().containsKey("KeybindEditorWnd"))
	{
		restoreWindowStateFromString(cfg().getValue("KeybindEditorWnd"));
	}
	else
	{
		setSize(600, 500);
		centreWithSize(getWidth(), getHeight());
	}
	setVisible(true);
}

dm::KeybindEditorWnd::~KeybindEditorWnd()
{
	cfg().setValue("KeybindEditorWnd", getWindowStateAsString());
}

void dm::KeybindEditorWnd::closeButtonPressed()
{
	dmapp().keybind_editor_wnd.reset(nullptr);
}

void dm::KeybindEditorWnd::userTriedToCloseWindow()
{
	closeButtonPressed();
}

void dm::KeybindEditorWnd::resized()
{
	// get the document window to resize the canvas, then we'll deal with the rest of the components
	DocumentWindow::resized();

	const int margin_size = 5;
	auto area = getLocalBounds();
	area.reduce(margin_size, margin_size);

	auto button_area = area.removeFromBottom(40);
	button_area.removeFromTop(10);
	
	int button_width = 120;
	close_button.setBounds(button_area.removeFromRight(button_width).reduced(5));
	save_button.setBounds(button_area.removeFromRight(button_width).reduced(5));
	reset_button.setBounds(button_area.removeFromRight(button_width).reduced(5));

	area.removeFromBottom(10);
	keybind_table.setBounds(area);
}

void dm::KeybindEditorWnd::buttonClicked(Button * button)
{
	if (button == &reset_button)
	{
		resetToDefaults();
	}
	else if (button == &save_button)
	{
		saveKeybinds();
	}
	else if (button == &close_button)
	{
		closeButtonPressed();
	}
}

int dm::KeybindEditorWnd::getNumRows()
{
	return static_cast<int>(keybind_rows.size());
}

void dm::KeybindEditorWnd::paintRowBackground(Graphics & g, int rowNumber, int width, int height, bool rowIsSelected)
{
	if (rowNumber == editing_row)
	{
		g.fillAll(Colours::yellow);
	}
	else if (rowIsSelected)
	{
		g.fillAll(Colours::lightblue);
	}
	else if (rowNumber % 2 == 0)
	{
		g.fillAll(Colours::lightgrey.withAlpha(0.1f));
	}
}

void dm::KeybindEditorWnd::paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	if (rowNumber >= 0 && rowNumber < static_cast<int>(keybind_rows.size()))
	{
		const auto & row = keybind_rows[rowNumber];
		g.setColour(Colours::white);
		g.setFont(12.0f);

		String text;
		switch (columnId)
		{
			case 1: // Action
				text = row.action_name;
				break;
			case 2: // Current Key
				if (row.is_editing)
				{
					text = "Press a key... (Enter to confirm)";
					g.setColour(Colours::red);
					if (pending_key.isValid())
					{
						text = pending_key.getTextDescription();
					}
				}
				else
				{
					text = row.current_key;
				}
				break;
			case 3: // Default Key
				text = row.default_key;
				break;
		}

		g.drawText(text, 4, 0, width - 8, height, Justification::centredLeft);
	}
}

Component * dm::KeybindEditorWnd::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component * existingComponentToUpdate)
{
	return nullptr; // We're using paintCell instead
}

void dm::KeybindEditorWnd::cellClicked(int rowNumber, int columnId, const MouseEvent & event)
{
	if (columnId == 2 && rowNumber >= 0 && rowNumber < static_cast<int>(keybind_rows.size())) // Current Key column
	{
		startEditingKeybind(rowNumber);
	}
}



int dm::KeybindEditorWnd::getColumnAutoSizeWidth(int columnId)
{
	switch (columnId)
	{
		case 1: return 200; // Action
		case 2: return 150; // Current Key
		case 3: return 150; // Default Key
		default: return 100;
	}
}

String dm::KeybindEditorWnd::getCellTooltip(int rowNumber, int columnId)
{
	if (rowNumber >= 0 && rowNumber < static_cast<int>(keybind_rows.size()))
	{
		const auto & row = keybind_rows[rowNumber];
		if (columnId == 1) // Action column
		{
			return row.action_description;
		}
	}
	return String();
}

void dm::KeybindEditorWnd::populateKeybindRows()
{
	keybind_rows.clear();

	const auto& keybinds = dmapp().keybind_manager->getAllKeybinds();

	for (const auto& pair : keybinds)
	{
		KeybindRow row;
		row.action = pair.first;
		row.action_name = KeybindManager::getActionName(pair.first);
		row.action_description = KeybindManager::getActionDescription(pair.first);
		row.current_key = pair.second.getTextDescription();
		row.default_key = KeybindManager::getDefaultKey(pair.first).getTextDescription();
		row.is_editing = false;

		keybind_rows.push_back(row);
	}
}

void dm::KeybindEditorWnd::refreshKeybinds()
{
	populateKeybindRows();
	updateKeybindDisplay();
}

void dm::KeybindEditorWnd::resetToDefaults()
{
	if (dmapp().keybind_manager)
	{
		dmapp().keybind_manager->resetToDefaults();
		refreshKeybinds();
	}
}

void dm::KeybindEditorWnd::saveKeybinds()
{
	if (dmapp().keybind_manager)
	{
		for (const auto& row : keybind_rows)
		{
			if (row.current_key == "None")
			{
				dmapp().keybind_manager->removeKeybind(row.action);
			}
			else
			{
				dmapp().keybind_manager->setKeybind(row.action, KeyPress::createFromDescription(row.current_key));
			}
		}
		dmapp().keybind_manager->saveKeybinds();
		AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon, "DarkMark Keybinds", "Keybinds saved successfully!", "OK", this);
	}
}

void dm::KeybindEditorWnd::startEditingKeybind(int row)
{
	if (editing_row >= 0)
	{
		cancelEditingKeybind();
	}

	editing_row = row;
	keybind_rows[row].is_editing = true;
	pending_key = KeyPress();
	keybind_table.repaintRow(row);

	// Request keyboard focus to capture key presses
	keybind_table.grabKeyboardFocus();
}

void dm::KeybindEditorWnd::finishEditingKeybind()
{
	if (editing_row >= 0 && editing_row < static_cast<int>(keybind_rows.size()))
	{
		// Check if the key is already bound to another action
		if (dmapp().keybind_manager && dmapp().keybind_manager->isKeyBound(pending_key, keybind_rows[editing_row].action))
		{
			AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::WarningIcon, "DarkMark Keybinds", "This key is already bound to another action!");
			cancelEditingKeybind();
			return;
		}

		keybind_rows[editing_row].current_key = pending_key.getTextDescription();
		keybind_rows[editing_row].is_editing = false;

		editing_row = -1;
		updateKeybindDisplay();
	}
}



void dm::KeybindEditorWnd::cancelEditingKeybind()
{
	if (editing_row >= 0 && editing_row < static_cast<int>(keybind_rows.size()))
	{
		keybind_rows[editing_row].is_editing = false;
		editing_row = -1;
		updateKeybindDisplay();
	}
}

void dm::KeybindEditorWnd::updateKeybindDisplay()
{
	keybind_table.updateContent();
	keybind_table.repaint();
}



bool dm::KeybindEditorWnd::keyPressed(const KeyPress & key, Component * originatingComponent)
{
	if (editing_row >= 0)
	{
		if (key.getKeyCode() == KeyPress::escapeKey)
		{
			cancelEditingKeybind();
			return true;
		}
		else if (key.getKeyCode() == KeyPress::returnKey)
		{
			finishEditingKeybind();
			return true;
		}
		else
		{
			pending_key = key;
			keybind_rows[editing_row].current_key = key.getTextDescription();
			updateKeybindDisplay();
			return true;
		}
	}
	
	return false;
}
