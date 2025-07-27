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

	class NN
	{
		public:
			NN(const std::string & onnx_filename, const std::vector<std::string>& class_names);
			~NN();
			PredictionResults predict(const cv::Mat& image, float conf_threshold = 0.3f, float nms_threshold = 0.45f) const;
			
			// Check if the model has dynamic input dimensions
			bool is_dynamic() const { return is_dynamic_input; }
			
			// Set custom input size for dynamic models (ignored for static models)
			void set_input_size(const cv::Size& size);
			
			// Get current input size
			cv::Size get_input_size() const { return input_size; }

		private:
			static Ort::SessionOptions GetSessionOptions();
			static cv::Size GetModelInputSize(Ort::Session& session, bool& is_dynamic);
			Ort::Env env;
			mutable Ort::Session session;
			Ort::AllocatorWithDefaultOptions allocator;
			Ort::MemoryInfo memory_info;

			std::vector<Ort::AllocatedStringPtr> input_node_names_char;
			std::vector<Ort::AllocatedStringPtr> output_node_names_char;

			cv::Size input_size;
			bool is_dynamic_input;
			std::vector<std::string> class_names;

			// Cached vectors to avoid repeated allocations
			mutable std::vector<float> input_tensor_values;
			mutable std::vector<int64_t> input_shape;
			mutable std::vector<const char*> input_names;
			mutable std::vector<const char*> output_names;

			void resize_unscale(const cv::Mat& mat, cv::Mat& mat_rs, float& ratio) const;
	};
}
