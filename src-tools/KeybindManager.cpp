// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"

dm::KeybindManager::KeybindManager()
{
	initializeDefaults();
	loadKeybinds();
}

dm::KeybindManager::~KeybindManager()
{
}

void dm::KeybindManager::initializeDefaults()
{
	keybinds.clear();
	// Navigation
	keybinds.emplace_back(KeybindAction::NavigateLeft, KeyPress(KeyPress::leftKey));
	keybinds.emplace_back(KeybindAction::NavigateRight, KeyPress(KeyPress::rightKey));
	keybinds.emplace_back(KeybindAction::NavigateUp, KeyPress(KeyPress::upKey));
	keybinds.emplace_back(KeybindAction::NavigateDown, KeyPress(KeyPress::downKey));
	keybinds.emplace_back(KeybindAction::NavigatePageUp, KeyPress(KeyPress::pageUpKey));
	keybinds.emplace_back(KeybindAction::NavigatePageDown, KeyPress(KeyPress::pageDownKey));
	keybinds.emplace_back(KeybindAction::NavigateHome, KeyPress(KeyPress::homeKey));
	keybinds.emplace_back(KeybindAction::NavigateEnd, KeyPress(KeyPress::endKey));
	keybinds.emplace_back(KeybindAction::NavigateNextMark, KeyPress(KeyPress::tabKey));
	keybinds.emplace_back(KeybindAction::NavigatePreviousMark, KeyPress::createFromDescription("shift + tab"));

	// Image operations
	keybinds.emplace_back(KeybindAction::AcceptAllMarks, KeyPress::createFromDescription("a"));
	keybinds.emplace_back(KeybindAction::AcceptCurrentMark, KeyPress::createFromDescription("shift + a"));
	keybinds.emplace_back(KeybindAction::EraseAllMarks, KeyPress::createFromDescription("shift + c"));
	keybinds.emplace_back(KeybindAction::DeleteCurrentImage, KeyPress::createFromDescription("shift + delete"));
	keybinds.emplace_back(KeybindAction::DeleteSelectedMark, KeyPress(KeyPress::deleteKey));
	keybinds.emplace_back(KeybindAction::CopyAnnotations, KeyPress::createFromDescription("ctrl + c"));
	keybinds.emplace_back(KeybindAction::PasteAnnotations, KeyPress::createFromDescription("ctrl + v"));

	// Zoom operations
	keybinds.emplace_back(KeybindAction::ZoomIn, KeyPress::createFromDescription("+"));
	keybinds.emplace_back(KeybindAction::ZoomInLarge, KeyPress::createFromDescription("shift + +"));
	keybinds.emplace_back(KeybindAction::ZoomOut, KeyPress::createFromDescription("-"));
	keybinds.emplace_back(KeybindAction::ToggleAutoZoom, KeyPress(KeyPress::spaceKey));

	// Display toggles
	keybinds.emplace_back(KeybindAction::ToggleShowPredictions, KeyPress::createFromDescription("p"));
	keybinds.emplace_back(KeybindAction::ToggleShowMarks, KeyPress::createFromDescription("m"));
	keybinds.emplace_back(KeybindAction::ToggleShowLabels, KeyPress::createFromDescription("l"));
	keybinds.emplace_back(KeybindAction::ToggleBoldLabels, KeyPress::createFromDescription("b"));
	keybinds.emplace_back(KeybindAction::ToggleShadeRectangles, KeyPress::createFromDescription("shift + b"));
	keybinds.emplace_back(KeybindAction::ToggleBlackAndWhiteMode, KeyPress::createFromDescription("w"));
	keybinds.emplace_back(KeybindAction::ToggleHeatmaps, KeyPress::createFromDescription("shift + h"));
	keybinds.emplace_back(KeybindAction::CycleHeatmaps, KeyPress::createFromDescription("h"));
	keybinds.emplace_back(KeybindAction::ToggleSnapping, KeyPress::createFromDescription("shift + d"));
	keybinds.emplace_back(KeybindAction::ToggleImageTiling, KeyPress::createFromDescription("t"));

	// Sorting
	keybinds.emplace_back(KeybindAction::SortRandom, KeyPress::createFromDescription("r"));
	keybinds.emplace_back(KeybindAction::SortAlphabetical, KeyPress::createFromDescription("shift + r"));

	// Special modes
	keybinds.emplace_back(KeybindAction::ToggleMassDeleteMode, KeyPress::createFromDescription("o"));
	keybinds.emplace_back(KeybindAction::ToggleMergeMode, KeyPress::createFromDescription("`"));
	keybinds.emplace_back(KeybindAction::ConfirmMergeSelection, KeyPress(KeyPress::returnKey));
	keybinds.emplace_back(KeybindAction::ShowClassMenu, KeyPress::createFromDescription("c"));
	keybinds.emplace_back(KeybindAction::ShowSettings, KeyPress::createFromDescription("e"));
	keybinds.emplace_back(KeybindAction::ShowKeybindEditor, KeyPress::createFromDescription("ctrl + e"));
	keybinds.emplace_back(KeybindAction::ShowFilter, KeyPress::createFromDescription("f"));
	keybinds.emplace_back(KeybindAction::ShowJumpWindow, KeyPress::createFromDescription("j"));
	keybinds.emplace_back(KeybindAction::ShowAbout, KeyPress(KeyPress::F1Key));
	keybinds.emplace_back(KeybindAction::ZoomAndReview, KeyPress::createFromDescription("z"));
	keybinds.emplace_back(KeybindAction::SnapAnnotations, KeyPress::createFromDescription("d"));
	keybinds.emplace_back(KeybindAction::MarkImageEmpty, KeyPress::createFromDescription("n"));
	keybinds.emplace_back(KeybindAction::CopyMarksFromPrevious, KeyPress::createFromDescription("y"));
	keybinds.emplace_back(KeybindAction::CopyMarksFromNext, KeyPress::createFromDescription("shift + y"));
	keybinds.emplace_back(KeybindAction::SaveScreenshot, KeyPress::createFromDescription("s"));
	keybinds.emplace_back(KeybindAction::SaveScreenshotFullSize, KeyPress::createFromDescription("shift + s"));

	// Threshold adjustments
	keybinds.emplace_back(KeybindAction::IncreaseThreshold, KeyPress(KeyPress::upKey));
	keybinds.emplace_back(KeybindAction::DecreaseThreshold, KeyPress(KeyPress::downKey));

	// Class selection (0-9)
	keybinds.emplace_back(KeybindAction::SelectClass0, KeyPress::createFromDescription("0"));
	keybinds.emplace_back(KeybindAction::SelectClass1, KeyPress::createFromDescription("1"));
	keybinds.emplace_back(KeybindAction::SelectClass2, KeyPress::createFromDescription("2"));
	keybinds.emplace_back(KeybindAction::SelectClass3, KeyPress::createFromDescription("3"));
	keybinds.emplace_back(KeybindAction::SelectClass4, KeyPress::createFromDescription("4"));
	keybinds.emplace_back(KeybindAction::SelectClass5, KeyPress::createFromDescription("5"));
	keybinds.emplace_back(KeybindAction::SelectClass6, KeyPress::createFromDescription("6"));
	keybinds.emplace_back(KeybindAction::SelectClass7, KeyPress::createFromDescription("7"));
	keybinds.emplace_back(KeybindAction::SelectClass8, KeyPress::createFromDescription("8"));
	keybinds.emplace_back(KeybindAction::SelectClass9, KeyPress::createFromDescription("9"));

	// Class selection (Ctrl+0-9)
	keybinds.emplace_back(KeybindAction::SelectClass10, KeyPress::createFromDescription("ctrl + 0"));
	keybinds.emplace_back(KeybindAction::SelectClass11, KeyPress::createFromDescription("ctrl + 1"));
	keybinds.emplace_back(KeybindAction::SelectClass12, KeyPress::createFromDescription("ctrl + 2"));
	keybinds.emplace_back(KeybindAction::SelectClass13, KeyPress::createFromDescription("ctrl + 3"));
	keybinds.emplace_back(KeybindAction::SelectClass14, KeyPress::createFromDescription("ctrl + 4"));
	keybinds.emplace_back(KeybindAction::SelectClass15, KeyPress::createFromDescription("ctrl + 5"));
	keybinds.emplace_back(KeybindAction::SelectClass16, KeyPress::createFromDescription("ctrl + 6"));
	keybinds.emplace_back(KeybindAction::SelectClass17, KeyPress::createFromDescription("ctrl + 7"));
	keybinds.emplace_back(KeybindAction::SelectClass18, KeyPress::createFromDescription("ctrl + 8"));
	keybinds.emplace_back(KeybindAction::SelectClass19, KeyPress::createFromDescription("ctrl + 9"));

	// Class selection (Alt+0-9)
	keybinds.emplace_back(KeybindAction::SelectClass20, KeyPress::createFromDescription("alt + 0"));
	keybinds.emplace_back(KeybindAction::SelectClass21, KeyPress::createFromDescription("alt + 1"));
	keybinds.emplace_back(KeybindAction::SelectClass22, KeyPress::createFromDescription("alt + 2"));
	keybinds.emplace_back(KeybindAction::SelectClass23, KeyPress::createFromDescription("alt + 3"));
	keybinds.emplace_back(KeybindAction::SelectClass24, KeyPress::createFromDescription("alt + 4"));
	keybinds.emplace_back(KeybindAction::SelectClass25, KeyPress::createFromDescription("alt + 5"));
	keybinds.emplace_back(KeybindAction::SelectClass26, KeyPress::createFromDescription("alt + 6"));
	keybinds.emplace_back(KeybindAction::SelectClass27, KeyPress::createFromDescription("alt + 7"));
	keybinds.emplace_back(KeybindAction::SelectClass28, KeyPress::createFromDescription("alt + 8"));
	keybinds.emplace_back(KeybindAction::SelectClass29, KeyPress::createFromDescription("alt + 9"));

	// Special actions
	keybinds.emplace_back(KeybindAction::Quit, KeyPress(KeyPress::escapeKey));
}

void dm::KeybindManager::loadKeybinds()
{
	// Load each keybind from configuration, falling back to defaults
	for (auto& pair : keybinds)
	{
		const String configKey = actionToConfigKey(pair.first);
		const String keyDescription = String(cfg().get_str(configKey.toStdString(), pair.second.getTextDescription().toStdString()));

		if (keyDescription.isNotEmpty())
		{
			KeyPress key = KeyPress::createFromDescription(keyDescription);
			if (key.isValid())
			{
				pair.second = key;
			}
		}
	}
}

void dm::KeybindManager::saveKeybinds()
{
	// Save each keybind to configuration
	for (const auto& pair : keybinds)
	{
		const String configKey = actionToConfigKey(pair.first);
		cfg().setValue(configKey.toStdString(), var(pair.second.getTextDescription().toStdString()));
	}
}

dm::KeybindAction dm::KeybindManager::getActionForKey(const KeyPress& key) const
{
	for (const auto& pair : keybinds)
	{
		if (pair.second == key)
		{
			return pair.first;
		}
	}
	return KeybindAction::Unknown;
}

KeyPress dm::KeybindManager::getKeyForAction(KeybindAction action) const
{
	for (const auto& pair : keybinds)
	{
		if (pair.first == action)
		{
			return pair.second;
		}
	}
	return KeyPress();
}

void dm::KeybindManager::setKeybind(KeybindAction action, const KeyPress& key)
{
	for (auto& pair : keybinds)
	{
		if (pair.first == action)
		{
			pair.second = key;
			return;
		}
	}
	keybinds.emplace_back(action, key);
}

void dm::KeybindManager::removeKeybind(KeybindAction action)
{
	for (auto it = keybinds.begin(); it != keybinds.end(); ++it)
	{
		if (it->first == action)
		{
			keybinds.erase(it);
			return;
		}
	}
}

const std::vector<std::pair<dm::KeybindAction, KeyPress>>& dm::KeybindManager::getAllKeybinds() const
{
	return keybinds;
}

KeyPress dm::KeybindManager::getDefaultKey(KeybindAction action)
{
	KeybindManager temp;
	temp.initializeDefaults();
	return temp.getKeyForAction(action);
}

String dm::KeybindManager::getActionName(KeybindAction action)
{
	switch (action)
	{
		case KeybindAction::NavigateLeft: return "Navigate Left";
		case KeybindAction::NavigateRight: return "Navigate Right";
		case KeybindAction::NavigateUp: return "Navigate Up";
		case KeybindAction::NavigateDown: return "Navigate Down";
		case KeybindAction::NavigatePageUp: return "Navigate Page Up";
		case KeybindAction::NavigatePageDown: return "Navigate Page Down";
		case KeybindAction::NavigateHome: return "Navigate Home";
		case KeybindAction::NavigateEnd: return "Navigate End";
		case KeybindAction::NavigateNextMark: return "Navigate Next Mark";
		case KeybindAction::NavigatePreviousMark: return "Navigate Previous Mark";
		case KeybindAction::AcceptAllMarks: return "Accept All Marks";
		case KeybindAction::AcceptCurrentMark: return "Accept Current Mark";
		case KeybindAction::EraseAllMarks: return "Erase All Marks";
		case KeybindAction::DeleteCurrentImage: return "Delete Current Image";
		case KeybindAction::DeleteSelectedMark: return "Delete Selected Mark";
		case KeybindAction::CopyAnnotations: return "Copy Annotations";
		case KeybindAction::PasteAnnotations: return "Paste Annotations";
		case KeybindAction::ZoomIn: return "Zoom In";
		case KeybindAction::ZoomInLarge: return "Zoom In Large";
		case KeybindAction::ZoomOut: return "Zoom Out";
		case KeybindAction::ToggleAutoZoom: return "Toggle Auto Zoom";
		case KeybindAction::ToggleShowPredictions: return "Toggle Show Predictions";
		case KeybindAction::ToggleShowMarks: return "Toggle Show Marks";
		case KeybindAction::ToggleShowLabels: return "Toggle Show Labels";
		case KeybindAction::ToggleBoldLabels: return "Toggle Bold Labels";
		case KeybindAction::ToggleShadeRectangles: return "Toggle Shade Rectangles";
		case KeybindAction::ToggleBlackAndWhiteMode: return "Toggle Black and White Mode";
		case KeybindAction::ToggleHeatmaps: return "Toggle Heatmaps";
		case KeybindAction::CycleHeatmaps: return "Cycle Heatmaps";
		case KeybindAction::ToggleSnapping: return "Toggle Snapping";
		case KeybindAction::ToggleImageTiling: return "Toggle Image Tiling";
		case KeybindAction::SortRandom: return "Sort Random";
		case KeybindAction::SortAlphabetical: return "Sort Alphabetical";
		case KeybindAction::ToggleMassDeleteMode: return "Toggle Mass Delete Mode";
		case KeybindAction::ToggleMergeMode: return "Toggle Merge Mode";
		case KeybindAction::ConfirmMergeSelection: return "Confirm Merge Selection";
		case KeybindAction::ShowClassMenu: return "Show Class Menu";
		case KeybindAction::ShowSettings: return "Show Settings";
		case KeybindAction::ShowKeybindEditor: return "Show Keybind Editor";
		case KeybindAction::ShowFilter: return "Show Filter";
		case KeybindAction::ShowJumpWindow: return "Show Jump Window";
		case KeybindAction::ShowAbout: return "Show About";
		case KeybindAction::ZoomAndReview: return "Zoom and Review";
		case KeybindAction::SnapAnnotations: return "Snap Annotations";
		case KeybindAction::MarkImageEmpty: return "Mark Image Empty";
		case KeybindAction::CopyMarksFromPrevious: return "Copy Marks From Previous";
		case KeybindAction::CopyMarksFromNext: return "Copy Marks From Next";
		case KeybindAction::SaveScreenshot: return "Save Screenshot";
		case KeybindAction::SaveScreenshotFullSize: return "Save Screenshot Full Size";
		case KeybindAction::IncreaseThreshold: return "Increase Threshold";
		case KeybindAction::DecreaseThreshold: return "Decrease Threshold";
		case KeybindAction::SelectClass0: return "Select Class 0";
		case KeybindAction::SelectClass1: return "Select Class 1";
		case KeybindAction::SelectClass2: return "Select Class 2";
		case KeybindAction::SelectClass3: return "Select Class 3";
		case KeybindAction::SelectClass4: return "Select Class 4";
		case KeybindAction::SelectClass5: return "Select Class 5";
		case KeybindAction::SelectClass6: return "Select Class 6";
		case KeybindAction::SelectClass7: return "Select Class 7";
		case KeybindAction::SelectClass8: return "Select Class 8";
		case KeybindAction::SelectClass9: return "Select Class 9";
		case KeybindAction::SelectClass10: return "Select Class 10";
		case KeybindAction::SelectClass11: return "Select Class 11";
		case KeybindAction::SelectClass12: return "Select Class 12";
		case KeybindAction::SelectClass13: return "Select Class 13";
		case KeybindAction::SelectClass14: return "Select Class 14";
		case KeybindAction::SelectClass15: return "Select Class 15";
		case KeybindAction::SelectClass16: return "Select Class 16";
		case KeybindAction::SelectClass17: return "Select Class 17";
		case KeybindAction::SelectClass18: return "Select Class 18";
		case KeybindAction::SelectClass19: return "Select Class 19";
		case KeybindAction::SelectClass20: return "Select Class 20";
		case KeybindAction::SelectClass21: return "Select Class 21";
		case KeybindAction::SelectClass22: return "Select Class 22";
		case KeybindAction::SelectClass23: return "Select Class 23";
		case KeybindAction::SelectClass24: return "Select Class 24";
		case KeybindAction::SelectClass25: return "Select Class 25";
		case KeybindAction::SelectClass26: return "Select Class 26";
		case KeybindAction::SelectClass27: return "Select Class 27";
		case KeybindAction::SelectClass28: return "Select Class 28";
		case KeybindAction::SelectClass29: return "Select Class 29";
		case KeybindAction::Quit: return "Quit";
		case KeybindAction::Unknown: return "Unknown";
	}
	return "Unknown";
}

String dm::KeybindManager::getActionDescription(KeybindAction action)
{
	switch (action)
	{
		case KeybindAction::NavigateLeft: return "Go to previous image";
		case KeybindAction::NavigateRight: return "Go to next image";
		case KeybindAction::NavigateUp: return "Increase neural network threshold";
		case KeybindAction::NavigateDown: return "Decrease neural network threshold";
		case KeybindAction::NavigatePageUp: return "Go to previous unmarked image";
		case KeybindAction::NavigatePageDown: return "Go to next unmarked image";
		case KeybindAction::NavigateHome: return "Go to first image";
		case KeybindAction::NavigateEnd: return "Go to last image";
		case KeybindAction::NavigateNextMark: return "Move focus to next marking";
		case KeybindAction::NavigatePreviousMark: return "Move focus to previous marking";
		case KeybindAction::AcceptAllMarks: return "Accept all marks";
		case KeybindAction::AcceptCurrentMark: return "Accept current mark";
		case KeybindAction::EraseAllMarks: return "Erase all marks";
		case KeybindAction::DeleteCurrentImage: return "Delete current image from disk";
		case KeybindAction::DeleteSelectedMark: return "Delete currently selected marking";
		case KeybindAction::CopyAnnotations: return "Copy annotations to clipboard";
		case KeybindAction::PasteAnnotations: return "Paste annotations from clipboard";
		case KeybindAction::ZoomIn: return "Zoom image +10%";
		case KeybindAction::ZoomInLarge: return "Zoom image 500%";
		case KeybindAction::ZoomOut: return "Zoom image -10%";
		case KeybindAction::ToggleAutoZoom: return "Toggle between automatic and manual zoom";
		case KeybindAction::ToggleShowPredictions: return "Toggle showing predictions";
		case KeybindAction::ToggleShowMarks: return "Toggle showing user marks";
		case KeybindAction::ToggleShowLabels: return "Toggle showing labels";
		case KeybindAction::ToggleBoldLabels: return "Toggle bold labels";
		case KeybindAction::ToggleShadeRectangles: return "Toggle shading rectangles";
		case KeybindAction::ToggleBlackAndWhiteMode: return "Toggle black and white mode";
		case KeybindAction::ToggleHeatmaps: return "Toggle heatmaps";
		case KeybindAction::CycleHeatmaps: return "Cycle through heatmaps";
		case KeybindAction::ToggleSnapping: return "Toggle annotation snapping";
		case KeybindAction::ToggleImageTiling: return "Toggle image tiling";
		case KeybindAction::SortRandom: return "Random sort";
		case KeybindAction::SortAlphabetical: return "Alphabetical sort";
		case KeybindAction::ToggleMassDeleteMode: return "Toggle mass delete mode";
		case KeybindAction::ToggleMergeMode: return "Toggle merge mode";
		case KeybindAction::ConfirmMergeSelection: return "Confirm merge selection";
		case KeybindAction::ShowClassMenu: return "Show class menu";
		case KeybindAction::ShowSettings: return "Show settings window";
		case KeybindAction::ShowKeybindEditor: return "Show keybind editor window";
		case KeybindAction::ShowFilter: return "Show filter window";
		case KeybindAction::ShowJumpWindow: return "Show jump window";
		case KeybindAction::ShowAbout: return "Show about window";
		case KeybindAction::ZoomAndReview: return "Zoom and review";
		case KeybindAction::SnapAnnotations: return "Snap annotations to grid";
		case KeybindAction::MarkImageEmpty: return "Mark image as empty";
		case KeybindAction::CopyMarksFromPrevious: return "Copy marks from previous image";
		case KeybindAction::CopyMarksFromNext: return "Copy marks from next image";
		case KeybindAction::SaveScreenshot: return "Save screenshot";
		case KeybindAction::SaveScreenshotFullSize: return "Save screenshot at full size";
		case KeybindAction::IncreaseThreshold: return "Increase threshold";
		case KeybindAction::DecreaseThreshold: return "Decrease threshold";
		case KeybindAction::SelectClass0: return "Select class 0";
		case KeybindAction::SelectClass1: return "Select class 1";
		case KeybindAction::SelectClass2: return "Select class 2";
		case KeybindAction::SelectClass3: return "Select class 3";
		case KeybindAction::SelectClass4: return "Select class 4";
		case KeybindAction::SelectClass5: return "Select class 5";
		case KeybindAction::SelectClass6: return "Select class 6";
		case KeybindAction::SelectClass7: return "Select class 7";
		case KeybindAction::SelectClass8: return "Select class 8";
		case KeybindAction::SelectClass9: return "Select class 9";
		case KeybindAction::SelectClass10: return "Select class 10";
		case KeybindAction::SelectClass11: return "Select class 11";
		case KeybindAction::SelectClass12: return "Select class 12";
		case KeybindAction::SelectClass13: return "Select class 13";
		case KeybindAction::SelectClass14: return "Select class 14";
		case KeybindAction::SelectClass15: return "Select class 15";
		case KeybindAction::SelectClass16: return "Select class 16";
		case KeybindAction::SelectClass17: return "Select class 17";
		case KeybindAction::SelectClass18: return "Select class 18";
		case KeybindAction::SelectClass19: return "Select class 19";
		case KeybindAction::SelectClass20: return "Select class 20";
		case KeybindAction::SelectClass21: return "Select class 21";
		case KeybindAction::SelectClass22: return "Select class 22";
		case KeybindAction::SelectClass23: return "Select class 23";
		case KeybindAction::SelectClass24: return "Select class 24";
		case KeybindAction::SelectClass25: return "Select class 25";
		case KeybindAction::SelectClass26: return "Select class 26";
		case KeybindAction::SelectClass27: return "Select class 27";
		case KeybindAction::SelectClass28: return "Select class 28";
		case KeybindAction::SelectClass29: return "Select class 29";
		case KeybindAction::Quit: return "Quit application";
		case KeybindAction::Unknown: return "Unknown action";
	}
	return "Unknown action";
}

void dm::KeybindManager::resetToDefaults()
{
	initializeDefaults();
}

bool dm::KeybindManager::isKeyBound(const KeyPress& key, KeybindAction excludeAction) const
{
	for (const auto& pair : keybinds)
	{
		if (pair.first != excludeAction && pair.second == key)
		{
			return true;
		}
	}
	return false;
}

String dm::KeybindManager::actionToConfigKey(KeybindAction action) const
{
	return "keybind_" + String(static_cast<int>(action));
}

dm::KeybindAction dm::KeybindManager::configKeyToAction(const String& key) const
{
	if (key.startsWith("keybind_"))
	{
		int actionInt = key.substring(8).getIntValue();
		if (actionInt >= 0 && actionInt < static_cast<int>(KeybindAction::Unknown))
		{
			return static_cast<KeybindAction>(actionInt);
		}
	}
	return KeybindAction::Unknown;
}
