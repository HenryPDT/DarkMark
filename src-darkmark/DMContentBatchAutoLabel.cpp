// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include <darknet.hpp>
#include "DarkMarkApp.hpp"
#include "DMContent.hpp"
#include "DMContentBatchAutoLabel.hpp"


dm::DMContentBatchAutoLabel::DMContentBatchAutoLabel(dm::DMContent & c) :
	DocumentWindow("Batch Auto-Label Images - " + c.project_info.project_name, Colours::darkgrey, TitleBarButtons::closeButton),
	ThreadWithProgressWindow("Batch Auto-Labeling...", true, true),
	content(c),
	safe_parent(&c),
	btn_select_all_classes("Select All"),
	btn_select_no_classes("Select None"),
	btn_start("Start Auto-Labeling"),
	btn_cancel("Cancel")
{
	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(false);
	}

	setContentNonOwned(&canvas, false);
	setUsingNativeTitleBar(true);
	setResizable(true, true);
	setDropShadowEnabled(true);

	setIcon(DarkMarkLogo());
	ComponentPeer * peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
	}

	const bool using_onnx = (dmapp().onnx_nn != nullptr);

	lbl_title.setText("Automatically run detection and generate annotations across multiple images.", dontSendNotification);
	lbl_title.setJustificationType(Justification::centred);
	lbl_title.setColour(Label::textColourId, Colours::white);
	canvas.addAndMakeVisible(lbl_title);

	lbl_scope.setText("Scope:", dontSendNotification);
	canvas.addAndMakeVisible(lbl_scope);

	cb_scope.addItem("All images in project (" + String(content.image_filenames.size()) + ")", 1);
	cb_scope.addItem("Only unannotated images", 2);
	cb_scope.setSelectedId(1);
	canvas.addAndMakeVisible(cb_scope);

	const int conf_default = cfg().get_int("onnx_threshold", 30);
	lbl_conf.setText("Confidence Threshold: " + String(conf_default) + "%", dontSendNotification);
	canvas.addAndMakeVisible(lbl_conf);
	sl_conf.setRange(5, 95, 1);
	sl_conf.setValue(conf_default);
	sl_conf.setEnabled(using_onnx || dmapp().darkhelp_nn != nullptr);
	sl_conf.onValueChange = [this]()
	{
		lbl_conf.setText("Confidence Threshold: " + String(static_cast<int>(sl_conf.getValue())) + "%", dontSendNotification);
	};
	canvas.addAndMakeVisible(sl_conf);

	const int nms_default = cfg().get_int("onnx_nms_threshold", 45);
	lbl_nms.setText("NMS Threshold: " + String(nms_default) + "%", dontSendNotification);
	canvas.addAndMakeVisible(lbl_nms);
	sl_nms.setRange(5, 95, 1);
	sl_nms.setValue(nms_default);
	sl_nms.setEnabled(using_onnx || dmapp().darkhelp_nn != nullptr);
	sl_nms.onValueChange = [this]()
	{
		lbl_nms.setText("NMS Threshold: " + String(static_cast<int>(sl_nms.getValue())) + "%", dontSendNotification);
	};
	canvas.addAndMakeVisible(sl_nms);

	tb_overlap.setButtonText("Skip detection if it overlaps existing mark of same class (IoU >= threshold)");
	tb_overlap.setToggleState(true, dontSendNotification);
	canvas.addAndMakeVisible(tb_overlap);

	const int iou_default = static_cast<int>(std::round(content.assisted_labeling_iou_threshold * 100.0));
	lbl_iou.setText("IoU Overlap Threshold: " + String(iou_default) + "%", dontSendNotification);
	canvas.addAndMakeVisible(lbl_iou);
	sl_iou.setRange(10, 90, 1);
	sl_iou.setValue(iou_default);
	sl_iou.onValueChange = [this]()
	{
		lbl_iou.setText("IoU Overlap Threshold: " + String(static_cast<int>(sl_iou.getValue())) + "%", dontSendNotification);
	};
	canvas.addAndMakeVisible(sl_iou);

	lbl_classes.setText("Target Classes to Auto-Label:", dontSendNotification);
	lbl_classes.setFont(Font(FontOptions(14.0f, Font::bold)));
	canvas.addAndMakeVisible(lbl_classes);

	canvas.addAndMakeVisible(pp_classes);

	const size_t num_project_classes = content.names.size() > 0 ? (content.names.size() - 1) : 0;
	class_checkbox_values.reserve(num_project_classes);
	for (size_t j = 0; j < num_project_classes; j++)
	{
		bool mapped_from_model = false;
		for (const auto & target_idx : content.model_to_project_class_map)
		{
			if (target_idx == static_cast<int>(j))
			{
				mapped_from_model = true;
				break;
			}
		}

		if (content.model_to_project_class_map.empty())
		{
			mapped_from_model = true;
		}

		class_checkbox_values.emplace_back(mapped_from_model);
		auto prop = new BooleanPropertyComponent(class_checkbox_values.back(), String(j) + ": " + content.names.at(j), "Enable");
		class_props.add(prop);
	}
	pp_classes.addProperties(class_props);

	canvas.addAndMakeVisible(btn_select_all_classes);
	canvas.addAndMakeVisible(btn_select_no_classes);
	btn_select_all_classes.addListener(this);
	btn_select_no_classes.addListener(this);

	canvas.addAndMakeVisible(btn_start);
	canvas.addAndMakeVisible(btn_cancel);
	btn_start.addListener(this);
	btn_cancel.addListener(this);

	const int win_w = 620;
	const int win_h = 580;
	setResizeLimits(520, 450, 1600, 1200);
	centreAroundComponent(safe_parent, win_w, win_h);
	setVisible(true);
	toFront(true);

	// Re-assert proper positioning and sizing after window manager maps the window
	MessageManager::callAsync([safe_this = Component::SafePointer<DMContentBatchAutoLabel>(this), parent = safe_parent, win_w, win_h]()
	{
		if (safe_this != nullptr)
		{
			safe_this->centreAroundComponent(parent, win_w, win_h);
			safe_this->toFront(true);
		}
	});
}


dm::DMContentBatchAutoLabel::~DMContentBatchAutoLabel()
{
	signalThreadShouldExit();
	stopThread(2000);

	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(true);
		safe_parent->toFront(true);
	}
}


void dm::DMContentBatchAutoLabel::closeButtonPressed()
{
	setVisible(false);

	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(true);
		safe_parent->toFront(true);
	}

	MessageManager::callAsync([]()
	{
		dmapp().batch_autolabel_wnd.reset(nullptr);
	});
}


void dm::DMContentBatchAutoLabel::userTriedToCloseWindow()
{
	closeButtonPressed();
}


void dm::DMContentBatchAutoLabel::resized()
{
	DocumentWindow::resized();

	const int margin = 15;
	const int button_h = 28;
	const auto bounds = canvas.getLocalBounds();
	const int bottom_y = bounds.getHeight() - margin - button_h;

	lbl_title.setBounds(margin, margin, bounds.getWidth() - 2 * margin, 24);

	int y = margin + 32;
	lbl_scope.setBounds(margin, y, 60, 24);
	cb_scope.setBounds(margin + 70, y, bounds.getWidth() - 2 * margin - 70, 24);

	y += 32;
	lbl_conf.setBounds(margin, y, 220, 24);
	sl_conf.setBounds(margin + 230, y, bounds.getWidth() - 2 * margin - 230, 24);

	y += 30;
	lbl_nms.setBounds(margin, y, 220, 24);
	sl_nms.setBounds(margin + 230, y, bounds.getWidth() - 2 * margin - 230, 24);

	y += 32;
	tb_overlap.setBounds(margin, y, bounds.getWidth() - 2 * margin, 24);

	y += 28;
	lbl_iou.setBounds(margin + 20, y, 200, 24);
	sl_iou.setBounds(margin + 230, y, bounds.getWidth() - 2 * margin - 230, 24);

	y += 34;
	lbl_classes.setBounds(margin, y, 220, 24);
	btn_select_all_classes.setBounds(bounds.getWidth() - margin - 175, y, 80, 22);
	btn_select_no_classes.setBounds(bounds.getWidth() - margin - 90, y, 85, 22);

	y += 28;
	const int pp_height = bottom_y - y - 10;
	pp_classes.setBounds(margin, y, bounds.getWidth() - 2 * margin, std::max(pp_height, 60));

	btn_cancel.setBounds(bounds.getWidth() - margin - 100, bottom_y, 100, button_h);
	btn_start.setBounds(bounds.getWidth() - margin - 270, bottom_y, 160, button_h);
}


void dm::DMContentBatchAutoLabel::buttonClicked(Button * button)
{
	if (button == &btn_select_all_classes)
	{
		for (auto & v : class_checkbox_values)
		{
			v.setValue(true);
		}
		pp_classes.refreshAll();
	}
	else if (button == &btn_select_no_classes)
	{
		for (auto & v : class_checkbox_values)
		{
			v.setValue(false);
		}
		pp_classes.refreshAll();
	}
	else if (button == &btn_cancel)
	{
		closeButtonPressed();
	}
	else if (button == &btn_start)
	{
		const int scope_id = cb_scope.getSelectedId();
		scope = (scope_id == 2) ? Scope::UnannotatedOnly : Scope::AllImages;

		conf_threshold = static_cast<float>(sl_conf.getValue()) / 100.0f;
		nms_threshold = static_cast<float>(sl_nms.getValue()) / 100.0f;
		overlap_suppression = tb_overlap.getToggleState();
		overlap_iou_threshold = sl_iou.getValue() / 100.0;

		enabled_classes.clear();
		bool any_enabled = false;
		for (auto & v : class_checkbox_values)
		{
			const bool is_on = static_cast<bool>(v.getValue());
			enabled_classes.push_back(is_on);
			if (is_on)
			{
				any_enabled = true;
			}
		}

		if (!any_enabled)
		{
			AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::WarningIcon, "DarkMark", "No target classes are selected for auto-labeling.");
			return;
		}

		// Save current marks on the Message Thread before worker thread runs
		if (content.need_to_save)
		{
			content.save_json();
			content.save_text();
		}

		canvas.setEnabled(false);
		runThread();
		closeButtonPressed();
	}
}


void dm::DMContentBatchAutoLabel::run()
{
	total_processed = 0;
	images_modified = 0;
	marks_added = 0;
	marks_skipped_overlap = 0;

	std::vector<std::string> targets = content.image_filenames;

	const double max_work = static_cast<double>(targets.size());
	if (max_work == 0.0)
	{
		return;
	}

	double work_completed = 0.0;
	bool cancelled = false;

	for (size_t idx = 0; idx < targets.size(); idx++)
	{
		if (threadShouldExit())
		{
			cancelled = true;
			break;
		}

		setProgress(work_completed / max_work);
		work_completed += 1.0;

		const std::string fn = targets.at(idx);
		setStatusMessage("Processing " + std::to_string(idx + 1) + "/" + std::to_string(targets.size()) + ": " + File(fn).getFileName().toStdString() +
			"\nAdded " + std::to_string(marks_added) + " marks across " + std::to_string(images_modified) + " images");

		VMarks existing_marks;
		bool image_is_completely_empty = false;
		content.load_marks_from_disk_files(fn, cv::Size(), existing_marks, image_is_completely_empty);

		if (scope == Scope::UnannotatedOnly && (!existing_marks.empty() || image_is_completely_empty))
		{
			continue;
		}

		cv::Mat mat = cv::imread(fn);
		if (mat.empty())
		{
			continue;
		}

		total_processed++;

		for (auto & m : existing_marks)
		{
			m.image_dimensions = mat.size();
		}

		std::vector<OnnxHelp::PredictionResult> onnx_results;
		std::vector<DarkHelp::PredictionResult> darkhelp_results;

		if (dmapp().onnx_nn)
		{
			onnx_results = dmapp().onnx_nn->predict(mat, conf_threshold, nms_threshold);
		}
		else if (dmapp().darkhelp_nn)
		{
			const float saved_threshold = dmapp().darkhelp_nn->config.threshold;
			const float saved_nms = dmapp().darkhelp_nn->config.non_maximal_suppression_threshold;
			dmapp().darkhelp_nn->config.threshold = conf_threshold;
			dmapp().darkhelp_nn->config.non_maximal_suppression_threshold = nms_threshold;
			darkhelp_results = dmapp().darkhelp_nn->predict(mat);
			dmapp().darkhelp_nn->config.threshold = saved_threshold;
			dmapp().darkhelp_nn->config.non_maximal_suppression_threshold = saved_nms;
		}
		else
		{
			break;
		}

		bool modified = false;

		for (const auto & pred : onnx_results)
		{
			int mapped_idx = content.get_mapped_class_idx(pred.class_idx);
			if (mapped_idx < 0 || static_cast<size_t>(mapped_idx) >= content.names.size() - 1)
			{
				continue;
			}

			if (static_cast<size_t>(mapped_idx) < enabled_classes.size() && !enabled_classes[static_cast<size_t>(mapped_idx)])
			{
				continue;
			}

			Mark candidate = content.make_prediction_mark_from_onnx(pred, mapped_idx, mat.size());

			if (overlap_suppression && content.is_duplicate_of_existing_marks(candidate, existing_marks, mat.size(), overlap_iou_threshold))
			{
				marks_skipped_overlap++;
				continue;
			}

			candidate.is_prediction = false;
			candidate.name = content.names.at(mapped_idx);
			candidate.description = content.names.at(mapped_idx);
			existing_marks.push_back(candidate);
			marks_added++;
			modified = true;
		}

		for (const auto & pred : darkhelp_results)
		{
			int mapped_idx = content.get_mapped_class_idx(pred.best_class);
			if (mapped_idx < 0 || static_cast<size_t>(mapped_idx) >= content.names.size() - 1)
			{
				continue;
			}

			if (static_cast<size_t>(mapped_idx) < enabled_classes.size() && !enabled_classes[static_cast<size_t>(mapped_idx)])
			{
				continue;
			}

			Mark candidate = content.make_prediction_mark_from_darkhelp(pred, mapped_idx, mat.size());

			if (overlap_suppression && content.is_duplicate_of_existing_marks(candidate, existing_marks, mat.size(), overlap_iou_threshold))
			{
				marks_skipped_overlap++;
				continue;
			}

			candidate.is_prediction = false;
			candidate.name = content.names.at(mapped_idx);
			candidate.description = content.names.at(mapped_idx);
			existing_marks.push_back(candidate);
			marks_added++;
			modified = true;
		}

		if (modified)
		{
			images_modified++;
			image_is_completely_empty = false;
			content.save_marks_to_disk_files(fn, existing_marks, mat.size(), 1.0, image_is_completely_empty);
		}
	}

	juce::MessageManager::callAsync([safe_content = juce::Component::SafePointer<dm::DMContent>(&content),
									 proc = total_processed,
									 mod = images_modified,
									 added = marks_added,
									 skipped = marks_skipped_overlap,
									 cancelled]()
	{
		if (safe_content != nullptr)
		{
			safe_content->load_image(safe_content->image_filename_index);
			safe_content->scrollfield.rebuild_entire_field_on_thread();

			if (cancelled)
			{
				return;
			}

			std::string msg = "Batch Auto-Labeling Complete!\n\n"
							  "Images processed: " + std::to_string(proc) + "\n"
							  "Images updated: " + std::to_string(mod) + "\n"
							  "Marks added: " + std::to_string(added);
			if (skipped > 0)
			{
				msg += "\nDuplicate marks skipped (IoU): " + std::to_string(skipped);
			}
			AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon, "DarkMark", msg);
		}
	});
}
