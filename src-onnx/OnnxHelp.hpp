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
			NN(const std::string & onnx_filename, const std::vector<std::string>& class_names, const cv::Size& input_size = cv::Size(640, 640));
			~NN();
			PredictionResults predict(cv::Mat image, float conf_threshold = 0.3f, float nms_threshold = 0.45f);

		private:
			Ort::Env env;
			Ort::Session session;
			Ort::AllocatorWithDefaultOptions allocator;
			Ort::MemoryInfo memory_info;

			std::vector<Ort::AllocatedStringPtr> input_node_names_char;
			std::vector<Ort::AllocatedStringPtr> output_node_names_char;

			cv::Size input_size;
			std::vector<std::string> class_names;

			void resize_unscale(const cv::Mat& mat, cv::Mat& mat_rs, float& ratio);
	};
}
