// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"

namespace dm
{
	class KeybindEditorWnd : public DocumentWindow, public Button::Listener, public TableListBoxModel, public KeyListener
	{
		public:

			KeybindEditorWnd();

			virtual ~KeybindEditorWnd();

			virtual void closeButtonPressed() override;
			virtual void userTriedToCloseWindow() override;
			virtual void resized() override;
			virtual void buttonClicked(Button * button) override;
			virtual bool keyPressed(const KeyPress & key, Component * originatingComponent) override;

			// TableListBoxModel implementation
			virtual int getNumRows() override;
			virtual void paintRowBackground(Graphics & g, int rowNumber, int width, int height, bool rowIsSelected) override;
			virtual void paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
			virtual Component * refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component * existingComponentToUpdate) override;
			virtual void cellClicked(int rowNumber, int columnId, const MouseEvent & event) override;
			virtual int getColumnAutoSizeWidth(int columnId) override;
			virtual String getCellTooltip(int rowNumber, int columnId) override;

			// Custom methods
			void refreshKeybinds();
			void resetToDefaults();
			void saveKeybinds();

		private:

			Component canvas;
			TableListBox keybind_table;
			TextButton reset_button;
			TextButton save_button;
			TextButton close_button;
			
			struct KeybindRow
			{
				KeybindAction action;
				String action_name;
				String action_description;
				String current_key;
				String default_key;
				bool is_editing = false;
			};

			std::vector<KeybindRow> keybind_rows;
			int editing_row = -1;
			KeyPress pending_key;
			
			void populateKeybindRows();
			void startEditingKeybind(int row);
			void finishEditingKeybind();
			
			void cancelEditingKeybind();
			void updateKeybindDisplay();
	};
}

