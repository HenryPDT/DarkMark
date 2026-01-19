// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include <random>
#include <magic.h>


dm::UnifiedPredictionResult::UnifiedPredictionResult(const DarkHelp::PredictionResult& dh_result, const VStr& names)
	: rect(dh_result.rect)
	, probability(dh_result.best_probability)
	, class_idx(dh_result.best_class)
	, name(names.at(dh_result.best_class))
{
}

dm::UnifiedPredictionResult::UnifiedPredictionResult(const OnnxHelp::PredictionResult& onnx_result)
	: rect(onnx_result.rect)
	, probability(onnx_result.probability)
	, class_idx(onnx_result.class_idx)
	, name(onnx_result.name)
{
}


dm::VideoImportWindow::VideoImportWindow(const std::string & dir, const VStr & v) :
	DocumentWindow("DarkMark - Import Video Frames", Colours::darkgrey, TitleBarButtons::closeButton),
	ThreadWithProgressWindow("DarkMark", true, true							),
	base_directory			(dir											),
	filenames				(v												),
	tb_extract_all			("extract all frames"							),
	tb_extract_sequences	("extract sequences of consecutive frames"		),
	sl_sequences			(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	txt_sequences			("", "sequences"								),
	sl_consecutive_frames	(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	txt_consecutive_frames	("", "consecutive frames"						),
	tb_extract_maximum		("maximum number of random frames to extract:"	),
	sl_maximum				(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	tb_extract_percentage	("percentage of random frames to extract:"		),
	sl_percentage			(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	tb_do_not_resize		("do not resize frames"							),
	tb_resize				("resize frames:"								),
	txt_x					("", "x"										),
	tb_keep_aspect_ratio	("maintain aspect ratio"						),
	tb_force_resize			("resize to exact dimensions"					),
	tb_save_as_png			("save as PNG"									),
	tb_save_as_jpeg			("save as JPEG"									),
	txt_jpeg_quality		("", "image quality:"							),
	sl_jpeg_quality			(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	cancel					("Cancel"),
	ok						("Import"),
	tb_enable_auto_annotation("Enable auto-annotation"),
	tb_model_type_darknet("Darknet model"),
	tb_model_type_onnx("ONNX model"),
	btn_select_darknet_weights("Select .weights file"),
	btn_select_darknet_cfg("Select .cfg file"),
	btn_select_darknet_names("Select .names file"),
	btn_select_onnx_model("Select .onnx file"),
	btn_select_onnx_names("Select .names file"),
	tb_import_with_detections("Import only frames with detections"),
	tb_import_without_detections("Import only frames without detections"),
	tb_import_all_frames("Import all frames regardless of detections"),
	txt_confidence_threshold("", "Confidence threshold:"),
	sl_confidence_threshold(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	txt_nms_threshold("", "NMS threshold:"),
	sl_nms_threshold(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	tb_enable_tiling("Enable image tiling"),
	txt_hierarchy_threshold("", "Hierarchy threshold:"),
	sl_hierarchy_threshold(Slider::SliderStyle::LinearHorizontal, Slider::TextEntryBoxPosition::TextBoxRight),
	extra_lines_needed(0),
	number_of_processed_frames(0)
{
	setContentNonOwned		(&canvas, true	);
	setUsingNativeTitleBar	(true			);
	setResizable			(true, false	);
	setDropShadowEnabled	(true			);

	canvas.addAndMakeVisible(header_message			);
	canvas.addAndMakeVisible(tb_extract_all			);
	canvas.addAndMakeVisible(tb_extract_sequences	);
	canvas.addAndMakeVisible(sl_sequences			);
	canvas.addAndMakeVisible(txt_sequences			);
	canvas.addAndMakeVisible(sl_consecutive_frames	);
	canvas.addAndMakeVisible(txt_consecutive_frames	);
	canvas.addAndMakeVisible(tb_extract_maximum		);
	canvas.addAndMakeVisible(sl_maximum				);
	canvas.addAndMakeVisible(tb_extract_percentage	);
	canvas.addAndMakeVisible(sl_percentage			);
	canvas.addAndMakeVisible(tb_do_not_resize		);
	canvas.addAndMakeVisible(tb_resize				);
	canvas.addAndMakeVisible(ef_width				);
	canvas.addAndMakeVisible(txt_x					);
	canvas.addAndMakeVisible(ef_height				);
	canvas.addAndMakeVisible(tb_keep_aspect_ratio	);
	canvas.addAndMakeVisible(tb_force_resize		);
	canvas.addAndMakeVisible(tb_save_as_png			);
	canvas.addAndMakeVisible(tb_save_as_jpeg		);
	canvas.addAndMakeVisible(txt_jpeg_quality		);
	canvas.addAndMakeVisible(sl_jpeg_quality		);
	canvas.addAndMakeVisible(cancel					);
	canvas.addAndMakeVisible(ok						);

	canvas.addAndMakeVisible(tb_enable_auto_annotation);
	canvas.addAndMakeVisible(tb_model_type_darknet);
	canvas.addAndMakeVisible(tb_model_type_onnx);
	canvas.addAndMakeVisible(btn_select_darknet_weights);
	canvas.addAndMakeVisible(btn_select_darknet_cfg);
	canvas.addAndMakeVisible(btn_select_darknet_names);
	canvas.addAndMakeVisible(lbl_darknet_weights);
	canvas.addAndMakeVisible(lbl_darknet_cfg);
	canvas.addAndMakeVisible(lbl_darknet_names);
	canvas.addAndMakeVisible(btn_select_onnx_model);
	canvas.addAndMakeVisible(btn_select_onnx_names);
	canvas.addAndMakeVisible(lbl_onnx_model);
	canvas.addAndMakeVisible(lbl_onnx_names);
	canvas.addAndMakeVisible(tb_import_with_detections);
	canvas.addAndMakeVisible(tb_import_without_detections);
	canvas.addAndMakeVisible(tb_import_all_frames);
	canvas.addAndMakeVisible(txt_confidence_threshold);
	canvas.addAndMakeVisible(sl_confidence_threshold);
	canvas.addAndMakeVisible(txt_nms_threshold);
	canvas.addAndMakeVisible(sl_nms_threshold);
	canvas.addAndMakeVisible(tb_enable_tiling);
	canvas.addAndMakeVisible(txt_hierarchy_threshold);
	canvas.addAndMakeVisible(sl_hierarchy_threshold);

	tb_extract_all			.setRadioGroupId(1);
	tb_extract_sequences	.setRadioGroupId(1);
	tb_extract_maximum		.setRadioGroupId(1);
	tb_extract_percentage	.setRadioGroupId(1);

	tb_do_not_resize		.setRadioGroupId(2);
	tb_resize				.setRadioGroupId(2);

	tb_keep_aspect_ratio	.setRadioGroupId(3);
	tb_force_resize			.setRadioGroupId(3);

	tb_save_as_png			.setRadioGroupId(4);
	tb_save_as_jpeg			.setRadioGroupId(4);

	tb_model_type_darknet.setRadioGroupId(5);
	tb_model_type_onnx.setRadioGroupId(5);

	tb_import_with_detections.setRadioGroupId(6);
	tb_import_without_detections.setRadioGroupId(6);
	tb_import_all_frames.setRadioGroupId(6);

	tb_extract_all			.addListener(this);
	tb_extract_sequences	.addListener(this);
	tb_extract_maximum		.addListener(this);
	tb_extract_percentage	.addListener(this);
	tb_do_not_resize		.addListener(this);
	tb_resize				.addListener(this);
	tb_keep_aspect_ratio	.addListener(this);
	tb_force_resize			.addListener(this);
	tb_save_as_png			.addListener(this);
	tb_save_as_jpeg			.addListener(this);
	cancel					.addListener(this);
	ok						.addListener(this);

	tb_enable_auto_annotation.addListener(this);
	tb_model_type_darknet.addListener(this);
	tb_model_type_onnx.addListener(this);
	btn_select_darknet_weights.addListener(this);
	btn_select_darknet_cfg.addListener(this);
	btn_select_darknet_names.addListener(this);
	btn_select_onnx_model.addListener(this);
	btn_select_onnx_names.addListener(this);
	tb_import_with_detections.addListener(this);
	tb_import_without_detections.addListener(this);
	tb_import_all_frames.addListener(this);
	tb_enable_tiling.addListener(this);

	tb_extract_sequences	.setToggleState(true, NotificationType::sendNotification);
	tb_do_not_resize		.setToggleState(true, NotificationType::sendNotification);
	tb_keep_aspect_ratio	.setToggleState(true, NotificationType::sendNotification);
	tb_save_as_jpeg			.setToggleState(true, NotificationType::sendNotification);
	tb_model_type_darknet	.setToggleState(true, NotificationType::sendNotification);
	tb_import_all_frames	.setToggleState(true, NotificationType::sendNotification);

	sl_sequences			.setRange(1.0, 999.0, 1.0);
	sl_sequences			.setNumDecimalPlacesToDisplay(0);
	sl_sequences			.setValue(10);

	sl_consecutive_frames	.setRange(10.0, 9999.0, 1.0);
	sl_consecutive_frames	.setNumDecimalPlacesToDisplay(0);
	sl_consecutive_frames	.setValue(30);

	sl_maximum				.setRange(1.0, 9999.0, 1.0);
	sl_maximum				.setNumDecimalPlacesToDisplay(0);
	sl_maximum				.setValue(500.0);

	sl_percentage			.setRange(1.0, 99.0, 1.0);
	sl_percentage			.setNumDecimalPlacesToDisplay(0);
	sl_percentage			.setValue(25.0);

	sl_jpeg_quality			.setRange(30.0, 99.0, 1.0);
	sl_jpeg_quality			.setNumDecimalPlacesToDisplay(0);
	sl_jpeg_quality			.setValue(75.0);

	sl_confidence_threshold.setRange(0.0, 100.0, 1.0);
	sl_confidence_threshold.setValue(25.0);
	sl_nms_threshold.setRange(0.0, 100.0, 1.0);
	sl_nms_threshold.setValue(25.0);
	sl_hierarchy_threshold.setRange(0.0, 100.0, 1.0);
	sl_hierarchy_threshold.setValue(25.0);

	// Load saved file paths from configuration
	darknet_weights_path = cfg().getValue("video_import_darknet_weights", "").toStdString();
	darknet_cfg_path = cfg().getValue("video_import_darknet_cfg", "").toStdString();
	darknet_names_path = cfg().getValue("video_import_darknet_names", "").toStdString();
	onnx_model_path = cfg().getValue("video_import_onnx_model", "").toStdString();
	onnx_names_path = cfg().getValue("video_import_onnx_names", "").toStdString();

	// Update labels with loaded paths
	if (!darknet_weights_path.empty())
		lbl_darknet_weights.setText(File(darknet_weights_path).getFileName(), dontSendNotification);
	if (!darknet_cfg_path.empty())
		lbl_darknet_cfg.setText(File(darknet_cfg_path).getFileName(), dontSendNotification);
	if (!darknet_names_path.empty())
		lbl_darknet_names.setText(File(darknet_names_path).getFileName(), dontSendNotification);
	if (!onnx_model_path.empty())
		lbl_onnx_model.setText(File(onnx_model_path).getFileName(), dontSendNotification);
	if (!onnx_names_path.empty())
		lbl_onnx_names.setText(File(onnx_names_path).getFileName(), dontSendNotification);

	ef_width				.setInputRestrictions(5, "0123456789");
	ef_height				.setInputRestrictions(5, "0123456789");
	ef_width				.setText("640");
	ef_height				.setText("480");
	ef_width				.setJustification(Justification::centred);
	ef_height				.setJustification(Justification::centred);

	std::stringstream ss;
	try
	{
		const std::string filename = filenames.at(0);
		const std::string shortname = File(filename).getFileName().toStdString();

		cv::VideoCapture cap;
		const bool success = cap.open(filename);
		if (not success)
		{
			throw std::runtime_error("failed to open video file " + filename);
		}

		const auto number_of_frames		= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT			);
		const auto fps					= cap.get(cv::VideoCaptureProperties::CAP_PROP_FPS					);
		const auto frame_width			= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_WIDTH			);
		const auto frame_height			= cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT			);

		const auto fpm					= fps * 60.0;
		const auto len_minutes			= std::floor(number_of_frames / fpm);
		const auto len_seconds			= (number_of_frames - (len_minutes * fpm)) / fps;

		std::string opencv_ver_and_name	= "OpenCV v" + std::to_string(CV_VERSION_MAJOR) + "." + std::to_string(CV_VERSION_MINOR) + "." + std::to_string(CV_VERSION_REVISION);

		// Almost all videos will be "avc1" and "I420", but every once in a while we might see something else.
		const uint32_t fourcc			= cap.get(cv::VideoCaptureProperties::CAP_PROP_FOURCC				);

#if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 2)
		// Turns out the pixel format enum and backend name only exists in OpenCV v4.x and newer,
		// but Ubuntu 18.04 is still using the older version 3.2, so those have to be handled differently.
		const uint32_t format			= cap.get(cv::VideoCaptureProperties::CAP_PROP_CODEC_PIXEL_FORMAT	);
		opencv_ver_and_name				+= "/" + cap.getBackendName();
#else
		const uint32_t format			= 0;
#endif

		const std::string fourcc_str	= (fourcc == 0 ? std::to_string(fourcc) : std::string(reinterpret_cast<const char *>(&fourcc), 4));
		const std::string format_str	= (format == 0 ? std::to_string(format) : std::string(reinterpret_cast<const char *>(&format), 4));

		std::string description;
		magic_t magic_cookie = magic_open(MAGIC_NONE);
		if (magic_cookie != 0)
		{
			const auto rc = magic_load(magic_cookie, nullptr);
			if (rc == 0)
			{
				description = magic_file(magic_cookie, filename.c_str());
			}
			magic_close(magic_cookie);
		}

		if (v.size() > 1)
		{
			ss << "There are " << v.size() << " videos to import. The settings below will be applied individually to each video." << std::endl << std::endl;
			extra_lines_needed += 2;
		}

		ss	<< "Using " << opencv_ver_and_name << " to read the video file \"" << shortname << "\"." << std::endl
			<< std::endl
			<< "File type is FourCC: " << fourcc_str << ", pixel format: " << format_str
			<< (description.empty() ? "" : ", " + description) << "." << std::endl
			<< std::endl
			<< "The video contains " << number_of_frames << " individual frames at " << fps << " FPS "
			<< "for a total length of " << len_minutes << "m " << std::fixed << std::setprecision(1) << len_seconds << "s. " << std::endl
			<< std::endl
			<< "Each frame measures " << static_cast<int>(frame_width) << " x " << static_cast<int>(frame_height) << ".";
	}
	catch (...)
	{
		ss << "Error reading video file \"" << filenames.at(0) << "\".";
	}
	header_message.setText(ss.str(), NotificationType::sendNotification);

	setIcon(DarkMarkLogo());
	ComponentPeer *peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	if (cfg().containsKey("VideoImportWnd"))
	{
		restoreWindowStateFromString(cfg().getValue("VideoImportWnd"));
	}
	else
	{
		centreWithSize(380, 600);
	}

	setVisible(true);

	return;
}


dm::VideoImportWindow::~VideoImportWindow()
{
	signalThreadShouldExit();
	cfg().setValue("VideoImportWnd", getWindowStateAsString());

	return;
}


void dm::VideoImportWindow::closeButtonPressed()
{
	// close button

	setVisible(false);
	exitModalState(0);

	return;
}


void dm::VideoImportWindow::userTriedToCloseWindow()
{
	// ALT+F4

	closeButtonPressed();

	return;
}


void dm::VideoImportWindow::resized()
{
	// get the document window to resize the canvas, then we'll deal with the rest of the components
	DocumentWindow::resized();

	const auto height = 20.0f;
	const auto margin_size = 5;
	const FlexItem::Margin left_indent(0.0f, 0.0f, 0.0f, margin_size * 5.0f);
	const FlexItem::Margin new_row_indent(margin_size * 5.0f, 0.0f, 0.0f, 0.0f);

	FlexBox fb_rows;
	fb_rows.flexDirection	= FlexBox::Direction::column;
	fb_rows.alignItems		= FlexBox::AlignItems::stretch;
	fb_rows.justifyContent	= FlexBox::JustifyContent::flexStart;

	fb_rows.items.add(FlexItem(header_message			).withHeight(height * (8 + extra_lines_needed)).withFlex(1.0));
	fb_rows.items.add(FlexItem(tb_extract_all			).withHeight(height).withMargin(new_row_indent));
	fb_rows.items.add(FlexItem(tb_extract_sequences		).withHeight(height));

	FlexBox fb_sequences_1;
	fb_sequences_1.flexDirection	= FlexBox::Direction::row;
	fb_sequences_1.justifyContent	= FlexBox::JustifyContent::flexStart;
	fb_sequences_1.items.add(FlexItem(sl_sequences		).withHeight(height).withWidth(150.0f));
	fb_sequences_1.items.add(FlexItem(txt_sequences		).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_sequences_1			).withHeight(height).withMargin(left_indent));

	FlexBox fb_sequences_2;
	fb_sequences_2.flexDirection	= FlexBox::Direction::row;
	fb_sequences_2.justifyContent	= FlexBox::JustifyContent::flexStart;
	fb_sequences_2.items.add(FlexItem(sl_consecutive_frames	).withHeight(height).withWidth(150.0f));
	fb_sequences_2.items.add(FlexItem(txt_consecutive_frames).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_sequences_2			).withHeight(height).withMargin(left_indent));

	fb_rows.items.add(FlexItem(tb_extract_maximum		).withHeight(height));
	fb_rows.items.add(FlexItem(sl_maximum				).withHeight(height).withMaxWidth(150.0f).withMargin(left_indent));
	fb_rows.items.add(FlexItem(tb_extract_percentage	).withHeight(height));
	fb_rows.items.add(FlexItem(sl_percentage			).withHeight(height).withMaxWidth(150.0f).withMargin(left_indent));
	fb_rows.items.add(FlexItem(tb_do_not_resize			).withHeight(height).withMargin(new_row_indent));
	fb_rows.items.add(FlexItem(tb_resize				).withHeight(height));

	FlexBox fb_dimensions;
	fb_dimensions.flexDirection	= FlexBox::Direction::row;
	fb_dimensions.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_dimensions.items.add(FlexItem(ef_width			).withHeight(height).withWidth(50.0f));
	fb_dimensions.items.add(FlexItem(txt_x				).withHeight(height).withWidth(20.0f));
	fb_dimensions.items.add(FlexItem(ef_height			).withHeight(height).withWidth(50.0f));
	fb_rows.items.add(FlexItem(fb_dimensions			).withHeight(height).withMargin(left_indent));

	fb_rows.items.add(FlexItem(tb_keep_aspect_ratio		).withHeight(height).withMargin(left_indent));
	fb_rows.items.add(FlexItem(tb_force_resize			).withHeight(height).withMargin(left_indent));
	fb_rows.items.add(FlexItem(tb_save_as_png			).withHeight(height).withMargin(new_row_indent));
	fb_rows.items.add(FlexItem(tb_save_as_jpeg			).withHeight(height));

	FlexBox fb_quality;
	fb_quality.flexDirection = FlexBox::Direction::row;
	fb_quality.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_quality.items.add(FlexItem(txt_jpeg_quality		).withHeight(height).withWidth(100.0f));
	fb_quality.items.add(FlexItem(sl_jpeg_quality		).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_quality				).withHeight(height).withMargin(left_indent));

	fb_rows.items.add(FlexItem(tb_enable_auto_annotation).withHeight(height).withMargin(new_row_indent));

	FlexBox fb_model_type;
	fb_model_type.flexDirection = FlexBox::Direction::row;
	fb_model_type.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_model_type.items.add(FlexItem(tb_model_type_darknet).withHeight(height).withWidth(150.0f));
	fb_model_type.items.add(FlexItem(tb_model_type_onnx).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_model_type).withHeight(height).withMargin(left_indent));

	FlexBox fb_darknet_weights;
	fb_darknet_weights.flexDirection = FlexBox::Direction::row;
	fb_darknet_weights.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_darknet_weights.items.add(FlexItem(btn_select_darknet_weights).withHeight(height).withWidth(150.0f));
	fb_darknet_weights.items.add(FlexItem(lbl_darknet_weights).withHeight(height).withFlex(1.0f));
	fb_rows.items.add(FlexItem(fb_darknet_weights).withHeight(height).withMargin(left_indent));

	FlexBox fb_darknet_cfg;
	fb_darknet_cfg.flexDirection = FlexBox::Direction::row;
	fb_darknet_cfg.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_darknet_cfg.items.add(FlexItem(btn_select_darknet_cfg).withHeight(height).withWidth(150.0f));
	fb_darknet_cfg.items.add(FlexItem(lbl_darknet_cfg).withHeight(height).withFlex(1.0f));
	fb_rows.items.add(FlexItem(fb_darknet_cfg).withHeight(height).withMargin(left_indent));

	FlexBox fb_darknet_names;
	fb_darknet_names.flexDirection = FlexBox::Direction::row;
	fb_darknet_names.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_darknet_names.items.add(FlexItem(btn_select_darknet_names).withHeight(height).withWidth(150.0f));
	fb_darknet_names.items.add(FlexItem(lbl_darknet_names).withHeight(height).withFlex(1.0f));
	fb_rows.items.add(FlexItem(fb_darknet_names).withHeight(height).withMargin(left_indent));

	FlexBox fb_onnx_model;
	fb_onnx_model.flexDirection = FlexBox::Direction::row;
	fb_onnx_model.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_onnx_model.items.add(FlexItem(btn_select_onnx_model).withHeight(height).withWidth(150.0f));
	fb_onnx_model.items.add(FlexItem(lbl_onnx_model).withHeight(height).withFlex(1.0f));
	fb_rows.items.add(FlexItem(fb_onnx_model).withHeight(height).withMargin(left_indent));

	FlexBox fb_onnx_names;
	fb_onnx_names.flexDirection = FlexBox::Direction::row;
	fb_onnx_names.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_onnx_names.items.add(FlexItem(btn_select_onnx_names).withHeight(height).withWidth(150.0f));
	fb_onnx_names.items.add(FlexItem(lbl_onnx_names).withHeight(height).withFlex(1.0f));
	fb_rows.items.add(FlexItem(fb_onnx_names).withHeight(height).withMargin(left_indent));

	FlexBox fb_confidence;
	fb_confidence.flexDirection = FlexBox::Direction::row;
	fb_confidence.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_confidence.items.add(FlexItem(txt_confidence_threshold).withHeight(height).withWidth(150.0f));
	fb_confidence.items.add(FlexItem(sl_confidence_threshold).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_confidence).withHeight(height).withMargin(left_indent));

	FlexBox fb_nms;
	fb_nms.flexDirection = FlexBox::Direction::row;
	fb_nms.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_nms.items.add(FlexItem(txt_nms_threshold).withHeight(height).withWidth(150.0f));
	fb_nms.items.add(FlexItem(sl_nms_threshold).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_nms).withHeight(height).withMargin(left_indent));

	FlexBox fb_hierarchy;
	fb_hierarchy.flexDirection = FlexBox::Direction::row;
	fb_hierarchy.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_hierarchy.items.add(FlexItem(txt_hierarchy_threshold).withHeight(height).withWidth(150.0f));
	fb_hierarchy.items.add(FlexItem(sl_hierarchy_threshold).withHeight(height).withWidth(150.0f));
	fb_rows.items.add(FlexItem(fb_hierarchy).withHeight(height).withMargin(left_indent));

	fb_rows.items.add(FlexItem(tb_enable_tiling).withHeight(height).withMargin(left_indent));

	FlexBox fb_import_filter;
	fb_import_filter.flexDirection = FlexBox::Direction::row;
	fb_import_filter.justifyContent = FlexBox::JustifyContent::flexStart;
	fb_import_filter.items.add(FlexItem(tb_import_with_detections).withHeight(height).withWidth(250.0f));
	fb_import_filter.items.add(FlexItem(tb_import_without_detections).withHeight(height).withWidth(250.0f));
	fb_import_filter.items.add(FlexItem(tb_import_all_frames).withHeight(height).withWidth(250.0f));
	fb_rows.items.add(FlexItem(fb_import_filter).withHeight(height).withMargin(left_indent));

	FlexBox button_row;
	button_row.flexDirection = FlexBox::Direction::row;
	button_row.justifyContent = FlexBox::JustifyContent::flexEnd;
	button_row.items.add(FlexItem()				.withFlex(1.0));
	button_row.items.add(FlexItem(cancel)		.withWidth(100.0).withMargin(FlexItem::Margin(0, margin_size, 0, 0)));
	button_row.items.add(FlexItem(ok)			.withWidth(100.0));

	fb_rows.items.add(FlexItem().withFlex(1.0));
	fb_rows.items.add(FlexItem(button_row).withHeight(30.0));

	auto r = getLocalBounds();
	r.reduce(margin_size, margin_size);
	fb_rows.performLayout(r);

	return;
}


void dm::VideoImportWindow::buttonClicked(Button * button)
{
	if (button == &cancel)
	{
		closeButtonPressed();
		return;
	}

	bool b = tb_extract_maximum.getToggleState();
	sl_maximum.setEnabled(b);

	b = tb_extract_sequences.getToggleState();
	sl_sequences.setEnabled(b);
	txt_sequences.setEnabled(b);
	sl_consecutive_frames.setEnabled(b);
	txt_consecutive_frames.setEnabled(b);

	b = tb_extract_percentage.getToggleState();
	sl_percentage.setEnabled(b);

	b = tb_resize.getToggleState();
	ef_width.setEnabled(b);
	txt_x.setEnabled(b);
	ef_height.setEnabled(b);
	tb_keep_aspect_ratio.setEnabled(b);
	tb_force_resize.setEnabled(b);

	b = tb_save_as_jpeg.getToggleState();
	txt_jpeg_quality.setEnabled(b);
	sl_jpeg_quality.setEnabled(b);

	if (button == &btn_select_darknet_weights)
	{
		FileChooser fc("Select Darknet weights file", File(base_directory), "*.weights");
		if (fc.browseForFileToOpen())
		{
			darknet_weights_path = fc.getResult().getFullPathName().toStdString();
			lbl_darknet_weights.setText(fc.getResult().getFileName(), dontSendNotification);
			cfg().setValue("video_import_darknet_weights", String(darknet_weights_path));
		}
	}
	else if (button == &btn_select_darknet_cfg)
	{
		FileChooser fc("Select Darknet cfg file", File(base_directory), "*.cfg");
		if (fc.browseForFileToOpen())
		{
			darknet_cfg_path = fc.getResult().getFullPathName().toStdString();
			lbl_darknet_cfg.setText(fc.getResult().getFileName(), dontSendNotification);
			cfg().setValue("video_import_darknet_cfg", String(darknet_cfg_path));
		}
	}
	else if (button == &btn_select_darknet_names)
	{
		FileChooser fc("Select Darknet names file", File(base_directory), "*.names");
		if (fc.browseForFileToOpen())
		{
			darknet_names_path = fc.getResult().getFullPathName().toStdString();
			lbl_darknet_names.setText(fc.getResult().getFileName(), dontSendNotification);
			cfg().setValue("video_import_darknet_names", String(darknet_names_path));
		}
	}
	else if (button == &btn_select_onnx_model)
	{
		FileChooser fc("Select ONNX model file", File(base_directory), "*.onnx");
		if (fc.browseForFileToOpen())
		{
			onnx_model_path = fc.getResult().getFullPathName().toStdString();
			lbl_onnx_model.setText(fc.getResult().getFileName(), dontSendNotification);
			cfg().setValue("video_import_onnx_model", String(onnx_model_path));
		}
	}
	else if (button == &btn_select_onnx_names)
	{
		FileChooser fc("Select ONNX names file", File(base_directory), "*.names");
		if (fc.browseForFileToOpen())
		{
			onnx_names_path = fc.getResult().getFullPathName().toStdString();
			lbl_onnx_names.setText(fc.getResult().getFileName(), dontSendNotification);
			cfg().setValue("video_import_onnx_names", String(onnx_names_path));
		}
	}

	update_model_type_ui();

	if (button == &ok)
	{
		// disable all of the controls and start the frame extraction
		canvas.setEnabled(false);
		runThread(); // this waits for the thread to be done
		closeButtonPressed();
	}

	return;
}


void dm::VideoImportWindow::update_model_type_ui()
{
	const bool auto_annotation_enabled = tb_enable_auto_annotation.getToggleState();
	const bool darknet_selected = tb_model_type_darknet.getToggleState();
	const bool onnx_selected = tb_model_type_onnx.getToggleState();

	tb_model_type_darknet.setVisible(auto_annotation_enabled);
	tb_model_type_onnx.setVisible(auto_annotation_enabled);

	btn_select_darknet_weights.setVisible(auto_annotation_enabled && darknet_selected);
	btn_select_darknet_cfg.setVisible(auto_annotation_enabled && darknet_selected);
	btn_select_darknet_names.setVisible(auto_annotation_enabled && darknet_selected);
	lbl_darknet_weights.setVisible(auto_annotation_enabled && darknet_selected);
	lbl_darknet_cfg.setVisible(auto_annotation_enabled && darknet_selected);
	lbl_darknet_names.setVisible(auto_annotation_enabled && darknet_selected);

	btn_select_onnx_model.setVisible(auto_annotation_enabled && onnx_selected);
	btn_select_onnx_names.setVisible(auto_annotation_enabled && onnx_selected);
	lbl_onnx_model.setVisible(auto_annotation_enabled && onnx_selected);
	lbl_onnx_names.setVisible(auto_annotation_enabled && onnx_selected);

	tb_import_with_detections.setVisible(auto_annotation_enabled);
	tb_import_without_detections.setVisible(auto_annotation_enabled);
	tb_import_all_frames.setVisible(auto_annotation_enabled);

	txt_confidence_threshold.setVisible(auto_annotation_enabled);
	sl_confidence_threshold.setVisible(auto_annotation_enabled);
	txt_nms_threshold.setVisible(auto_annotation_enabled);
	sl_nms_threshold.setVisible(auto_annotation_enabled);

	tb_enable_tiling.setVisible(auto_annotation_enabled && darknet_selected);
	txt_hierarchy_threshold.setVisible(auto_annotation_enabled && darknet_selected);
	sl_hierarchy_threshold.setVisible(auto_annotation_enabled && darknet_selected);

	ok.setEnabled(validate_model_files());
}


bool dm::VideoImportWindow::validate_model_files()
{
	if (tb_enable_auto_annotation.getToggleState())
	{
		if (tb_model_type_darknet.getToggleState())
		{
			return !darknet_weights_path.empty() && !darknet_cfg_path.empty() && !darknet_names_path.empty();
		}
		else if (tb_model_type_onnx.getToggleState())
		{
			return !onnx_model_path.empty() && !onnx_names_path.empty();
		}
	}

	return true;
}


void dm::VideoImportWindow::run()
{
	std::string current_filename		= "?";
	double work_completed				= 0.0;
	double work_to_be_done				= 1.0;
	bool error_shown					= false;
	number_of_processed_frames			= 0;

	if (tb_enable_auto_annotation.getToggleState()) {
		try {
			load_selected_model();
		}
		catch (const std::exception& e) {
			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon,
				"Model Loading Error", e.what());
			return;
		}
	}

	try
	{
		const bool extract_all_frames		= tb_extract_all		.getToggleState();
		const bool extract_sequences		= tb_extract_sequences	.getToggleState();
		const bool extract_maximum_frames	= tb_extract_maximum	.getToggleState();
		const bool extract_percentage		= tb_extract_percentage	.getToggleState();
		const double number_of_sequences	= sl_sequences			.getValue();
		const double consecutive_frames		= sl_consecutive_frames	.getValue();
		const double maximum_to_extract		= sl_maximum			.getValue();
		const double percent_to_extract		= sl_percentage			.getValue() / 100.0;
		const bool resize_frame				= tb_resize				.getToggleState();
		const bool maintain_aspect_ratio	= tb_keep_aspect_ratio	.getToggleState();
		const int new_width					= std::atoi(ef_width	.getText().toStdString().c_str());
		const int new_height				= std::atoi(ef_height	.getText().toStdString().c_str());
		const bool save_as_png				= tb_save_as_png		.getToggleState();
		const bool save_as_jpg				= tb_save_as_jpeg		.getToggleState();
		const int jpg_quality				= sl_jpeg_quality		.getValue();

		setStatusMessage("Determining the amount of frames to extract...");

		for (auto && filename : filenames)
		{
			if (threadShouldExit())
			{
				break;
			}

			current_filename = filename;

			cv::VideoCapture cap;
			cap.open(filename);
			const auto number_of_frames = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);

			if (extract_all_frames)
			{
				work_to_be_done += number_of_frames;
			}
			if (extract_sequences)
			{
				const double work = number_of_sequences * consecutive_frames;
				work_to_be_done += std::min(work, number_of_frames);
			}
			else if (extract_maximum_frames)
			{
				work_to_be_done += std::min(number_of_frames, maximum_to_extract);
			}
			else if (extract_percentage)
			{
				work_to_be_done += number_of_frames * percent_to_extract;
			}
		}

		// now start actually extracting frames
		for (auto && filename : filenames)
		{
			if (threadShouldExit())
			{
				break;
			}

			current_filename = filename;

			const std::string shortname = File(filename).getFileName().toStdString(); // filename+extension, but no path

			std::string sanitized_name = shortname;
			while (true)
			{
				auto p = sanitized_name.find_first_not_of(
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
					"abcdefghijklmnopqrstuvwxyz"
					"0123456789"
					"-_");
				if (p == std::string::npos)
				{
					break;
				}
				sanitized_name[p] = '_';
			}

			File dir(base_directory);
			File child = dir.getChildFile(Time::getCurrentTime().formatted("video_import_%Y-%m-%d_%H-%M-%S_" + sanitized_name));
			child.createDirectory();
			std::string partial_output_filename = child.getChildFile(shortname).getFullPathName().toStdString();
			size_t pos = partial_output_filename.rfind("."); // erase the extension if we find one
			if (pos != std::string::npos)
			{
				partial_output_filename.erase(pos);
			}

			setStatusMessage("Processing video file " + shortname + "...");

			cv::VideoCapture cap;
			cap.open(filename);
			const auto number_of_frames = cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT);

			auto & rng = get_random_engine();
			std::uniform_int_distribution<size_t> uni(0, number_of_frames - 1);

			SId frames_needed;
			if (extract_sequences)
			{
				/* Say the video is this long:
				 *
				 *		[...............................]
				 *
				 * If we need to extract 3 sequences, then we divide it into 3+1 sections:
				 *
				 *		[...............................]
				 *		|		|		|		|		|
				 *		0		1		2		3		4
				 *
				 * Each of those numbered sections becomes the mid-point of a sequence,
				 * so we take some frames before and some frames after each mid-point.
				 */
				for (size_t sequence_counter = 1; sequence_counter <= number_of_sequences; sequence_counter ++)
				{
					const size_t mid_point = number_of_frames * sequence_counter / (number_of_sequences + 1);
					const size_t half = consecutive_frames / 2;
					size_t min_point = 0;
					size_t max_point = mid_point + half;
					if (mid_point > half)
					{
						min_point = mid_point - half;
					}
					if (max_point > number_of_frames - 1)
					{
						max_point = number_of_frames - 1;
					}

					// if the number of frames is even, then the mid point + 2 * half will give us 1 too many frames
					if (max_point - min_point + 1 > consecutive_frames)
					{
						max_point -= 1;
					}

					Log("sequence counter .. " + std::to_string(sequence_counter));
					Log("-> min point ...... " + std::to_string(min_point));
					Log("-> mid point ...... " + std::to_string(mid_point));
					Log("-> max point ...... " + std::to_string(max_point));
					Log("-> total frames ... " + std::to_string(max_point - min_point + 1));

					for (size_t frame = min_point; frame <= max_point; frame ++)
					{
						frames_needed.insert(frame);
					}
				}
			}
			else if (extract_maximum_frames)
			{
				while (threadShouldExit() == false and frames_needed.size() < std::min(number_of_frames, maximum_to_extract))
				{
					const auto random_frame = uni(rng);
					frames_needed.insert(random_frame);
				}
			}
			else if (extract_percentage)
			{
				while (threadShouldExit() == false and frames_needed.size() < percent_to_extract * number_of_frames)
				{
					const auto random_frame = uni(rng);
					frames_needed.insert(random_frame);
				}
			}

			Log("about to start extracting " + std::to_string(frames_needed.size()) + " frames from " + filename);

			size_t frame_number = 0;
			size_t previous_frame_number = 0;
			size_t stuck_frame_count = 0;
			while (threadShouldExit() == false)
			{
				// Safety check: prevent infinite loops in "extract all frames" mode
				if (extract_all_frames && frame_number >= number_of_frames)
				{
					Log("reached end of video (frame " + std::to_string(frame_number) + " >= " + std::to_string(number_of_frames) + ") - breaking out of loop");
					break;
				}

				if (extract_sequences or extract_maximum_frames or extract_percentage)
				{
					if (frames_needed.empty())
					{
						// we've extracted all the frames we need
						break;
					}

					const auto next_frame_needed = *frames_needed.begin();
					frames_needed.erase(next_frame_needed);
					if (frame_number != next_frame_needed)
					{
						// only explicitely set the absolute frame position if the frames are not consecutive
						// but first check if the frame position is valid
						if (next_frame_needed < number_of_frames)
						{
							cap.set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, static_cast<double>(next_frame_needed));
							frame_number = next_frame_needed;
						}
						else
						{
							// frame position is beyond video length, skip this frame
							Log("skipping frame " + std::to_string(next_frame_needed) + " as it's beyond video length (" + std::to_string(number_of_frames) + ")");
							continue;
						}
					}
				}

				cv::Mat mat;
				cap >> mat;
				if (mat.empty())
				{
					// must have reached the EOF - break out of the loop to prevent infinite hanging
					Log("received an empty mat while reading frame #" + std::to_string(frame_number) + " - reached end of video");
					break;
				}

				// Check if we're stuck at the same frame (video capture not advancing)
				if (frame_number == previous_frame_number)
				{
					stuck_frame_count++;
					if (stuck_frame_count > 10) // Allow a few retries before giving up
					{
						Log("video capture appears to be stuck at frame " + std::to_string(frame_number) + " - breaking out of loop");
						break;
					}
				}
				else
				{
					stuck_frame_count = 0; // Reset counter when frame advances
				}
				previous_frame_number = frame_number;

				if (resize_frame and (mat.cols != new_width or mat.rows != new_height))
				{
					if (maintain_aspect_ratio)
					{
						mat = DarkHelp::resize_keeping_aspect_ratio(mat, {new_width, new_height});
					}
					else
					{
						mat = DarkHelp::slow_resize_ignore_aspect_ratio(mat, {new_width, new_height});
					}
				}

				std::stringstream ss;
				ss << partial_output_filename << "_frame_" << std::setfill('0') << std::setw(6) << frame_number;

				if (tb_enable_auto_annotation.getToggleState() && (temp_darknet_nn || temp_onnx_nn))
				{
					auto predictions = run_inference(mat);

					bool has_detections = !predictions.empty();
					bool should_import = true;

					if (tb_import_with_detections.getToggleState() && !has_detections)
					{
						should_import = false;
					}
					else if (tb_import_without_detections.getToggleState() && has_detections)
					{
						should_import = false;
					}

					if (should_import)
					{
						if (save_as_png)
						{
							cv::imwrite(ss.str() + ".png", mat, { CV_IMWRITE_PNG_COMPRESSION, 1 });
						}
						else if (save_as_jpg)
						{
							cv::imwrite(ss.str() + ".jpg", mat, { CV_IMWRITE_JPEG_QUALITY, jpg_quality });
						}

						if (has_detections)
						{
							generate_annotation_file(ss.str(), predictions, mat.size());
						}
					}
				}
				else
				{
					if (save_as_png)
					{
						cv::imwrite(ss.str() + ".png", mat, { CV_IMWRITE_PNG_COMPRESSION, 1 });
					}
					else if (save_as_jpg)
					{
						cv::imwrite(ss.str() + ".jpg", mat, { CV_IMWRITE_JPEG_QUALITY, jpg_quality });
					}
				}

				frame_number ++;
				number_of_processed_frames ++;
				work_completed += 1.0;
				setProgress(work_completed / work_to_be_done);
			}
		}
	}
	catch (const std::exception & e)
	{
		std::stringstream ss;
		ss	<< "An error was detected while processing the video file \"" + current_filename + "\":" << std::endl
			<< std::endl
			<< e.what();
		dm::Log(ss.str());
		error_shown = true;
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "DarkMark - Error!", ss.str());
	}
	catch (...)
	{
		const std::string msg = "An unknown error was encountered while processing the video file \"" + current_filename + "\".";
		dm::Log(msg);
		error_shown = true;
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "DarkMark - Error!", msg);
	}

	File dir(base_directory);
	dir.revealToUser();

	if (error_shown == false and threadShouldExit() == false)
	{
		Log("finished extracting " + std::to_string(number_of_processed_frames) + " video frames");
		AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon, "DarkMark", "Extracted " + std::to_string(number_of_processed_frames) + " video frames.");
	}

	clear_model();

	return;
}


void dm::VideoImportWindow::load_darknet_model()
{
	try {
		temp_darknet_nn.reset(new DarkHelp::NN(darknet_cfg_path, darknet_weights_path, darknet_names_path));

		temp_darknet_nn->config.threshold = sl_confidence_threshold.getValue() / 100.0;
		temp_darknet_nn->config.non_maximal_suppression_threshold = sl_nms_threshold.getValue() / 100.0;
		temp_darknet_nn->config.hierarchy_threshold = sl_hierarchy_threshold.getValue() / 100.0;
		temp_darknet_nn->config.enable_tiles = enable_tiling;

		class_names = temp_darknet_nn->names;

		Log("Darknet model loaded successfully");
	}
	catch (const std::exception& e) {
		temp_darknet_nn.reset(nullptr);
		Log("Failed to load Darknet model: " + std::string(e.what()));
		throw;
	}
}


void dm::VideoImportWindow::load_onnx_model()
{
	try {
		std::vector<std::string> names;
		std::ifstream ifs(onnx_names_path);
		std::string line;
		while (std::getline(ifs, line)) {
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
			if (!line.empty()) names.push_back(line);
		}

		temp_onnx_nn.reset(new OnnxHelp::NN(onnx_model_path, names));
		class_names = names;
		
		// Auto-detect preprocessing mode based on filename
		std::string lower_filename = onnx_model_path;
		std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
		
		if (lower_filename.find("dfine") != std::string::npos ||
			lower_filename.find("rtdetr") != std::string::npos ||
			lower_filename.find("rt-detr") != std::string::npos ||
			lower_filename.find("detr") != std::string::npos)
		{
			temp_onnx_nn->set_preprocess_config(OnnxHelp::PreprocessConfig::dfine());
			Log("Auto-detected D-FINE/DETR-style model, using direct resize preprocessing");
		}
		else
		{
			temp_onnx_nn->set_preprocess_config(OnnxHelp::PreprocessConfig::yolox());
			Log("Using YOLOX-style preprocessing (letterbox)");
		}

		Log("ONNX model loaded successfully");
	}
	catch (const std::exception& e) {
		temp_onnx_nn.reset(nullptr);
		Log("Failed to load ONNX model: " + std::string(e.what()));
		throw;
	}
}


void dm::VideoImportWindow::load_selected_model()
{
	if (tb_model_type_darknet.getToggleState())
	{
		selected_model_type = ModelType::Darknet;
		load_darknet_model();
	}
	else if (tb_model_type_onnx.getToggleState())
	{
		selected_model_type = ModelType::ONNX;
		load_onnx_model();
	}
}


void dm::VideoImportWindow::clear_model()
{
	temp_darknet_nn.reset(nullptr);
	temp_onnx_nn.reset(nullptr);
}


std::vector<dm::UnifiedPredictionResult> dm::VideoImportWindow::run_inference(const cv::Mat& frame)
{
	std::vector<UnifiedPredictionResult> results;

	if (selected_model_type == ModelType::Darknet && temp_darknet_nn)
	{
		temp_darknet_nn->predict(frame);
		for (const auto& pred : temp_darknet_nn->prediction_results)
		{
			results.emplace_back(pred, class_names);
		}
	}
	else if (selected_model_type == ModelType::ONNX && temp_onnx_nn)
	{
		auto onnx_results = temp_onnx_nn->predict(frame, sl_confidence_threshold.getValue() / 100.0, sl_nms_threshold.getValue() / 100.0);
		for (const auto& pred : onnx_results)
		{
			results.emplace_back(pred);
		}
	}

	return results;
}


void dm::VideoImportWindow::generate_annotation_file(const std::string& base_path, const std::vector<UnifiedPredictionResult>& predictions, const cv::Size& image_size)
{
	std::ofstream annotation_file(base_path + ".txt");
	annotation_file << std::fixed << std::setprecision(10);

	for (const auto& pred : predictions) {
		double center_x = (pred.rect.x + pred.rect.width / 2.0) / image_size.width;
		double center_y = (pred.rect.y + pred.rect.height / 2.0) / image_size.height;
		double norm_width = pred.rect.width / (double)image_size.width;
		double norm_height = pred.rect.height / (double)image_size.height;

		annotation_file << pred.class_idx << " "
			<< center_x << " " << center_y << " "
			<< norm_width << " " << norm_height << std::endl;
	}
}
