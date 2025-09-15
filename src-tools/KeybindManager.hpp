// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"

namespace dm
{
	/// Enumeration of all available keybind actions
	enum class KeybindAction
	{
		// Navigation
		NavigateLeft,
		NavigateRight,
		NavigateUp,
		NavigateDown,
		NavigatePageUp,
		NavigatePageDown,
		NavigateHome,
		NavigateEnd,
		NavigateNextMark,
		NavigatePreviousMark,
		
		// Image operations
		AcceptAllMarks,
		AcceptCurrentMark,
		EraseAllMarks,
		DeleteCurrentImage,
		DeleteSelectedMark,
		CopyAnnotations,
		PasteAnnotations,
		
		// Zoom operations
		ZoomIn,
		ZoomInLarge,
		ZoomOut,
		ToggleAutoZoom,
		
		// Display toggles
		ToggleShowPredictions,
		ToggleShowMarks,
		ToggleShowLabels,
		ToggleBoldLabels,
		ToggleShadeRectangles,
		ToggleBlackAndWhiteMode,
		ToggleHeatmaps,
		CycleHeatmaps,
		ToggleSnapping,
		ToggleImageTiling,
		
		// Sorting
		SortRandom,
		SortAlphabetical,
		
		// Special modes
		ToggleMassDeleteMode,
		ToggleMergeMode,
		ConfirmMergeSelection,
		ShowClassMenu,
		ShowSettings,
		ShowKeybindEditor,
		ShowFilter,
		ShowJumpWindow,
		ShowAbout,
		ZoomAndReview,
		SnapAnnotations,
		MarkImageEmpty,
		CopyMarksFromPrevious,
		CopyMarksFromNext,
		SaveScreenshot,
		SaveScreenshotFullSize,
		
		// Threshold adjustments
		IncreaseThreshold,
		DecreaseThreshold,
		
		// Class selection (0-9, Ctrl+0-9, Alt+0-9)
		SelectClass0,
		SelectClass1,
		SelectClass2,
		SelectClass3,
		SelectClass4,
		SelectClass5,
		SelectClass6,
		SelectClass7,
		SelectClass8,
		SelectClass9,
		SelectClass10,
		SelectClass11,
		SelectClass12,
		SelectClass13,
		SelectClass14,
		SelectClass15,
		SelectClass16,
		SelectClass17,
		SelectClass18,
		SelectClass19,
		SelectClass20,
		SelectClass21,
		SelectClass22,
		SelectClass23,
		SelectClass24,
		SelectClass25,
		SelectClass26,
		SelectClass27,
		SelectClass28,
		SelectClass29,
		
		// Special actions
		Quit,
		Unknown
	};

	/// Manages configurable keybinds for DarkMark
	class KeybindManager
	{
		public:
			KeybindManager();
			virtual ~KeybindManager();
			
			/// Load keybinds from configuration
			void loadKeybinds();
			
			/// Save keybinds to configuration
			void saveKeybinds();
			
			/// Get the action for a given key press
			KeybindAction getActionForKey(const KeyPress& key) const;
			
			/// Get the key press for a given action
			KeyPress getKeyForAction(KeybindAction action) const;
			
			/// Set a keybind for an action
			void setKeybind(KeybindAction action, const KeyPress& key);
			
			/// Remove a keybind for an action
			void removeKeybind(KeybindAction action);
			
			/// Get all current keybinds
			const std::vector<std::pair<KeybindAction, KeyPress>>& getAllKeybinds() const;
			
			/// Get the default key press for an action
			static KeyPress getDefaultKey(KeybindAction action);
			
			/// Get a human-readable name for an action
			static String getActionName(KeybindAction action);
			
			/// Get a human-readable description for an action
			static String getActionDescription(KeybindAction action);
			
			/// Reset all keybinds to defaults
			void resetToDefaults();
			
			/// Check if a key is already bound to another action
			bool isKeyBound(const KeyPress& key, KeybindAction excludeAction = KeybindAction::Unknown) const;

		private:
			std::vector<std::pair<KeybindAction, KeyPress>> keybinds;
			
			/// Initialize default keybinds
			void initializeDefaults();
			
			/// Convert action to configuration key
			String actionToConfigKey(KeybindAction action) const;
			
			/// Convert configuration key to action
			KeybindAction configKeyToAction(const String& key) const;
	};
}