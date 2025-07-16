// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"


dm::DMReviewWnd::DMReviewWnd(DMContent & c) :
	DocumentWindow("DarkMark v" DARKMARK_VERSION " Review", Colours::darkgrey, TitleBarButtons::allButtons),
	content(c)
{
	setContentNonOwned		(&notebook, true);
	setUsingNativeTitleBar	(true			);
	setResizable			(true, false	);
	setDropShadowEnabled	(true			);

	setIcon(DarkMarkLogo());
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	if (cfg().containsKey("ReviewWnd"))
	{
		restoreWindowStateFromString( cfg().getValue("ReviewWnd") );
	}
	else
	{
		centreWithSize(600, 200);
	}

	setVisible(true);

	return;
}


dm::DMReviewWnd::~DMReviewWnd()
{
	cfg().setValue("ReviewWnd", getWindowStateAsString());

	return;
}


void dm::DMReviewWnd::closeButtonPressed()
{
	// close button

	dmapp().review_wnd.reset(nullptr);

	return;
}


void dm::DMReviewWnd::userTriedToCloseWindow()
{
	// ALT+F4

	dmapp().review_wnd.reset(nullptr);

	return;
}


void dm::DMReviewWnd::rebuild_notebook()
{
	while (notebook.getNumTabs() > 0)
	{
		notebook.removeTab(0);
	}

	for (auto iter : m)
	{
		const size_t class_idx = iter.first;
		MReviewInfo & mri = m.at(class_idx);

		std::string name = "#" + std::to_string(class_idx);
		if (content.names.size() > class_idx)
		{
			// if we can, we'd much rather use the "official" name for this class
			name = content.names.at(class_idx);
		}
		if (content.names.size() == class_idx)
		{
			name = "* errors *";
		}

		Log("creating a notebook tab for class \"" + name + "\", mri has " + std::to_string(mri.size()) + " entries");

		auto* canvas = new DMReviewCanvas(content, mri, md5s);
		canvas->addChangeListener(this);
		notebook.addTab(name, Colours::darkgrey, canvas, true);
	}

	return;
}

void dm::DMReviewWnd::changeListenerCallback(ChangeBroadcaster* source)
{
	DMReviewCanvas* canvas = dynamic_cast<DMReviewCanvas*>(source);
	if (canvas)
	{
		// Something in one of the canvases has changed. This is currently only triggered when an image is removed.
		// We need to synchronize our data model with the main image list to remove entries for deleted images.
		SStr current_filenames(content.image_filenames.begin(), content.image_filenames.end());

		for (auto& pair : m)
		{
			MReviewInfo& class_mri = pair.second;
			for (auto it = class_mri.begin(); it != class_mri.end(); )
			{
				if (current_filenames.find(it->second.filename) == current_filenames.end())
				{
					it = class_mri.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		// Now that the data model is clean, rebuild all canvases.
		rebuild_notebook();
	}
}
