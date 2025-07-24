// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include "OnnxHelp.hpp"
#include "Tools.hpp" // for Log
#include <vector>

namespace OnnxHelp
{
Ort::SessionOptions NN::GetSessionOptions()
{
	Ort::SessionOptions session_options;
	session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

	// Try to use CUDA execution provider if available
	auto available_providers = Ort::GetAvailableProviders();
	bool cuda_available = false;
	for (const auto& p : available_providers)
	{
		if (p == "CUDAExecutionProvider")
		{
			cuda_available = true;
			break;
		}
	}

	if (cuda_available)
	{
		dm::Log("ONNX Runtime: Using CUDA execution provider.");
		OrtCUDAProviderOptions cuda_options{};
		session_options.AppendExecutionProvider_CUDA(cuda_options);
	}
	else
	{
		dm::Log("ONNX Runtime: CUDA execution provider not available. Using CPU.");
	}

	return session_options;
}
NN::NN(const std::string & onnx_filename, const std::vector<std::string>& class_names, const cv::Size& input_size) :
	env(ORT_LOGGING_LEVEL_WARNING, "YOLOX-DarkMark"),
	session(env, onnx_filename.c_str(), GetSessionOptions()),
	memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
	input_size(input_size),
	class_names(class_names)
{
	size_t num_input_nodes = session.GetInputCount();
	for (size_t i = 0; i < num_input_nodes; i++) {
		input_node_names_char.push_back(session.GetInputNameAllocated(i, allocator));
	}

	size_t num_output_nodes = session.GetOutputCount();
	for (size_t i = 0; i < num_output_nodes; i++) {
		output_node_names_char.push_back(session.GetOutputNameAllocated(i, allocator));
	}
}

NN::~NN()
{
    // No need to manually free, Ort::AllocatedStringPtr handles memory automatically
}

void NN::resize_unscale(const cv::Mat& mat, cv::Mat& mat_rs, float& ratio)
{
	if (mat.empty()) return;
	int img_height = mat.rows;
	int img_width = mat.cols;

	mat_rs = cv::Mat(input_size.height, input_size.width, CV_8UC3, cv::Scalar(114, 114, 114));

	ratio = std::min((float)input_size.width / (float)img_width, (float)input_size.height / (float)img_height);

	int new_unpad_w = static_cast<int>((float)img_width * ratio);
	int new_unpad_h = static_cast<int>((float)img_height * ratio);

	cv::Mat new_unpad_mat;
	cv::resize(mat, new_unpad_mat, cv::Size(new_unpad_w, new_unpad_h));
	new_unpad_mat.copyTo(mat_rs(cv::Rect(0, 0, new_unpad_w, new_unpad_h)));
}

PredictionResults NN::predict(cv::Mat image, float conf_threshold, float nms_threshold)
{
	PredictionResults results;
	if (image.empty()) return results;

	float ratio;
	cv::Mat preprocessed_image;
	resize_unscale(image, preprocessed_image, ratio);

	std::vector<float> input_tensor_values(1 * 3 * input_size.height * input_size.width);

	// HWC to CHW, BGR channel order
	for (int i = 0; i < input_size.height; i++) {
		for (int j = 0; j < input_size.width; j++) {
			for (int c = 0; c < 3; c++) {
				input_tensor_values[c * (input_size.height * input_size.width) + i * input_size.width + j] = (float)preprocessed_image.at<cv::Vec3b>(i, j)[c];
			}
		}
	}

	std::vector<int64_t> input_shape = {1, 3, input_size.height, input_size.width};
	auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

	std::vector<const char*> input_names;
	for (auto& ptr : input_node_names_char) input_names.push_back(ptr.get());
	std::vector<const char*> output_names;
	for (auto& ptr : output_node_names_char) output_names.push_back(ptr.get());

	auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1, output_names.data(), output_names.size());

	// The output from export_yolox.py is [1, N, 6] where N is number of detections and 6 is [x1, y1, x2, y2, score, class_id]
	auto* raw_output = output_tensors[0].GetTensorMutableData<float>();
	auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
	size_t num_detections = output_shape[1];

	std::vector<cv::Rect> boxes;
	std::vector<float> scores;
	std::vector<int> class_indices;

	for(size_t i = 0; i < num_detections; ++i)
	{
		float score = raw_output[i * 6 + 4];
		if(score > conf_threshold)
		{
			float x1 = raw_output[i * 6 + 0];
			float y1 = raw_output[i * 6 + 1];
			float x2 = raw_output[i * 6 + 2];
			float y2 = raw_output[i * 6 + 3];

			x1 /= ratio;
			y1 /= ratio;
			x2 /= ratio;
			y2 /= ratio;

			x1 = std::max(0.0f, x1);
			y1 = std::max(0.0f, y1);
			x2 = std::min((float)image.cols - 1.0f, x2);
			y2 = std::min((float)image.rows - 1.0f, y2);

			boxes.push_back(cv::Rect(cv::Point((int)x1, (int)y1), cv::Point((int)x2, (int)y2)));
			scores.push_back(score);
			class_indices.push_back((int)raw_output[i * 6 + 5]);
		}
	}

	std::vector<int> nms_result;
	cv::dnn::NMSBoxes(boxes, scores, conf_threshold, nms_threshold, nms_result);

	for (int idx : nms_result)
	{
		PredictionResult res;
		res.rect = boxes[idx];
		res.probability = scores[idx];
		res.class_idx = class_indices[idx];
		if (static_cast<size_t>(res.class_idx) < class_names.size())
		{
			res.name = class_names[res.class_idx];
		}
		else
		{
			res.name = "class_" + std::to_string(res.class_idx);
		}
		results.push_back(res);
	}

	return results;
}

} // namespace OnnxHelp
