// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include <iomanip>
#include <optional>
#include <random>


dm::ExportDialog::ExportDialog(Callback* callback) :
	DocumentWindow("DarkMark - Export Dataset", Colours::darkgrey, TitleBarButtons::closeButton),
	callback(callback),
	header_message("", "Configure export settings:"),
	lbl_image_selection("", "Images to export:"),
	btn_all_images("All Images"),
	btn_annotated_only("Annotated Only"),
	lbl_format_selection("", "Export format:"),
	btn_darknet_yolo("Darknet/YOLOv4"),
	btn_yolov5("YOLOv5"),
	btn_coco("COCO"),
	btn_dfine("D-FINE"),
	cb_enable_split("Enable train/validation split"),
	lbl_train_percentage("", "Training %:"),
	sl_train_percentage(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	lbl_val_percentage("", "Validation %: 20.0"),
	lbl_seed("", "Seed (optional):"),
	txt_seed(),
	help_seed("", "Leave empty for random split, or enter a number for reproducible split"),
	ok_button("Export"),
	cancel_button("Cancel"),
	ok_pressed(false),
	export_all_images(false),
	export_yolov5_format(false),
	export_coco_format(false),
	export_dfine_format(false)
{
	setContentNonOwned(&canvas, true);
	setUsingNativeTitleBar(true);
	setResizable(false, false);
	setDropShadowEnabled(true);

	canvas.addAndMakeVisible(header_message);
	canvas.addAndMakeVisible(lbl_image_selection);
	canvas.addAndMakeVisible(btn_all_images);
	canvas.addAndMakeVisible(btn_annotated_only);
	canvas.addAndMakeVisible(lbl_format_selection);
	canvas.addAndMakeVisible(btn_darknet_yolo);
	canvas.addAndMakeVisible(btn_yolov5);
	canvas.addAndMakeVisible(btn_coco);
	canvas.addAndMakeVisible(btn_dfine);
	canvas.addAndMakeVisible(cb_enable_split);
	canvas.addAndMakeVisible(lbl_train_percentage);
	canvas.addAndMakeVisible(sl_train_percentage);
	canvas.addAndMakeVisible(lbl_val_percentage);
	canvas.addAndMakeVisible(lbl_seed);
	canvas.addAndMakeVisible(txt_seed);
	canvas.addAndMakeVisible(help_seed);
	canvas.addAndMakeVisible(ok_button);
	canvas.addAndMakeVisible(cancel_button);

	// Set up image selection buttons (exclusive)
	btn_annotated_only.setToggleState(true, NotificationType::dontSendNotification);
	btn_annotated_only.setColour(TextButton::buttonOnColourId, Colours::lightblue);
	btn_all_images.setColour(TextButton::buttonOnColourId, Colours::lightblue);

	// Set up format selection buttons (exclusive)
	btn_darknet_yolo.setToggleState(true, NotificationType::dontSendNotification); // Default to Darknet/YOLOv4
	export_yolov5_format = false;
	export_coco_format = false;
	export_dfine_format = false;
	btn_darknet_yolo.setColour(TextButton::buttonOnColourId, Colours::lightgreen);
	btn_yolov5.setColour(TextButton::buttonOnColourId, Colours::lightgreen);
	btn_coco.setColour(TextButton::buttonOnColourId, Colours::lightgreen);
	btn_dfine.setColour(TextButton::buttonOnColourId, Colours::lightgreen);

	// Set up split controls
	cb_enable_split.setToggleState(false, NotificationType::dontSendNotification); // Default disabled
	sl_train_percentage.setRange(50.0, 95.0, 1.0);
	sl_train_percentage.setNumDecimalPlacesToDisplay(0);
	sl_train_percentage.setValue(80.0); // Default 80% training, 20% validation
	
	// Add slider listener to update validation percentage display
	sl_train_percentage.onValueChange = [this]() {
		double train_pct = sl_train_percentage.getValue();
		double val_pct = 100.0 - train_pct;
		lbl_val_percentage.setText("Validation %: " + String(val_pct, 1), NotificationType::dontSendNotification);
	};

	// Configure seed input
	txt_seed.setMultiLine(false);
	txt_seed.setReturnKeyStartsNewLine(false);
	txt_seed.setInputFilter(new TextEditor::LengthAndCharacterRestriction(10, "0123456789"), true);
	txt_seed.setTooltip("Enter a numeric seed for reproducible splits, or leave empty for random");
	
	help_seed.setFont(Font(FontOptions(12.0f, Font::italic)));

	// Add toggle listener to enable/disable split controls
	cb_enable_split.onStateChange = [this]() { updateSplitControls(); };

	// Add listeners
	btn_all_images.addListener(this);
	btn_annotated_only.addListener(this);
	btn_darknet_yolo.addListener(this);
	btn_yolov5.addListener(this);
	btn_coco.addListener(this);
	btn_dfine.addListener(this);
	ok_button.addListener(this);
	cancel_button.addListener(this);

	updateSplitControls(); // Initialize split control states

	setIcon(DarkMarkLogo());
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	centreWithSize(500, 450);
	setVisible(true);
	
	// Force a repaint to ensure proper rendering on problematic systems
	repaint();
	
	// Add a small delay to ensure the window is properly initialized
	MessageManager::callAsync([this]() {
		repaint();
		toFront(true);
	});
}

dm::ExportDialog::~ExportDialog()
{
}

void dm::ExportDialog::closeButtonPressed()
{
	dismissDialog(false);
}

void dm::ExportDialog::userTriedToCloseWindow()
{
	closeButtonPressed();
}

void dm::ExportDialog::updateSplitControls()
{
	bool enabled = cb_enable_split.getToggleState();
	lbl_train_percentage.setEnabled(enabled);
	sl_train_percentage.setEnabled(enabled);
	lbl_val_percentage.setEnabled(enabled);
	lbl_seed.setEnabled(enabled);
	txt_seed.setEnabled(enabled);
	help_seed.setEnabled(enabled);
}

void dm::ExportDialog::dismissDialog(bool okPressed)
{
	ok_pressed = okPressed;
	setVisible(false);
	
	if (callback)
	{
		callback->exportDialogFinished(okPressed, this);
	}
}

void dm::ExportDialog::resized()
{
	DocumentWindow::resized();

	const auto height = 25.0f;
	const auto margin_size = 10;

	FlexBox fb_rows;
	fb_rows.flexDirection = FlexBox::Direction::column;
	fb_rows.alignItems = FlexBox::AlignItems::stretch;
	fb_rows.justifyContent = FlexBox::JustifyContent::flexStart;

	// Header
	fb_rows.items.add(FlexItem(header_message).withHeight(height).withMargin(FlexItem::Margin(margin_size, 0, margin_size, 0)));
	
	// Image selection
	fb_rows.items.add(FlexItem(lbl_image_selection).withHeight(height).withMargin(FlexItem::Margin(0, 0, 5, 0)));
	FlexBox fb_images;
	fb_images.flexDirection = FlexBox::Direction::row;
	fb_images.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_images.items.add(FlexItem(btn_annotated_only).withWidth(150.0f).withHeight(height));
	fb_images.items.add(FlexItem().withWidth(margin_size));
	fb_images.items.add(FlexItem(btn_all_images).withWidth(150.0f).withHeight(height));
	fb_rows.items.add(FlexItem(fb_images).withHeight(height).withMargin(FlexItem::Margin(0, 0, margin_size, 0)));

	// Format selection
	fb_rows.items.add(FlexItem(lbl_format_selection).withHeight(height).withMargin(FlexItem::Margin(0, 0, 5, 0)));
	FlexBox fb_formats;
	fb_formats.flexDirection = FlexBox::Direction::row;
	fb_formats.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_formats.items.add(FlexItem(btn_darknet_yolo).withWidth(110.0f).withHeight(height));
	fb_formats.items.add(FlexItem().withWidth(5));
	fb_formats.items.add(FlexItem(btn_yolov5).withWidth(80.0f).withHeight(height));
	fb_formats.items.add(FlexItem().withWidth(5));
	fb_formats.items.add(FlexItem(btn_coco).withWidth(70.0f).withHeight(height));
	fb_formats.items.add(FlexItem().withWidth(5));
	fb_formats.items.add(FlexItem(btn_dfine).withWidth(80.0f).withHeight(height));
	fb_rows.items.add(FlexItem(fb_formats).withHeight(height).withMargin(FlexItem::Margin(0, 0, margin_size, 0)));

	// Split options
	fb_rows.items.add(FlexItem(cb_enable_split).withHeight(height).withMargin(FlexItem::Margin(0, 0, 5, 0)));
	
	FlexBox fb_train;
	fb_train.flexDirection = FlexBox::Direction::row;
	fb_train.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_train.items.add(FlexItem(lbl_train_percentage).withWidth(100.0f).withHeight(height));
	fb_train.items.add(FlexItem(sl_train_percentage).withWidth(200.0f).withHeight(height));
	fb_rows.items.add(FlexItem(fb_train).withHeight(height).withMargin(FlexItem::Margin(0, 0, 0, 0)));

	fb_rows.items.add(FlexItem(lbl_val_percentage).withHeight(height).withMargin(FlexItem::Margin(5, 0, 5, 0)));

	// Seed input row
	FlexBox fb_seed;
	fb_seed.flexDirection = FlexBox::Direction::row;
	fb_seed.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_seed.items.add(FlexItem(lbl_seed).withWidth(100.0f).withHeight(height));
	fb_seed.items.add(FlexItem(txt_seed).withWidth(200.0f).withHeight(height));
	fb_rows.items.add(FlexItem(fb_seed).withHeight(height).withMargin(FlexItem::Margin(0, 0, 0, 0)));

	// Help text
	fb_rows.items.add(FlexItem(help_seed).withHeight(height).withMargin(FlexItem::Margin(5, 0, margin_size, 0)));

	// Buttons
	FlexBox button_row;
	button_row.flexDirection = FlexBox::Direction::row;
	button_row.justifyContent = FlexBox::JustifyContent::flexEnd;
	button_row.items.add(FlexItem().withFlex(1.0));
	button_row.items.add(FlexItem(cancel_button).withWidth(80.0).withMargin(FlexItem::Margin(0, margin_size, 0, 0)));
	button_row.items.add(FlexItem(ok_button).withWidth(80.0));

	fb_rows.items.add(FlexItem().withFlex(1.0));
	fb_rows.items.add(FlexItem(button_row).withHeight(30.0).withMargin(FlexItem::Margin(0, 0, margin_size, 0)));

	auto r = getLocalBounds();
	r.reduce(margin_size, margin_size);
	fb_rows.performLayout(r);
}

void dm::ExportDialog::buttonClicked(Button * button)
{
	if (button == &btn_all_images)
	{
		export_all_images = true;
		btn_all_images.setToggleState(true, NotificationType::dontSendNotification);
		btn_annotated_only.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &btn_annotated_only)
	{
		export_all_images = false;
		btn_annotated_only.setToggleState(true, NotificationType::dontSendNotification);
		btn_all_images.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &btn_darknet_yolo)
	{
		export_yolov5_format = false;
		export_coco_format = false;
		export_dfine_format = false;
		btn_darknet_yolo.setToggleState(true, NotificationType::dontSendNotification);
		btn_yolov5.setToggleState(false, NotificationType::dontSendNotification);
		btn_coco.setToggleState(false, NotificationType::dontSendNotification);
		btn_dfine.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &btn_yolov5)
	{
		export_yolov5_format = true;
		export_coco_format = false;
		export_dfine_format = false;
		btn_yolov5.setToggleState(true, NotificationType::dontSendNotification);
		btn_darknet_yolo.setToggleState(false, NotificationType::dontSendNotification);
		btn_coco.setToggleState(false, NotificationType::dontSendNotification);
		btn_dfine.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &btn_coco)
	{
		export_yolov5_format = false;
		export_coco_format = true;
		export_dfine_format = false;
		btn_coco.setToggleState(true, NotificationType::dontSendNotification);
		btn_darknet_yolo.setToggleState(false, NotificationType::dontSendNotification);
		btn_yolov5.setToggleState(false, NotificationType::dontSendNotification);
		btn_dfine.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &btn_dfine)
	{
		export_yolov5_format = false;
		export_coco_format = false;
		export_dfine_format = true;
		btn_dfine.setToggleState(true, NotificationType::dontSendNotification);
		btn_darknet_yolo.setToggleState(false, NotificationType::dontSendNotification);
		btn_yolov5.setToggleState(false, NotificationType::dontSendNotification);
		btn_coco.setToggleState(false, NotificationType::dontSendNotification);
	}
	else if (button == &ok_button)
	{
		ok_pressed = true;
		dismissDialog(true);
	}
	else if (button == &cancel_button)
	{
		dismissDialog(false);
	}
}


dm::ClassIdWnd::ClassIdWnd(File project_dir, const std::string & fn) :
	DocumentWindow("DarkMark - " + File(fn).getFileName(), Colours::darkgrey, TitleBarButtons::closeButton),
	ThreadWithProgressWindow("DarkMark", true, true),
	done					(false),
	dir						(project_dir),
	names_fn				(fn),
	error_count				(0),
	add_button				("Add Row"),
	up_button				("up"	, 0.75f, Colours::lightblue),
	down_button				("down"	, 0.25f, Colours::lightblue),
	export_button			("Export..."),
	apply_button			("Apply..."),
	cancel_button			("Cancel"),
	done_looking_for_images	(false),
	is_exporting			(false),
	export_all_images		(false),
	export_yolov5_format	(false),
	export_coco_format		(false),
	export_dfine_format		(false),
	names_file_rewritten	(false),
	number_of_annotations_deleted(0),
	number_of_annotations_remapped(0),
	number_of_txt_files_rewritten(0),
	number_of_files_copied	(0),
	export_with_split		(false),
	train_percentage		(80.0)
{
	setContentNonOwned		(&canvas, true	);
	setUsingNativeTitleBar	(true			);
	setResizable			(true, false	);
	setDropShadowEnabled	(true			);

	canvas.addAndMakeVisible(table);
	canvas.addAndMakeVisible(add_button);
	canvas.addAndMakeVisible(up_button);
	canvas.addAndMakeVisible(down_button);
	canvas.addAndMakeVisible(export_button);
	canvas.addAndMakeVisible(apply_button);
	canvas.addAndMakeVisible(cancel_button);

	table.getHeader().addColumn("original id"	, 1, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("original name"	, 2, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("images"		, 3, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("count"			, 4, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("action"		, 5, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("new id"		, 6, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("new name"		, 7, 100, 30, -1, TableHeaderComponent::notSortable);

	table.getHeader().setStretchToFitActive(true);
	table.getHeader().setPopupMenuActive(false);
	table.setModel(this);

	add_button		.setTooltip("Add a new row to the bottom of the .names file.");
	up_button		.setTooltip("Move the selected class up.");
	down_button		.setTooltip("Move the selected class down.");
	export_button	.setTooltip("Export the entire dataset -- including the images -- with the changes shown above to a brand new project.  The current dataset will remain unchanged.");
	apply_button	.setTooltip("Apply the changes shown above to the current dataset.");

	up_button		.setVisible(false);
	down_button		.setVisible(false);

	add_button		.addListener(this);
	up_button		.addListener(this);
	down_button		.addListener(this);
	export_button	.addListener(this);
	apply_button	.addListener(this);
	cancel_button	.addListener(this);

	std::ifstream ifs(names_fn);
	std::string line;
	while (std::getline(ifs, line))
	{
		add_row(line);
	}

	setIcon(DarkMarkLogo());
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	if (cfg().containsKey("ClassIdWnd"))
	{
		restoreWindowStateFromString(cfg().getValue("ClassIdWnd"));
	}
	else
	{
		centreWithSize(640, 400);
	}

	for (const auto & info : vinfo)
	{
		count_files_per_class[info.original_id]			= 0;
		count_annotations_per_class[info.original_id]	= 0;
	}

	rebuild_table();

	setVisible(true);

	counting_thread = std::thread(&dm::ClassIdWnd::count_images_and_marks, this);

	// Force a repaint to ensure proper rendering on problematic systems
	repaint();
	
	// Add a small delay to ensure the window is properly initialized
	MessageManager::callAsync([this]() {
		repaint();
		toFront(true);
	});

	return;
}


dm::ClassIdWnd::~ClassIdWnd()
{
	Log("ClassId window is being closed; done=" + std::to_string(done) + " and threadShouldExit=" + std::to_string(threadShouldExit()));

	done = true;
	signalThreadShouldExit();
	cfg().setValue("ClassIdWnd", getWindowStateAsString());

	if (counting_thread.joinable())
	{
		counting_thread.join();
	}

	// since we're no longer a modal window, make sure the parent window is re-enabled
	// otherwise the user will be locked out of the application
	if (dmapp().startup_wnd)
	{
		dmapp().startup_wnd->setEnabled(true);
	}

	return;
}


void dm::ClassIdWnd::add_row(const std::string & name)
{
	Info info;
	info.original_id	= vinfo.size();
	info.original_name	= String(name).trim().toStdString();
	info.modified_name	= info.original_name;

	if (name == "new class")
	{
		info.original_id = -1;
	}

	vinfo.push_back(info);

	return;
}


void dm::ClassIdWnd::closeButtonPressed()
{
	Log("ClassId window is being closed via close button");

	setVisible(false);

	if (dmapp().startup_wnd)
	{
		// re-enable the launcher window which started us (fake modal mode)
		dmapp().startup_wnd->setEnabled(true);

		if (is_exporting)
		{
			String message = "The dataset has been exported to:\n"
					"\n" +
					String(export_directory.c_str()) + "\n"
					"\n";
			
			if (export_with_split)
			{
				message += "Train percentage: "				+ String(train_percentage, 1) + "%\n";
				message += "Validation percentage: "		+ String(100.0 - train_percentage, 1) + "%\n";
				if (export_seed.has_value())
				{
					message += "Seed used: "				+ String(*export_seed) + "\n";
				}
				else
				{
					message += "Random seed used\n";
				}
				message += "\n";
			}
			
			message += "Number of files copied: "			+ String(number_of_files_copied			) + "\n";
			message += "Number of annotations deleted: "	+ String(number_of_annotations_deleted	) + "\n";
			message += "Number of annotations remapped: "	+ String(number_of_annotations_remapped	) + "\n";
			message += "Number of .txt files modified: "	+ String(number_of_txt_files_rewritten	) + "\n";
			
			AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon, getTitle(), message);
		}

		else  if (names_file_rewritten)
		{
			dmapp().startup_wnd->refresh_button.triggerClick();

			AlertWindow::showMessageBoxAsync(MessageBoxIconType::InfoIcon,
					getTitle(),
					"The .names file has been saved.\n"
					"\n"
					"Number of annotations deleted: "	+ String(number_of_annotations_deleted	) + "\n"
					"Number of annotations remapped: "	+ String(number_of_annotations_remapped	) + "\n"
					"Number of .txt files modified: "	+ String(number_of_txt_files_rewritten	) + "\n"
					"\n"
					"If you've deleted, merged, or altered the order of any of your classes, please remember to re-train your network!");
		}
	}

	dmapp().class_id_wnd.reset(nullptr);

	return;
}


void dm::ClassIdWnd::userTriedToCloseWindow()
{
	// ALT+F4

	Log("ClassId window is being closed via ALT+F4");

	closeButtonPressed();

	return;
}


void dm::ClassIdWnd::resized()
{
	// get the document window to resize the canvas, then we'll deal with the rest of the components
	DocumentWindow::resized();

	const int margin_size = 5;

	FlexBox button_row;
	button_row.flexDirection = FlexBox::Direction::row;
	button_row.justifyContent = FlexBox::JustifyContent::flexEnd;
	button_row.items.add(FlexItem(add_button)		.withWidth(100.0));
	button_row.items.add(FlexItem()					.withFlex(1.0));
	button_row.items.add(FlexItem(up_button)		.withWidth(50.0));
	button_row.items.add(FlexItem(down_button)		.withWidth(50.0));
	button_row.items.add(FlexItem()					.withFlex(1.0));
	button_row.items.add(FlexItem(export_button)	.withWidth(100.0));
	button_row.items.add(FlexItem()					.withWidth(margin_size));
	button_row.items.add(FlexItem(apply_button)		.withWidth(100.0));
	button_row.items.add(FlexItem()					.withWidth(margin_size));
	button_row.items.add(FlexItem(cancel_button)	.withWidth(100.0));

	FlexBox fb;
	fb.flexDirection = FlexBox::Direction::column;
	fb.items.add(FlexItem(table).withFlex(1.0));
	fb.items.add(FlexItem(button_row).withHeight(30.0).withMargin(FlexItem::Margin(margin_size, 0, 0, 0)));

	auto r = getLocalBounds();
	r.reduce(margin_size, margin_size);
	fb.performLayout(r);

	return;
}


void dm::ClassIdWnd::buttonClicked(Button * button)
{
	if (button == &add_button)
	{
		add_row("new class");
		rebuild_table();
	}
	else if (button == &export_button)
	{
		dm::Log("export button has been pressed!");
		setEnabled(false);

		// Create and show the export dialog using async pattern
		export_dialog.reset(new ExportDialog(this));
		export_dialog->setVisible(true);
		export_dialog->toFront(true);
	}
	else if (button == &apply_button)
	{
		dm::Log("apply button has been pressed!");
		setEnabled(false);
		const bool result = NativeMessageBox::showOkCancelBox(MessageBoxIconType::QuestionIcon, "DarkMark Modify Dataset", "This will overwrite your " + File(names_fn).getFileName().toStdString() + " file, and possibly modify all of your annotations. You should make a backup of your project before making these modifications.\n\nDo you wish to proceed?");
		if (result)
		{
			runThread(); // calls run() and waits for it to be done
			dm::Log("forcing the window to close");
			closeButtonPressed();
		}
		else
		{
			setEnabled(true);
		}
	}
	else if (button == &cancel_button)
	{
		dm::Log("cancel button has been pressed!");
		setEnabled(false);
		closeButtonPressed();
	}
	else if (button == &up_button)
	{
		const int row = table.getSelectedRow();
		if (row > 0)
		{
			std::swap(vinfo[row], vinfo[row - 1]);
			rebuild_table();
			table.selectRow(row - 1);
		}
	}
	else if (button == &down_button)
	{
		const int row = table.getSelectedRow();
		if (row + 1 < (int)vinfo.size())
		{
			std::swap(vinfo[row], vinfo[row + 1]);
			rebuild_table();
			table.selectRow(row + 1);
		}
	}

	return;
}


namespace
{
	void cp_files(const std::filesystem::path & src, const std::filesystem::path & dst)
	{
		// this will copy both the image and the .txt annotation file (if it exists)

		if (src.empty())
		{
			throw std::invalid_argument("cannot copy file (src is empty)");
		}
		if (dst.empty())
		{
			throw std::invalid_argument("cannot copy file (dst is empty)");
		}
		if (src == dst)
		{
			throw std::invalid_argument("cannot copy file (src and dst are the same)");
		}
		if (not std::filesystem::exists(src))
		{
			throw std::invalid_argument("src file does not exist: " + src.string());
		}

		std::error_code ec;
		std::filesystem::create_directories(dst.parent_path(), ec);
		if (ec)
		{
			throw std::filesystem::filesystem_error("error creating subdirectory", src, dst, ec);
		}

		dm::VStr extensions;
		extensions.push_back(src.extension().string());
		extensions.push_back(".txt");
		for (const auto & ext : extensions)
		{
			const auto f1 = std::filesystem::path(src).replace_extension(ext);
			const auto f2 = std::filesystem::path(dst).replace_extension(ext);

			if (std::filesystem::exists(f1))
			{
				bool success = std::filesystem::copy_file(f1, f2, ec);
				if (ec or not success)
				{
					dm::Log("failed to copy " + f1.string() + " to " + f2.string() + ": " + ec.message());
					throw std::filesystem::filesystem_error("file copy failed", f1, f2, ec);
				}
				std::filesystem::last_write_time(f2, std::filesystem::last_write_time(f1), ec); // we don't care if this call fails
			}
		}

		return;
	}
}


void dm::ClassIdWnd::run_export()
{
	const std::filesystem::path source = dir.getFullPathName().toStdString();
	std::string export_suffix = "_export_";
	if (export_with_split)
	{
		export_suffix = "_export_split_";
	}
	const std::filesystem::path target = (source.string() + export_suffix + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S").toStdString());
	export_directory = target;

	Log("export dataset src=" + source.string());
	Log("export dataset dst=" + target.string());

	setStatusMessage("Exporting files to " + target.string() + "...");

	// in addition to the images in "all_images" and annotations, we also need to copy the .cfg file(s)
	for (const auto & entry : std::filesystem::directory_iterator(source))
	{
		if (entry.is_regular_file() and
			entry.file_size() > 0 and
			entry.path().extension() == ".cfg")
		{
			const std::filesystem::path dst = target / entry.path().filename();

			cp_files(entry.path(), dst);
		}
	}

	// remember the new .names file so it gets saved in the right location in run()
	names_fn = (target / std::filesystem::relative(names_fn, source)).string();

	if (export_with_split)
	{
		// Use the same key-based approach as YOLOv5/COCO for consistency
		std::map<std::string, std::filesystem::path> image_map;
		std::map<std::string, std::filesystem::path> label_map;

		// Build maps of normalized keys to full paths (same as YOLOv5/COCO)
		for (const auto& image_path : all_images)
		{
			std::filesystem::path img_path(image_path);
			std::filesystem::path rel_path = std::filesystem::relative(img_path, source);
			
			// Use relative path without extension as key (same as YOLOv5/COCO)
			std::string key = rel_path.replace_extension("").string();
			image_map[key] = img_path;
		}

		// Find corresponding label files (same as YOLOv5/COCO)
		for (const auto& [key, image_path] : image_map)
		{
			std::filesystem::path txt_path = std::filesystem::path(image_path).replace_extension(".txt");
			if (std::filesystem::exists(txt_path))
			{
				label_map[key] = txt_path;
			}
		}

		// Determine valid keys (same as YOLOv5/COCO)
		VStr valid_keys;
		for (const auto& [key, image_path] : image_map)
		{
			if (export_all_images || label_map.count(key))
			{
				valid_keys.push_back(key);
			}
		}

		// Shuffle for split (identical to YOLOv5/COCO logic)
		std::mt19937 g;
		if (export_seed.has_value())
		{
			g.seed(*export_seed);
			Log("Using deterministic seed " + std::to_string(*export_seed) + " for Darknet export split");
		}
		else
		{
			std::random_device rd;
			int random_seed = rd();
			g.seed(random_seed);
			Log("Using random seed " + std::to_string(random_seed) + " for Darknet export split");
		}
		std::shuffle(valid_keys.begin(), valid_keys.end(), g);

		// Group shuffled keys by their parent directory (subdirectory)
		std::map<std::string, std::vector<std::pair<std::string, bool>>> subdir_assignments;
		
		for (size_t idx = 0; idx < valid_keys.size(); ++idx)
		{
			const std::string& key = valid_keys[idx];
			const auto& image_path = image_map[key];
			
			std::filesystem::path rel_path = std::filesystem::relative(image_path, source);
			std::string subdir = rel_path.parent_path().string();
			if (subdir.empty())
			{
				subdir = "."; // Root directory
			}
			
			// Determine train/val assignment using same logic as YOLOv5/COCO
			bool is_train = (idx < valid_keys.size() * train_percentage / 100.0);
			subdir_assignments[subdir].push_back({key, is_train});
		}
		
		VStr dst_images;
		dst_images.reserve(all_images.size());
		double work_completed = 0.0f;
		const double work_to_be_done = valid_keys.size();
		
		// Process each subdirectory
		for (const auto& [subdir, assignments] : subdir_assignments)
		{
			if (threadShouldExit()) break;
			
			Log("Processing subdirectory: " + subdir + " with " + std::to_string(assignments.size()) + " images");
			
			// Create train and val directories for this subdirectory
			std::filesystem::path subdir_train = target / subdir / "train";
			std::filesystem::path subdir_val = target / subdir / "val";
			
			std::error_code ec;
			std::filesystem::create_directories(subdir_train, ec);
			std::filesystem::create_directories(subdir_val, ec);
			if (ec)
			{
				Log("Failed to create split directories for " + subdir + ": " + ec.message());
				continue;
			}
			
			// Copy files to train or val directories
			for (const auto& [key, is_train] : assignments)
			{
				if (threadShouldExit()) break;
				
				setProgress(work_completed / work_to_be_done);
				work_completed++;
				
				const auto& image_path = image_map[key];
				std::filesystem::path src(image_path);
				std::filesystem::path filename = src.filename();
				
				// Determine destination directory (train or val)
				std::filesystem::path dst = is_train ? subdir_train / filename : subdir_val / filename;
				
				// Copy both image and annotation files
				cp_files(src, dst);
				number_of_files_copied++;
				dst_images.push_back(dst.string());
			}
		}
		
		all_images.swap(dst_images);
	}
	else
	{
		// Original export logic (no split)
		VStr dst_images;
		dst_images.reserve(all_images.size());
		double work_completed = 0.0f;
		const double work_to_be_done = all_images.size();

		for (size_t idx = 0; idx < all_images.size() and not threadShouldExit(); idx ++)
		{
			setProgress(work_completed / work_to_be_done);
			work_completed ++;

			std::filesystem::path src = all_images[idx];
			std::filesystem::path dst = target / std::filesystem::relative(src, source);

			if (export_all_images or std::filesystem::exists(std::filesystem::path(src).replace_extension(".txt")))
			{
				// this will copy both the image and the .txt annotation file (if it exists)
				cp_files(src, dst);
				number_of_files_copied ++;
				dst_images.push_back(dst.string());
			}
		}

		all_images.swap(dst_images);
	}

	return;
}


std::string dm::ClassIdWnd::generate_unique_filename(const std::filesystem::path& image_path, const std::filesystem::path& source)
{
	// Get relative path from source
	std::filesystem::path rel_path = std::filesystem::relative(image_path, source);
	
	// Get the original extension first
	std::string extension = image_path.extension().string();
	
	// Convert path to string without extension and replace directory separators with underscores
	std::string path_str = rel_path.replace_extension("").string();
	
	// Remove common directory names that don't add value
	std::vector<std::string> to_remove = {"train/", "val/", "valid/", "images/", "labels/"};
	for (const auto& remove_str : to_remove)
	{
		size_t pos = 0;
		while ((pos = path_str.find(remove_str, pos)) != std::string::npos)
		{
			path_str.erase(pos, remove_str.length());
		}
	}
	
	// Replace remaining directory separators and dots with underscores
	std::replace(path_str.begin(), path_str.end(), '/', '_');
	std::replace(path_str.begin(), path_str.end(), '\\', '_');
	std::replace(path_str.begin(), path_str.end(), '.', '_');
	
	// Remove any trailing underscores and add extension back
	while (!path_str.empty() && path_str.back() == '_')
	{
		path_str.pop_back();
	}
	
	return path_str + extension;
}


void dm::ClassIdWnd::run_export_yolov5()
{
	const std::filesystem::path source = dir.getFullPathName().toStdString();
	std::string export_suffix = "_yolov5_export_";
	if (export_with_split)
	{
		export_suffix = "_yolov5_export_split_";
	}
	const std::filesystem::path target = (source.string() + export_suffix + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S").toStdString());
	export_directory = target;

	Log("YOLOv5 export dataset src=" + source.string());
	Log("YOLOv5 export dataset dst=" + target.string());

	setStatusMessage("Exporting files to YOLOv5 format: " + target.string() + "...");

	// Create YOLOv5 directory structure
	const std::filesystem::path train_images_dir = target / "images" / "train";
	const std::filesystem::path val_images_dir = target / "images" / "val";
	const std::filesystem::path train_labels_dir = target / "labels" / "train";
	const std::filesystem::path val_labels_dir = target / "labels" / "val";

	std::error_code ec;
	std::filesystem::create_directories(train_images_dir, ec);
	std::filesystem::create_directories(val_images_dir, ec);
	std::filesystem::create_directories(train_labels_dir, ec);
	std::filesystem::create_directories(val_labels_dir, ec);

	if (ec)
	{
		Log("Failed to create YOLOv5 directory structure: " + ec.message());
		return;
	}

	// Analyze and pair image/label files
	std::map<std::string, std::filesystem::path> image_map;
	std::map<std::string, std::filesystem::path> label_map;

	// Build maps of normalized keys to full paths
	for (const auto& image_path : all_images)
	{
		std::filesystem::path img_path(image_path);
		std::filesystem::path rel_path = std::filesystem::relative(img_path, source);
		
		// Use relative path without extension as key to avoid collisions between train/val folders
		std::string key = rel_path.replace_extension("").string();
		
		// Simple approach: use filename without extension as key
		image_map[key] = img_path;
	}

	// Find corresponding label files
	for (const auto& [key, image_path] : image_map)
	{
		std::filesystem::path txt_path = std::filesystem::path(image_path).replace_extension(".txt");
		if (std::filesystem::exists(txt_path))
		{
			label_map[key] = txt_path;
		}
	}

	// Determine valid pairs and copy files
	VStr valid_keys;
	for (const auto& [key, image_path] : image_map)
	{
		if (export_all_images || label_map.count(key))
		{
			valid_keys.push_back(key);
		}
	}

	// Shuffle for split if enabled
	std::optional<int> actual_seed_used = export_seed; // Track the actual seed used
	if (export_with_split)
	{
		std::mt19937 g;
		if (export_seed.has_value())
		{
			g.seed(*export_seed);
			Log("Using deterministic seed " + std::to_string(*export_seed) + " for YOLOv5 export split");
		}
		else
		{
			std::random_device rd;
			int random_seed = rd();
			g.seed(random_seed);
			actual_seed_used = random_seed; // Capture the generated random seed
			Log("Using random seed " + std::to_string(random_seed) + " for YOLOv5 export split");
		}
		std::shuffle(valid_keys.begin(), valid_keys.end(), g);
	}

	double work_completed = 0.0f;
	const double work_to_be_done = valid_keys.size();

	for (const auto& key : valid_keys)
	{
		if (threadShouldExit()) break;

		setProgress(work_completed / work_to_be_done);
		work_completed++;

		const auto& image_path = image_map[key];
		
		// Determine if this is train or val
		bool is_train = true; // Default to train
		
		if (export_with_split)
		{
			// Use deterministic split based on shuffle order
			is_train = (work_completed < valid_keys.size() * train_percentage / 100.0);
		}
		else
		{
			// Use path-based detection (existing behavior)
			std::string path_str = image_path.string();
			if (path_str.find("val") != std::string::npos || path_str.find("valid") != std::string::npos)
			{
				is_train = false;
			}
		}

		// Generate unique filename based on relative path
		std::string new_image_filename = generate_unique_filename(image_path, source);
		std::string new_label_filename = std::filesystem::path(new_image_filename).replace_extension(".txt").string();

		// Copy image file
		std::filesystem::path dest_image_path = is_train ? train_images_dir / new_image_filename : val_images_dir / new_image_filename;
		std::filesystem::copy_file(image_path, dest_image_path, ec);
		if (ec)
		{
			Log("Failed to copy image " + image_path.string() + ": " + ec.message());
			continue;
		}

		// Copy label file if it exists
		if (label_map.count(key))
		{
			std::filesystem::path label_path = label_map[key];
			std::filesystem::path dest_label_path = is_train ? train_labels_dir / new_label_filename : val_labels_dir / new_label_filename;
			std::filesystem::copy_file(label_path, dest_label_path, ec);
			if (ec)
			{
				Log("Failed to copy label " + label_path.string() + ": " + ec.message());
			}
		}

		number_of_files_copied++;
	}

	// Generate dataset.yaml
	generate_dataset_yaml(target, export_with_split, train_percentage, actual_seed_used);

	return;
}


void dm::ClassIdWnd::run()
{
	setEnabled(false);

	std::map<size_t, std::string>	names;
	std::set<size_t>				to_be_deleted;
	std::map<size_t, size_t>		to_be_renumbered;

	for (const auto & info : vinfo)
	{
		if (info.action == EAction::kDelete)
		{
			dm::Log("-> class to be deleted: #" + std::to_string(info.original_id));
			to_be_deleted.insert(info.original_id);
		}
		else if (info.original_id != info.modified_id)
		{
			dm::Log("-> class #" + std::to_string(info.original_id) + " remapped to #" + std::to_string(info.modified_id));
			to_be_renumbered[info.original_id] = info.modified_id;
		}

		if (info.action == EAction::kNone)
		{
			names[info.modified_id] = info.modified_name;
		}
	}

	if (is_exporting)
	{
		if (export_yolov5_format)
		{
			run_export_yolov5();
		}
		else if (export_coco_format)
		{
			run_export_coco();
		}
		else if (export_dfine_format)
		{
			run_export_dfine();
		}
		else
		{
			run_export();
		}
	}


	std::ofstream ofs(names_fn);
	if (ofs.good())
	{
		for (const auto & [key, val] : names)
		{
			dm::Log("-> class #" + std::to_string(key) + ": \"" + val + "\"");
			ofs << val << std::endl;
		}
	}
	ofs.close();
	names_file_rewritten = true;

	if (to_be_deleted.size() > 0 or to_be_renumbered.size() > 0)
	{
		double work_completed = 0.0f;
		const double work_to_be_done = all_images.size();

		setStatusMessage("Processing " + String(all_images.size()) + " images...");

		for (size_t idx = 0; idx < all_images.size() and not threadShouldExit(); idx ++)
		{
			dm::Log("idx=" + std::to_string(idx) + " fn=" + all_images[idx]);

			setProgress(work_completed / work_to_be_done);
			work_completed ++;

			File image_filename(all_images[idx]);
			File txt_filename = image_filename.withFileExtension(".txt");
			if (not txt_filename.exists())
			{
				// nothing we can do, this image is not annotated
				continue;
			}

			struct annotation
			{
				int class_idx;
				double x;
				double y;
				double w;
				double h;

				annotation() :
					class_idx(-1),
					x(-1.0),
					y(-1.0),
					w(-1.0),
					h(-1.0)
				{
					return;
				}
			};
			std::vector<annotation> v;

			bool modified = false;
			std::ifstream ifs(txt_filename.getFullPathName().toStdString());
			while (not threadShouldExit())
			{
				annotation a;
				ifs >> a.class_idx >> a.x >> a.y >> a.w >> a.h;
				if (not ifs.good())
				{
					break;
				}

				if (to_be_deleted.count(a.class_idx))
				{
					modified = true;
					number_of_annotations_deleted ++;
				}
				else
				{
					if (to_be_renumbered.count(a.class_idx))
					{
						modified = true;
						a.class_idx = to_be_renumbered[a.class_idx];
						number_of_annotations_remapped ++;
					}

					// remember this annotation
					v.push_back(a);
				}
			}

			if (modified)
			{
				ofs.open(txt_filename.getFullPathName().toStdString());
				ofs << std::fixed << std::setprecision(10);
				for (const auto & a : v)
				{
					ofs << a.class_idx << " " << a.x << " " << a.y << " " << a.w << " " << a.h << std::endl;
				}
				ofs.close();
				number_of_txt_files_rewritten ++;

				File json_filename = txt_filename.withFileExtension(".json");
				json_filename.deleteFile();
			}
		}
	}

	return;
}


int dm::ClassIdWnd::getNumRows()
{
	return vinfo.size();
}


void dm::ClassIdWnd::paintRowBackground(Graphics &g, int rowNumber, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0					or
		rowNumber >= (int)vinfo.size()	)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	Colour colour = Colours::white;
	if (rowIsSelected)
	{
		colour = Colours::lightblue; // selected rows will have a blue background
	}
	else if (vinfo[rowNumber].action == EAction::kDelete)
	{
		colour = Colours::darksalmon;
	}
	else
	{
		colour = Colours::lightgreen;
	}

	g.fillAll(colour);

	// draw the cell bottom divider between rows
	g.setColour( Colours::black.withAlpha(0.5f));
	g.drawLine(0, height, width, height);

	return;
}


void dm::ClassIdWnd::paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0					or
		rowNumber >= (int)vinfo.size()	or
		columnId < 1					or
		columnId > 7					)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	const auto & info = vinfo[rowNumber];

	String str;
	Justification justification = Justification::centredLeft;

	if (columnId == 1) // original ID
	{
		str = String(info.original_id);
	}
	else if (columnId == 2) // original name
	{
		str = info.original_name;
	}
	else if (columnId == 3) // images
	{
		if (count_files_per_class.count(info.original_id))
		{
			str = String(count_files_per_class.at(info.original_id));
			justification = Justification::centredRight;
		}
	}
	else if (columnId == 4) // counter
	{
		if (count_annotations_per_class.count(info.original_id))
		{
			str = String(count_annotations_per_class.at(info.original_id));
			justification = Justification::centredRight;
		}
	}
	else if (columnId == 5) // action
	{
		switch (info.action)
		{
			case EAction::kMerge:
			{
				str = "merged";
				break;
			}
			case EAction::kDelete:
			{
				str = "deleted";
				break;
			}
			case EAction::kNone:
			{
				if (info.original_name != info.modified_name)
				{
					str = "renamed";
				}
				else if (info.original_id != info.modified_id)
				{
					str = "reordered";
				}
				break;
			}
			default:
			{
				break;
			}
		}
	}
	else if (columnId == 6) // modified ID
	{
		if (info.action != EAction::kDelete)
		{
			str = String(info.modified_id);
		}
	}
	else if (columnId == 7) // modified name
	{
		if (info.action != EAction::kDelete)
		{
			str = info.modified_name;
		}
	}

	// draw the text and the right-hand-side dividing line between cells
	if (str.isNotEmpty())
	{
		g.setColour(Colours::black);
		Rectangle<int> r(0, 0, width, height);
		g.drawFittedText(str, r.reduced(2), justification, 1 );
	}

	// draw the divider on the right side of the column
	g.setColour(Colours::black.withAlpha(0.5f));
	g.drawLine(width, 0, width, height);

	return;
}


void dm::ClassIdWnd::selectedRowsChanged(int rowNumber)
{
	if (rowNumber < 0					or
		rowNumber >= (int)vinfo.size()	)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	up_button	.setVisible(vinfo.size() > 1 and rowNumber >= 1);
	down_button	.setVisible(vinfo.size() > 1 and rowNumber + 1 < (int)vinfo.size());

	table.repaint();

	return;
}


void dm::ClassIdWnd::cellClicked(int rowNumber, int columnId, const MouseEvent & event)
{
	if (rowNumber < 0					or
		rowNumber >= (int)vinfo.size())
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	if (columnId == 5 or columnId == 6) // "action" or "new id"
	{
		auto & info = vinfo[rowNumber];
		std::string name = info.modified_name;
		if (name.empty()) // for example, a deleted class has no name
		{
			name = info.original_name;
		}
		if (name.empty()) // a newly-added row might not yet have a name
		{
			name = "#" + std::to_string(info.original_id);
		}

		PopupMenu m;
		m.addSectionHeader("\"" + name + "\"");
		m.addSeparator();

		// is there another class that is supposed to merge to this one?
		bool found_a_class_that_merges_to_this_one = false;
		for (size_t idx = 0; idx < vinfo.size(); idx ++)
		{
			if (rowNumber == (int)idx)
			{
				continue;
			}

			if (vinfo[idx].action == EAction::kMerge and
				vinfo[idx].modified_id == info.modified_id)
			{
				found_a_class_that_merges_to_this_one = true;
				break;
			}
		}

		if (found_a_class_that_merges_to_this_one)
		{
			m.addItem("cannot delete this class due to pending merge", false, false, std::function<void()>( []{return;} ));
		}
		else if (info.action == EAction::kDelete)
		{
			// this will clear the deletion flag
			m.addItem("delete this class"	, true, true, std::function<void()>( [=]() { this->vinfo[rowNumber].action = EAction::kNone; } ));
		}
		else
		{
			// this will set the deletion flag
			m.addItem("delete this class"	, true, false, std::function<void()>( [=]() { this->vinfo[rowNumber].action = EAction::kDelete; } ));
		}

		if (vinfo.size() > 1)
		{
			m.addSeparator();

			if (found_a_class_that_merges_to_this_one)
			{
				m.addItem("cannot merge this class due to pending merge", false, false, std::function<void()>( []{return;} ));
			}
			else
			{
				PopupMenu merged;

				for (size_t idx = 0; idx < vinfo.size(); idx ++)
				{
					if (rowNumber == (int)idx)
					{
						continue;
					}

					const auto & dst = vinfo[idx];

					if (dst.action != EAction::kNone)
					{
						// cannot merge to a class that is deleted or already merged
						continue;
					}

					if (info.action == EAction::kMerge and info.merge_to_name == dst.modified_name)
					{
						merged.addItem("merge with class #" + std::to_string(dst.modified_id) + " \"" + info.merge_to_name + "\"", true, true, std::function<void()>(
							[=]()
							{
								// un-merge
								this->vinfo[rowNumber].action = EAction::kNone;
								this->vinfo[rowNumber].merge_to_name.clear();
								this->vinfo[rowNumber].modified_name = this->vinfo[rowNumber].original_name;
								return;
							}));
					}
					else
					{
						merged.addItem("merge with class #" + std::to_string(dst.modified_id) + " \"" + dst.modified_name + "\"", true, false, std::function<void()>(
							[=]()
							{
								// merge
								this->vinfo[rowNumber].action = EAction::kMerge;
								this->vinfo[rowNumber].merge_to_name = dst.modified_name;
								this->vinfo[rowNumber].modified_name = dst.modified_name;
								return;
							}));
					}
				}
				m.addSubMenu("merge", merged);
			}
		}

		m.show();
		rebuild_table();
	}

	return;
}


Component * dm::ClassIdWnd::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component * existingComponentToUpdate)
{
	// remove whatever previously existed for a different row,
	// we'll create a new one specific to this row if needed
	delete existingComponentToUpdate;
	existingComponentToUpdate = nullptr;

	// rows are 0-based, columns are 1-based
	// column #7 is where they get to type a new name for the class
	if (rowNumber >= 0					and
		rowNumber < (int)vinfo.size()	and
		columnId == 7					and
		isRowSelected					and
		vinfo[rowNumber].action == EAction::kNone)
	{
		auto editor = new TextEditor("class name editor");
		editor->setColour(TextEditor::ColourIds::backgroundColourId, Colours::lightblue);
		editor->setColour(TextEditor::ColourIds::textColourId, Colours::black);
		editor->setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
		editor->setMouseClickGrabsKeyboardFocus(true);
		editor->setEscapeAndReturnKeysConsumed(false);
		editor->setReturnKeyStartsNewLine(false);
		editor->setTabKeyUsedAsCharacter(false);
		editor->setSelectAllWhenFocused(true);
		editor->setWantsKeyboardFocus(true);
		editor->setPopupMenuEnabled(false);
		editor->setScrollbarsShown(false);
		editor->setCaretVisible(true);
		editor->setMultiLine(false);
		editor->setText(vinfo[rowNumber].modified_name);
		editor->moveCaretToEnd();
		editor->selectAll();

		editor->onEscapeKey = [=]
		{
			// go back to the previous name
			editor->setText(vinfo[rowNumber].modified_name);
			this->rebuild_table();
		};
		editor->onFocusLost = [=]
		{
			auto str = editor->getText().trim().toStdString();
			if (str.empty())
			{
				str = this->vinfo[rowNumber].modified_name;
				if (str.empty())
				{
					str = this->vinfo[rowNumber].original_name;
				}
			}

			this->vinfo[rowNumber].modified_name = str;

			// see if there are other classes which are merged here, in which case we need to update their names
			for (size_t idx = 0; idx < this->vinfo.size(); idx ++)
			{
				auto & dst = this->vinfo[idx];
				if (dst.action == EAction::kMerge and
					dst.modified_id == this->vinfo[rowNumber].modified_id)
				{
					dst.merge_to_name = str;
					dst.modified_name = str;
				}
			}

			this->rebuild_table();
		};
		editor->onReturnKey = editor->onFocusLost;

		existingComponentToUpdate = editor;
	}

	return existingComponentToUpdate;
}


String dm::ClassIdWnd::getCellTooltip(int rowNumber, int columnId)
{
	String str = "Click on \"action\" or \"new id\" columns to delete or merge.";

	if (error_count > 0)
	{
		str = "Errors detected while processing annotations.  See log file for details.";
	}
	else if (columnId == 3)
	{
		str = "The number of .txt files which reference this class ID.";
	}
	else if (columnId == 4)
	{
		str = "The total number of annotations for this class ID.";
	}

	return str;
}


void dm::ClassIdWnd::rebuild_table()
{
	dm::Log("rebuilding table");

	int next_class_id = 0;

	// assign the ID to be used by each class
	for (size_t idx = 0; idx < vinfo.size(); idx ++)
	{
		auto & info = vinfo[idx];

		if (info.action == EAction::kDelete)
		{
			dm::Log("class \"" + info.modified_name + "\" has been deleted");
			info.modified_id = -1;
		}
		else if (info.action == EAction::kMerge)
		{
			dm::Log("class \"" + info.original_name + "\" has been merged to \"" + info.merge_to_name + "\", will need to revisit once IDs have been assigned");
			info.modified_id = -1;
		}
		else
		{
			dm::Log("class \"" + info.modified_name + "\" has been assigned id #" + std::to_string(next_class_id));
			info.modified_id = next_class_id ++;
		}
	}

	// now that IDs have been assigned we need to look at merge entries and figure out the right ID to use
	for (size_t idx = 0; idx < vinfo.size(); idx ++)
	{
		auto & info = vinfo[idx];

		if (info.action == EAction::kMerge)
		{
			// look for the non-merge entry that has this name
			for (size_t tmp = 0; tmp < vinfo.size(); tmp ++)
			{
				if (vinfo[tmp].action != EAction::kMerge and vinfo[tmp].modified_name == info.merge_to_name)
				{
					// we found the item we need to merge to!
					info.modified_id = vinfo[tmp].modified_id;
					dm::Log("class \"" + info.original_name + "\" is merged with #" + std::to_string(info.modified_id) + " \"" + info.modified_name + "\"");
					break;
				}
			}
		}
	}

	// see if we have any changes to apply
	bool changes_to_apply = false;
	for (size_t idx = 0; idx < vinfo.size(); idx ++)
	{
		auto & info = vinfo[idx];
		if (info.original_id != info.modified_id or
			info.original_name != info.modified_name)
		{
			changes_to_apply = true;
			break;
		}
	}

//	dm::Log("done=" + std::to_string(done_looking_for_images) + " error_count=" + std::to_string(error_count) + " next_class_id=" + std::to_string(next_class_id));

	export_button	.setEnabled(done_looking_for_images and error_count == 0 and next_class_id > 0);
	apply_button	.setEnabled(done_looking_for_images and error_count == 0 and next_class_id > 0 and changes_to_apply);

	table.updateContent();
	table.repaint();

	return;
}


void dm::ClassIdWnd::count_images_and_marks()
{
	// this is started on a secondary thread

	try
	{
		error_count = 0;

		VStr image_filenames;
		VStr ignored_filenames;

		int previous_percentage = -1;
		const auto export_button_text = export_button.getButtonText();
		export_button.setButtonText("Verifying...");

		find_files(dir, image_filenames, ignored_filenames, ignored_filenames, done);
		ignored_filenames.clear();

		dm::Log("counting thread: done=" + std::to_string(done) + ": number of images found in " + dir.getFullPathName().toStdString() + ": " + std::to_string(image_filenames.size()));

		for (size_t idx = 0; idx < image_filenames.size() and not done and not threadShouldExit(); idx ++)
		{
			// don't bother updating the button if there is a trivial number of images
			if (image_filenames.size() > 100)
			{
				const int percentage = std::round(idx * 100.0f / image_filenames.size());
				if (percentage != previous_percentage)
				{
					export_button.setButtonText("Verifying " + std::to_string(percentage) + "% ...");
					previous_percentage = percentage;
				}
			}

			auto & fn = image_filenames[idx];
			File file = File(fn).withFileExtension(".txt");
			if (file.exists())
			{
				std::set<int> classes_found;

				size_t line_number = 0;
				std::ifstream ifs(file.getFullPathName().toStdString());
				while (not threadShouldExit())
				{
					int class_id = -1;
					double x = -1.0;
					double y = -1.0;
					double w = -1.0;
					double h = -1.0;

					line_number ++;
					ifs >> class_id >> x >> y >> w >> h;

					if (not ifs.good())
					{
						break;
					}

					if (class_id < 0 or class_id >= (int)vinfo.size())
					{
						error_count ++;
						dm::Log("ERROR: class #" + std::to_string(class_id) + " in " + file.getFullPathName().toStdString() + " on line #" + std::to_string(line_number) + " is invalid");
					}
					else if (
						x <= 0.0 or
						y <= 0.0 or
						w <= 0.0 or
						h <= 0.0)
					{
						// ignore these errors...we're not changing the coordinates of anything just renumbering and merging things together
//						error_count ++;
						dm::Log("WARNING: coordinates for class #" + std::to_string(class_id) + " in " + file.getFullPathName().toStdString() + " on line #" + std::to_string(line_number) + " are invalid");
					}
					else if (
						// take into account rounding errors, especially when converting coordinates between float and double
						x - w / 2.0 < -0.000001 or
						x + w / 2.0 >  1.000001 or
						y - h / 2.0 < -0.000001 or
						y + h / 2.0 >  1.000001)
					{
						// ignore these errors...we're not changing the coordinates of anything just renumbering and merging things together
//						error_count ++;
						dm::Log("WARNING: width or height for class #" + std::to_string(class_id) + " in " + file.getFullPathName().toStdString() + " on line #" + std::to_string(line_number) + " is invalid");
					}
					else
					{
						classes_found.insert(class_id);
						count_annotations_per_class[class_id] ++;
					}
				}

				for (const int id : classes_found)
				{
					count_files_per_class[id] ++;
				}
			}
		}

		// display a bit of information on all the classes and annotations we found
		for (const auto & [k, v] : count_files_per_class)
		{
			dm::Log("-> class #" + std::to_string(k) + ": " + std::to_string(v) + " files");
		}

		for (const auto & [k, v] : count_annotations_per_class)
		{
			dm::Log("-> class #" + std::to_string(k) + ": " + std::to_string(v) + " total annotations");
		}

		export_button.setButtonText(export_button_text);

		if (error_count)
		{
			dm::Log("-> errors found: " + std::to_string(error_count));
			export_button.setTooltip("Cannot export dataset due to errors.  See log file for details.");
			apply_button.setTooltip("Cannot apply changes due to errors.  See log file for details.");
		}
		else
		{
			all_images.swap(image_filenames);
			done_looking_for_images = true;
			rebuild_table();
		}
	}
	catch(const std::exception & e)
	{
		dm::Log(std::string("counting thread exception: ") + e.what());
		error_count ++;
	}
	catch(...)
	{
		dm::Log("unknown exception caught in counting thread");
		error_count ++;
	}

	dm::Log("counting thread is exiting"
		", done="				+ std::to_string(done)					+
		", threadShouldExit="	+ std::to_string(threadShouldExit())	+
		", errors="				+ std::to_string(error_count)			+
		", images="				+ std::to_string(all_images.size())		);

	return;
}


void dm::ClassIdWnd::run_export_coco()
{
	const std::filesystem::path source = dir.getFullPathName().toStdString();
	std::string export_suffix = "_coco_export_";
	if (export_with_split)
	{
		export_suffix = "_coco_export_split_";
	}
	const std::filesystem::path target = (source.string() + export_suffix + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S").toStdString());
	export_directory = target;

	Log("COCO export dataset src=" + source.string());
	Log("COCO export dataset dst=" + target.string());

	setStatusMessage("Exporting files to COCO format: " + target.string() + "...");

	// Create COCO directory structure
	const std::filesystem::path train_images_dir = target / "train2017";
	const std::filesystem::path val_images_dir = target / "val2017";
	const std::filesystem::path annotations_dir = target / "annotations";

	std::error_code ec;
	std::filesystem::create_directories(train_images_dir, ec);
	std::filesystem::create_directories(val_images_dir, ec);
	std::filesystem::create_directories(annotations_dir, ec);

	if (ec)
	{
		Log("Failed to create COCO directory structure: " + ec.message());
		return;
	}

	// Read class names from .names file
	std::vector<std::string> class_names;
	std::ifstream ifs(names_fn);
	if (ifs.good())
	{
		std::string line;
		while (std::getline(ifs, line))
		{
			// Remove any trailing whitespace
			while (!line.empty() && std::isspace(line.back()))
			{
				line.pop_back();
			}
			if (!line.empty())
			{
				class_names.push_back(line);
			}
		}
		ifs.close();
	}
	else
	{
		Log("Warning: Could not read .names file for COCO export");
		return;
	}

	// Analyze and pair image/label files
	std::map<std::string, std::filesystem::path> image_map;
	std::map<std::string, std::filesystem::path> label_map;

	// Build maps of normalized keys to full paths
	for (const auto& image_path : all_images)
	{
		std::filesystem::path img_path(image_path);
		std::filesystem::path rel_path = std::filesystem::relative(img_path, source);
		
		// Use relative path without extension as key to avoid collisions between train/val folders
		std::string key = rel_path.replace_extension("").string();
		
		// Simple approach: use filename without extension as key
		image_map[key] = img_path;
	}

	// Find corresponding label files
	for (const auto& [key, image_path] : image_map)
	{
		std::filesystem::path txt_path = std::filesystem::path(image_path).replace_extension(".txt");
		if (std::filesystem::exists(txt_path))
		{
			label_map[key] = txt_path;
		}
	}

	// Separate train and val images
	std::vector<std::pair<std::string, std::filesystem::path>> train_images, val_images;
	std::vector<std::pair<std::string, std::filesystem::path>> all_valid_images;
	
	for (const auto& [key, image_path] : image_map)
	{
		if (export_all_images || label_map.count(key))
		{
			all_valid_images.push_back({key, image_path});
		}
	}

	std::optional<int> actual_seed_used = export_seed; // Track the actual seed used
	if (export_with_split)
	{
		// Shuffle for split
		std::mt19937 g;
		if (export_seed.has_value())
		{
			g.seed(*export_seed);
			Log("Using deterministic seed " + std::to_string(*export_seed) + " for COCO export split");
		}
		else
		{
			std::random_device rd;
			int random_seed = rd();
			g.seed(random_seed);
			actual_seed_used = random_seed; // Capture the generated random seed
			Log("Using random seed " + std::to_string(random_seed) + " for COCO export split");
		}
		std::shuffle(all_valid_images.begin(), all_valid_images.end(), g);
		
		// Split based on percentage
		const size_t total_images = all_valid_images.size();
		const size_t train_count = static_cast<size_t>(total_images * train_percentage / 100.0);
		
		for (size_t i = 0; i < all_valid_images.size(); ++i)
		{
			if (i < train_count)
			{
				train_images.push_back(all_valid_images[i]);
			}
			else
			{
				val_images.push_back(all_valid_images[i]);
			}
		}
		
		Log("COCO split: " + std::to_string(train_images.size()) + " train, " + std::to_string(val_images.size()) + " val");
	}
	else
	{
		// Use path-based detection (existing behavior)
		for (const auto& [key, image_path] : all_valid_images)
		{
			std::string path_str = image_path.string();
			if (path_str.find("val") != std::string::npos || path_str.find("valid") != std::string::npos)
			{
				val_images.push_back({key, image_path});
			}
			else
			{
				train_images.push_back({key, image_path});
			}
		}
	}

	// Sort images for consistent ordering
	std::sort(train_images.begin(), train_images.end(), 
		[](const auto& a, const auto& b) { return a.first < b.first; });
	std::sort(val_images.begin(), val_images.end(), 
		[](const auto& a, const auto& b) { return a.first < b.first; });

	double work_completed = 0.0f;
	const double work_to_be_done = train_images.size() + val_images.size();

	// Generate COCO JSON files
	generate_coco_json(train_images, label_map, class_names, train_images_dir, annotations_dir / "instances_train2017.json", "train", source, work_completed, work_to_be_done, export_with_split, train_percentage, actual_seed_used);
	generate_coco_json(val_images, label_map, class_names, val_images_dir, annotations_dir / "instances_val2017.json", "val", source, work_completed, work_to_be_done, export_with_split, train_percentage, actual_seed_used);

	return;
}


void dm::ClassIdWnd::run_export_dfine()
{
	/* D-FINE format creates a flat directory structure:
	 *
	 *		project_dfine_export_YYYY-MM-DD_HH-MM-SS/
	 *		├── images/         <- All images flattened here
	 *		│   ├── img001.jpg
	 *		│   ├── img002.jpg
	 *		│   └── ...
	 *		├── labels/         <- All labels flattened here
	 *		│   ├── img001.txt
	 *		│   ├── img002.txt
	 *		│   └── ...
	 *		├── train.csv       <- List of training image filenames
	 *		├── val.csv         <- List of validation image filenames
	 *		└── classes.txt     <- Class names
	 */

	const std::filesystem::path source = dir.getFullPathName().toStdString();
	std::string export_suffix = "_dfine_export_";
	if (export_with_split)
	{
		export_suffix = "_dfine_export_split_";
	}
	const std::filesystem::path target = (source.string() + export_suffix + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S").toStdString());
	export_directory = target;

	Log("D-FINE export dataset src=" + source.string());
	Log("D-FINE export dataset dst=" + target.string());

	setStatusMessage("Exporting files to D-FINE format: " + target.string() + "...");

	// Create directories
	std::filesystem::path images_dir = target / "images";
	std::filesystem::path labels_dir = target / "labels";

	std::filesystem::create_directories(images_dir);
	std::filesystem::create_directories(labels_dir);

	// Read class names from .names file
	std::vector<std::string> class_names;
	{
		std::ifstream names_ifs(names_fn);
		std::string line;
		while (std::getline(names_ifs, line))
		{
			if (!line.empty())
			{
				class_names.push_back(line);
			}
		}
	}

	// Build map of all label files for quick lookup
	std::map<std::string, std::filesystem::path> label_map;
	for (const auto& image_path : all_images)
	{
		std::filesystem::path txt_path = std::filesystem::path(image_path).replace_extension(".txt");
		if (std::filesystem::exists(txt_path) && std::filesystem::file_size(txt_path) > 0)
		{
			std::string key = std::filesystem::path(image_path).stem().string();
			label_map[key] = txt_path;
		}
	}

	// Build list of all valid images
	std::vector<std::pair<std::string, std::filesystem::path>> all_valid_images;
	for (const auto& image_path : all_images)
	{
		std::string key = std::filesystem::path(image_path).stem().string();
		if (export_all_images || label_map.count(key))
		{
			all_valid_images.push_back({key, image_path});
		}
	}

	// Split images into train and val sets
	std::vector<std::pair<std::string, std::filesystem::path>> train_images;
	std::vector<std::pair<std::string, std::filesystem::path>> val_images;
	std::optional<int> actual_seed_used = export_seed;

	if (export_with_split)
	{
		// Shuffle and split based on train_percentage
		std::default_random_engine g;
		if (export_seed.has_value())
		{
			g.seed(*export_seed);
			Log("Using deterministic seed " + std::to_string(*export_seed) + " for D-FINE export split");
		}
		else
		{
			std::random_device rd;
			int random_seed = rd();
			g.seed(random_seed);
			actual_seed_used = random_seed;
			Log("Using random seed " + std::to_string(random_seed) + " for D-FINE export split");
		}

		std::shuffle(all_valid_images.begin(), all_valid_images.end(), g);

		size_t train_count = static_cast<size_t>(std::round(all_valid_images.size() * train_percentage / 100.0));
		for (size_t i = 0; i < all_valid_images.size(); i++)
		{
			if (i < train_count)
			{
				train_images.push_back(all_valid_images[i]);
			}
			else
			{
				val_images.push_back(all_valid_images[i]);
			}
		}

		Log("D-FINE split: " + std::to_string(train_images.size()) + " train, " + std::to_string(val_images.size()) + " val");
	}
	else
	{
		// Use path-based detection (existing behavior)
		for (const auto& [key, image_path] : all_valid_images)
		{
			std::string path_str = image_path.string();
			if (path_str.find("val") != std::string::npos || path_str.find("valid") != std::string::npos)
			{
				val_images.push_back({key, image_path});
			}
			else
			{
				train_images.push_back({key, image_path});
			}
		}
	}

	double work_completed = 0.0f;
	const double work_to_be_done = train_images.size() + val_images.size();

	// CSV entries
	std::vector<std::string> train_csv_entries;
	std::vector<std::string> val_csv_entries;

	// Process training images
	setStatusMessage("Exporting training images...");
	for (const auto& [key, image_path] : train_images)
	{
		if (threadShouldExit())
		{
			return;
		}

		work_completed++;
		setProgress(work_completed / work_to_be_done);

		std::string unique_name = generate_unique_filename(image_path, source);
		std::filesystem::path src_ext = std::filesystem::path(image_path).extension();
		std::string output_image_name = unique_name + src_ext.string();
		std::string output_label_name = unique_name + ".txt";

		// Copy image
		std::filesystem::copy_file(image_path, images_dir / output_image_name, std::filesystem::copy_options::overwrite_existing);
		number_of_files_copied++;

		// Copy label if exists
		if (label_map.count(key))
		{
			std::filesystem::copy_file(label_map[key], labels_dir / output_label_name, std::filesystem::copy_options::overwrite_existing);
		}
		else
		{
			// Create empty label file for negative samples
			std::ofstream ofs(labels_dir / output_label_name);
			ofs.close();
		}

		train_csv_entries.push_back(output_image_name);
	}

	// Process validation images
	setStatusMessage("Exporting validation images...");
	for (const auto& [key, image_path] : val_images)
	{
		if (threadShouldExit())
		{
			return;
		}

		work_completed++;
		setProgress(work_completed / work_to_be_done);

		std::string unique_name = generate_unique_filename(image_path, source);
		std::filesystem::path src_ext = std::filesystem::path(image_path).extension();
		std::string output_image_name = unique_name + src_ext.string();
		std::string output_label_name = unique_name + ".txt";

		// Copy image
		std::filesystem::copy_file(image_path, images_dir / output_image_name, std::filesystem::copy_options::overwrite_existing);
		number_of_files_copied++;

		// Copy label if exists
		if (label_map.count(key))
		{
			std::filesystem::copy_file(label_map[key], labels_dir / output_label_name, std::filesystem::copy_options::overwrite_existing);
		}
		else
		{
			// Create empty label file for negative samples
			std::ofstream ofs(labels_dir / output_label_name);
			ofs.close();
		}

		val_csv_entries.push_back(output_image_name);
	}

	// Write train.csv
	{
		std::ofstream ofs(target / "train.csv");
		for (const auto& entry : train_csv_entries)
		{
			ofs << entry << "\n";
		}
		Log("Generated train.csv with " + std::to_string(train_csv_entries.size()) + " entries");
	}

	// Write val.csv
	{
		std::ofstream ofs(target / "val.csv");
		for (const auto& entry : val_csv_entries)
		{
			ofs << entry << "\n";
		}
		Log("Generated val.csv with " + std::to_string(val_csv_entries.size()) + " entries");
	}

	// Write classes.txt
	{
		std::ofstream ofs(target / "classes.txt");
		for (const auto& name : class_names)
		{
			ofs << name << "\n";
		}
		Log("Generated classes.txt with " + std::to_string(class_names.size()) + " classes");
	}

	Log("D-FINE export complete: " + std::to_string(train_csv_entries.size()) + " train, " + std::to_string(val_csv_entries.size()) + " val images");

	return;
}


void dm::ClassIdWnd::generate_coco_json(const std::vector<std::pair<std::string, std::filesystem::path>>& images, 
										const std::map<std::string, std::filesystem::path>& label_map,
										const std::vector<std::string>& class_names,
										const std::filesystem::path& target_img_dir,
										const std::filesystem::path& json_path,
										const std::string& mode,
										const std::filesystem::path& source,
										double& work_completed,
										const double work_to_be_done,
										bool with_split,
										double train_percentage,
										const std::optional<int>& seed)
{
	// Create COCO JSON structure
	std::ostringstream json_stream;
	json_stream << std::fixed << std::setprecision(10); // Preserve floating point precision
	json_stream << "{\n";
	
	// Info section
	auto now = Time::getCurrentTime();
	json_stream << "  \"info\": {\n";
	json_stream << "    \"year\": " << now.getYear() << ",\n";
	json_stream << "    \"version\": \"1.0\",\n";
	json_stream << "    \"description\": \"Dataset exported from DarkMark v" << DARKMARK_VERSION << "\",\n";
	json_stream << "    \"contributor\": \"DarkMark\",\n";
	json_stream << "    \"url\": \"https://github.com/stephanecharette/DarkMark\",\n";
	json_stream << "    \"date_created\": \"" << now.formatted("%Y-%m-%d").toStdString() << "\"";
	
	// Add split information if applicable
	if (with_split)
	{
		json_stream << ",\n";
		json_stream << "    \"split\": {\n";
		json_stream << "      \"enabled\": true,\n";
		json_stream << "      \"train_percentage\": " << std::fixed << std::setprecision(1) << train_percentage << ",\n";
		json_stream << "      \"seed\": " << seed.value() << ",\n";
		json_stream << "      \"seed_note\": \"Use this seed for reproducible splits\"\n";
		json_stream << "    }";
	}
	
	json_stream << "\n";
	json_stream << "  },\n";
	
	// Licenses section
	json_stream << "  \"licenses\": [\n";
	json_stream << "    {\n";
	json_stream << "      \"id\": 1,\n";
	json_stream << "      \"name\": \"Apache License v2.0\",\n";
	json_stream << "      \"url\": \"https://www.apache.org/licenses/LICENSE-2.0\"\n";
	json_stream << "    }\n";
	json_stream << "  ],\n";
	
	// Categories section
	json_stream << "  \"categories\": [\n";
	for (size_t i = 0; i < class_names.size(); ++i)
	{
		json_stream << "    {\n";
		json_stream << "      \"id\": " << (i + 1) << ",\n";
		json_stream << "      \"name\": \"" << class_names[i] << "\",\n";
		json_stream << "      \"supercategory\": null\n";
		json_stream << "    }";
		if (i < class_names.size() - 1) json_stream << ",";
		json_stream << "\n";
	}
	json_stream << "  ],\n";
	
	// Images and annotations sections
	json_stream << "  \"images\": [\n";
	
	std::vector<std::string> annotations_json;
	size_t annotation_id = 1;
	
	for (size_t img_idx = 0; img_idx < images.size(); ++img_idx)
	{
		if (threadShouldExit()) break;
		
		setProgress(work_completed / work_to_be_done);
		work_completed++;
		
		const auto& [key, image_path] = images[img_idx];
		
		// Read image to get dimensions
		Image juce_image = ImageFileFormat::loadFrom(File(image_path.string()));
		if (!juce_image.isValid())
		{
			Log("Warning: Could not read image " + image_path.string() + ". Skipping.");
			continue;
		}
		
		int width = juce_image.getWidth();
		int height = juce_image.getHeight();
		
		// Generate unique filename and copy image to target directory
		std::string filename = generate_unique_filename(image_path, source);
		std::filesystem::path dest_img_path = target_img_dir / filename;
		std::error_code ec;
		std::filesystem::copy_file(image_path, dest_img_path, ec);
		if (ec)
		{
			Log("Failed to copy image " + image_path.string() + ": " + ec.message());
			continue;
		}
		
		number_of_files_copied++;
		
		// Add image info to JSON
		json_stream << "    {\n";
		json_stream << "      \"id\": " << (img_idx + 1) << ",\n";
		json_stream << "      \"file_name\": \"" << filename << "\",\n";
		json_stream << "      \"height\": " << height << ",\n";
		json_stream << "      \"width\": " << width << ",\n";
		json_stream << "      \"license\": null,\n";
		json_stream << "      \"date_captured\": null\n";
		json_stream << "    }";
		if (img_idx < images.size() - 1) json_stream << ",";
		json_stream << "\n";
		
		// Process annotations for this image
		auto label_it = label_map.find(key);
		if (label_it != label_map.end())
		{
			std::ifstream label_file(label_it->second);
			if (label_file.good())
			{
				std::string line;
				while (std::getline(label_file, line))
				{
					if (line.empty()) continue;
					
					std::istringstream iss(line);
					int class_id;
					double cx, cy, w, h;
					
					if (iss >> class_id >> cx >> cy >> w >> h)
					{
						// Convert from YOLO format (normalized) to COCO format (absolute)
						double abs_cx = cx * width;
						double abs_cy = cy * height;
						double abs_w = w * width;
						double abs_h = h * height;
						
						double x0 = std::max(abs_cx - abs_w / 2.0, 0.0);
						double y0 = std::max(abs_cy - abs_h / 2.0, 0.0);
						double x1 = std::min(x0 + abs_w, (double)width);
						double y1 = std::min(y0 + abs_h, (double)height);
						
						double bbox_w = x1 - x0;
						double bbox_h = y1 - y0;
						double area = bbox_w * bbox_h;
						
						// Create annotation JSON
						std::ostringstream ann_stream;
						ann_stream << std::fixed << std::setprecision(10); // Preserve floating point precision
						ann_stream << "    {\n";
						ann_stream << "      \"id\": " << annotation_id++ << ",\n";
						ann_stream << "      \"image_id\": " << (img_idx + 1) << ",\n";
						ann_stream << "      \"category_id\": " << (class_id + 1) << ",\n";
						ann_stream << "      \"bbox\": [" << x0 << ", " << y0 << ", " << bbox_w << ", " << bbox_h << "],\n";
						ann_stream << "      \"area\": " << area << ",\n";
						ann_stream << "      \"iscrowd\": 0\n";
						ann_stream << "    }";
						
						annotations_json.push_back(ann_stream.str());
					}
				}
				label_file.close();
			}
		}
	}
	
	json_stream << "  ],\n";
	
	// Annotations section
	json_stream << "  \"annotations\": [\n";
	for (size_t i = 0; i < annotations_json.size(); ++i)
	{
		json_stream << annotations_json[i];
		if (i < annotations_json.size() - 1) json_stream << ",";
		json_stream << "\n";
	}
	json_stream << "  ]\n";
	
	json_stream << "}\n";
	
	// Write JSON file
	std::ofstream json_file(json_path);
	if (json_file.good())
	{
		json_file << json_stream.str();
		json_file.close();
		Log("Generated " + mode + " COCO JSON with " + std::to_string(images.size()) + " images and " + std::to_string(annotations_json.size()) + " annotations");
	}
	else
	{
		Log("Error: Failed to write COCO JSON file: " + json_path.string());
	}
}


void dm::ClassIdWnd::generate_dataset_yaml(const std::filesystem::path & output_folder, bool with_split, double train_percentage, const std::optional<int>& seed)
{
	// Read class names from .names file
	std::vector<std::string> class_names;
	std::ifstream ifs(names_fn);
	if (ifs.good())
	{
		std::string line;
		while (std::getline(ifs, line))
		{
			// Remove any trailing whitespace
			while (!line.empty() && std::isspace(line.back()))
			{
				line.pop_back();
			}
			if (!line.empty())
			{
				class_names.push_back(line);
			}
		}
		ifs.close();
	}
	else
	{
		Log("Warning: Could not read .names file for dataset.yaml generation");
		return;
	}

	// Generate dataset.yaml
	std::filesystem::path yaml_path = output_folder / "dataset.yaml";
	std::ofstream ofs(yaml_path);
	if (ofs.good())
	{
		// Write YAML content
		ofs << "# YOLOv5 Dataset Configuration\n";
		ofs << "# Generated by DarkMark v" << DARKMARK_VERSION << "\n";
		ofs << "# " << Time::getCurrentTime().formatted("%a %Y-%m-%d %H:%M:%S %Z").toStdString() << "\n\n";
		
		ofs << "path: " << std::filesystem::absolute(output_folder).string() << "\n";
		ofs << "train: images/train\n";
		ofs << "val: images/val\n\n";
		
		// Add split information if applicable
		if (with_split)
		{
			ofs << "# Split configuration\n";
			ofs << "split:\n";
			ofs << "  enabled: true\n";
			ofs << "  train_percentage: " << std::fixed << std::setprecision(1) << train_percentage << "\n";
			ofs << "  seed: " << seed.value() << "  # Use this seed for reproducible splits\n";
			ofs << "\n";
		}
		
		ofs << "names:\n";
		for (size_t i = 0; i < class_names.size(); ++i)
		{
			ofs << "  " << i << ": " << class_names[i] << "\n";
		}
		
		ofs.close();
		Log("Generated dataset.yaml with " + std::to_string(class_names.size()) + " classes");
	}
	else
	{
		Log("Error: Failed to write dataset.yaml file");
	}
}


void dm::ClassIdWnd::exportDialogFinished(bool wasOkPressed, const ExportDialog* dialog)
{
	// Re-enable the window
	setEnabled(true);
	
	if (wasOkPressed)
	{
		export_all_images = dialog->getExportAllImages();
		export_yolov5_format = dialog->getExportYolov5Format();
		export_coco_format = dialog->getExportCocoFormat();
		export_dfine_format = dialog->getExportDfineFormat();
		export_with_split = dialog->getExportWithSplit();
		
		if (export_with_split)
		{
			train_percentage = dialog->getTrainPercentage();
			if (dialog->hasSeed())
			{
				export_seed = dialog->getSeed();
				dm::Log("Export with split: " + std::to_string(train_percentage) + "% train, seed: " + std::to_string(*export_seed));
			}
			else
			{
				export_seed = std::nullopt;
				dm::Log("Export with split: " + std::to_string(train_percentage) + "% train, random seed");
			}
		}
		else
		{
			dm::Log("Export without split");
		}
		
		// Now that all data is retrieved, it's safe to reset the dialog
		export_dialog.reset();
		
		is_exporting = true;
		runThread(); // calls run() and waits for it to be done
		dm::Log("forcing the window to close");
		closeButtonPressed();
	}
}



