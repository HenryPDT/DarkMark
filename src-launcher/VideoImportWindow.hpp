// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"
#include <OnnxHelp.hpp>


namespace dm
{
	struct UnifiedPredictionResult;

	class VideoImportWindow : public DocumentWindow, public Button::Listener, public ThreadWithProgressWindow
	{
		public:

			VideoImportWindow(const std::string & dir, const VStr & v);

			virtual ~VideoImportWindow();

			virtual void resized() override;
			virtual void closeButtonPressed() override;
			virtual void userTriedToCloseWindow() override;
			virtual void buttonClicked(Button * button) override;

			virtual void run() override;

			enum class ModelType { None, Darknet, ONNX };
			ModelType selected_model_type;

			std::string darknet_weights_path;
			std::string darknet_cfg_path;
			std::string darknet_names_path;
			std::string onnx_model_path;
			std::string onnx_names_path;

			std::unique_ptr<DarkHelp::NN> temp_darknet_nn;
			std::unique_ptr<OnnxHelp::NN> temp_onnx_nn;
			VStr class_names;

			float confidence_threshold;
			float nms_threshold;
			float hierarchy_threshold;
			bool enable_tiling;

			void update_model_type_ui();
			void load_selected_model();
			void clear_model();
			bool validate_model_files();
			std::vector<UnifiedPredictionResult> run_inference(const cv::Mat& frame);
			void generate_annotation_file(const std::string& base_path, const std::vector<UnifiedPredictionResult>& predictions, const cv::Size& image_size);
			void load_darknet_model();
			void load_onnx_model();

			const std::string base_directory;
			const VStr		filenames;

			Component canvas;

			Label			header_message;
			ToggleButton	tb_extract_all;
			ToggleButton	tb_extract_sequences;
			Slider			sl_sequences;
			Label			txt_sequences;
			Slider			sl_consecutive_frames;
			Label			txt_consecutive_frames;
			ToggleButton	tb_extract_maximum;
			Slider			sl_maximum;
			ToggleButton	tb_extract_percentage;
			Slider			sl_percentage;
			ToggleButton	tb_do_not_resize;
			ToggleButton	tb_resize;
			TextEditor		ef_width;
			Label			txt_x;
			TextEditor		ef_height;
			ToggleButton	tb_keep_aspect_ratio;
			ToggleButton	tb_force_resize;
			ToggleButton	tb_save_as_png;
			ToggleButton	tb_save_as_jpeg;
			Label			txt_jpeg_quality;
			Slider			sl_jpeg_quality;
			TextButton		cancel;
			TextButton		ok;

			ToggleButton    tb_enable_auto_annotation;
			ToggleButton    tb_model_type_darknet;
			ToggleButton    tb_model_type_onnx;

			TextButton      btn_select_darknet_weights;
			TextButton      btn_select_darknet_cfg;
			TextButton      btn_select_darknet_names;
			Label           lbl_darknet_weights;
			Label           lbl_darknet_cfg;
			Label           lbl_darknet_names;

			TextButton      btn_select_onnx_model;
			TextButton      btn_select_onnx_names;
			Label           lbl_onnx_model;
			Label           lbl_onnx_names;

			ToggleButton    tb_import_with_detections;
			ToggleButton    tb_import_without_detections;
			ToggleButton    tb_import_all_frames;

			Label           txt_confidence_threshold;
			Slider          sl_confidence_threshold;
			Label           txt_nms_threshold;
			Slider          sl_nms_threshold;

			ToggleButton    tb_enable_tiling;
			Label           txt_hierarchy_threshold;
			Slider          sl_hierarchy_threshold;

			size_t			extra_lines_needed;
			size_t			number_of_processed_frames;
	};

	struct UnifiedPredictionResult {
		cv::Rect rect;
		float probability;
		int class_idx;
		std::string name;

		UnifiedPredictionResult(const DarkHelp::PredictionResult& dh_result, const VStr& names);
		UnifiedPredictionResult(const OnnxHelp::PredictionResult& onnx_result);
	};
}
