// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4267)
#endif
#include <onnxruntime_cxx_api.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace OnnxHelp
{
	struct PredictionResult
	{
		cv::Rect rect;
		float probability;
		int class_idx;
		std::string name;
	};

	typedef std::vector<PredictionResult> PredictionResults;

	/** Preprocessing configuration for ONNX models.
	 * Different models may require different preprocessing:
	 * - YOLOX: letterbox, [0-255] BGR
	 * - D-FINE/RT-DETR: direct resize, [0-1] BGR
	 */
	struct PreprocessConfig
	{
		bool maintain_aspect_ratio = true;  ///< true = letterbox, false = direct resize
		float scale_factor = 1.0f;          ///< 1.0 for [0-255], 1/255 for [0-1]
		bool bgr_to_rgb = false;            ///< true = convert BGR to RGB
		
		/// Preset for YOLOX-style models (letterbox, [0-255] BGR)
		static PreprocessConfig yolox() {
			return PreprocessConfig{true, 1.0f, false};
		}
		
		/// Preset for D-FINE/RT-DETR-style models (direct resize, [0-1] BGR)
		static PreprocessConfig dfine() {
			return PreprocessConfig{false, 1.0f/255.0f, false};
		}
	};

	/** Neural network class for ONNX models with DeepStream-compatible output format.
	 * 
	 * Supports any ONNX object detection model that outputs the DeepStream format:
	 * - Input: [batch, channels, height, width] (NCHW format)
	 * - Output: [batch, num_detections, 6] where 6 = [x1, y1, x2, y2, score, class_id]
	 * 
	 * Compatible models include:
	 * - YOLOX, YOLOv7, YOLOv8, YOLOv9, YOLOv10, YOLO11 (use PreprocessConfig::yolox())
	 * - D-FINE, RT-DETR, and other transformer-based detectors (use PreprocessConfig::dfine())
	 * - Any model exported with DeepStream-YOLO compatible output format
	 */
	class NN
	{
		public:
			NN(const std::string & onnx_filename, const std::vector<std::string>& class_names);
			~NN();
			PredictionResults predict(const cv::Mat& image, float conf_threshold = 0.3f, float nms_threshold = 0.45f) const;
			
			// Check if the model has dynamic input dimensions
			bool is_dynamic() const { return is_dynamic_input; }
			
			// Set custom input size for models with dynamic height/width dimensions (ignored for static models)
			void set_input_size(const cv::Size& size);
			
			// Get current input size
			cv::Size get_input_size() const { return input_size; }
			
			// Set preprocessing configuration
			void set_preprocess_config(const PreprocessConfig& config);
			
			// Get current preprocessing configuration
			PreprocessConfig get_preprocess_config() const { return preprocess_config; }

		private:
			static Ort::SessionOptions GetSessionOptions();
			static cv::Size GetModelInputSize(Ort::Session& session, bool& is_dynamic);
			static void ValidateOutputFormat(Ort::Session& session);
			Ort::Env env;
			mutable Ort::Session session;
			Ort::AllocatorWithDefaultOptions allocator;
			Ort::MemoryInfo memory_info;

			std::vector<Ort::AllocatedStringPtr> input_node_names_char;
			std::vector<Ort::AllocatedStringPtr> output_node_names_char;

			cv::Size input_size;
			bool is_dynamic_input;
			std::vector<std::string> class_names;
			PreprocessConfig preprocess_config;

			// Cached vectors to avoid repeated allocations
			mutable std::vector<float> input_tensor_values;
			mutable std::vector<int64_t> input_shape;
			mutable std::vector<const char*> input_names;
			mutable std::vector<const char*> output_names;

			void preprocess_image(const cv::Mat& mat, cv::Mat& mat_rs, float& scale_x, float& scale_y) const;
	};
}
