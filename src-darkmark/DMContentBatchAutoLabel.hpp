// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"

namespace dm
{
	class DMContent;

	class DMContentBatchAutoLabel : public DocumentWindow, public Button::Listener, public ThreadWithProgressWindow
	{
		public:

			enum class Scope
			{
				AllImages,
				UnannotatedOnly
			};

			DMContentBatchAutoLabel(DMContent & c);

			virtual ~DMContentBatchAutoLabel();

			virtual void closeButtonPressed() override;
			virtual void userTriedToCloseWindow() override;
			virtual void resized() override;
			virtual void buttonClicked(Button * button) override;

			virtual void run() override;

		private:

			DMContent & content;
			Component::SafePointer<Component> safe_parent;
			Component canvas;

			Label lbl_title;

			Label lbl_scope;
			ComboBox cb_scope;

			Label lbl_conf;
			Slider sl_conf;

			Label lbl_nms;
			Slider sl_nms;

			ToggleButton tb_overlap;
			Label lbl_iou;
			Slider sl_iou;

			Label lbl_classes;
			PropertyPanel pp_classes;
			Array<PropertyComponent*> class_props;
			std::vector<Value> class_checkbox_values;
			TextButton btn_select_all_classes;
			TextButton btn_select_no_classes;

			TextButton btn_start;
			TextButton btn_cancel;

			// Configuration used by the background thread
			Scope scope = Scope::AllImages;
			float conf_threshold = 0.30f;
			float nms_threshold = 0.45f;
			bool overlap_suppression = true;
			double overlap_iou_threshold = 0.35;
			std::vector<bool> enabled_classes;

			size_t total_processed = 0;
			size_t images_modified = 0;
			size_t marks_added = 0;
			size_t marks_skipped_overlap = 0;
	};
}
