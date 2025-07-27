// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include "OnnxHelp.hpp"
#include "Tools.hpp" // for Log
#include <vector>
#include <cmath>

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

cv::Size NN::GetModelInputSize(Ort::Session& session, bool& is_dynamic)
{
	// Get the first input's shape to determine the expected input size
	size_t num_input_nodes = session.GetInputCount();
	if (num_input_nodes == 0)
	{
		throw std::runtime_error("ONNX model has no input nodes");
	}

	auto input_shape = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
	
	// ONNX models typically have shape [batch_size, channels, height, width]
	// We expect the shape to be [1, 3, height, width] or similar
	if (input_shape.size() != 4)
	{
		throw std::runtime_error("ONNX model input shape is not 4D as expected. Got " + std::to_string(input_shape.size()) + " dimensions");
	}

	// Check if any dimension is dynamic (-1 indicates dynamic dimension)
	is_dynamic = false;
	for (size_t i = 0; i < input_shape.size(); ++i)
	{
		if (input_shape[i] == -1)
		{
			is_dynamic = true;
			break;
		}
	}

	if (is_dynamic)
	{
		dm::Log("ONNX model has dynamic input dimensions");
		
		// For dynamic models, we'll use a default size but allow it to be overridden
		// Common default sizes for YOLO models are 640x640, 416x416, or 320x320
		int default_width = 640;
		int default_height = 640;
		
		// If height or width is not dynamic, use the specified value
		if (input_shape[2] != -1) default_height = static_cast<int>(input_shape[2]);
		if (input_shape[3] != -1) default_width = static_cast<int>(input_shape[3]);
		
		dm::Log("Using default input size for dynamic model: " + std::to_string(default_width) + "x" + std::to_string(default_height));
		return cv::Size(default_width, default_height);
	}
	else
	{
		// Static dimensions
		dm::Log("ONNX model has static input dimensions");
		
		// Validate that we have the expected batch size and channels
		if (input_shape[0] != 1)
		{
			dm::Log("Warning: ONNX model batch size is " + std::to_string(input_shape[0]) + ", expected 1");
		}
		
		if (input_shape[1] != 3)
		{
			dm::Log("Warning: ONNX model has " + std::to_string(input_shape[1]) + " channels, expected 3");
		}

		int height = static_cast<int>(input_shape[2]);
		int width = static_cast<int>(input_shape[3]);
		
		dm::Log("ONNX model input size detected: " + std::to_string(width) + "x" + std::to_string(height));
		
		return cv::Size(width, height);
	}
}

NN::NN(const std::string & onnx_filename, const std::vector<std::string>& class_names) :
	env(ORT_LOGGING_LEVEL_WARNING, "YOLOX-DarkMark"),
	session(env, onnx_filename.c_str(), GetSessionOptions()),
	memory_info(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
	class_names(class_names)
{
	// Automatically detect input size from the model
	input_size = GetModelInputSize(session, is_dynamic_input);
	
	// Initialize cached vectors
	input_tensor_values.resize(1 * 3 * input_size.height * input_size.width);
	input_shape = {1, 3, input_size.height, input_size.width};
	
	size_t num_input_nodes = session.GetInputCount();
	for (size_t i = 0; i < num_input_nodes; i++) {
		input_node_names_char.push_back(session.GetInputNameAllocated(i, allocator));
	}

	size_t num_output_nodes = session.GetOutputCount();
	for (size_t i = 0; i < num_output_nodes; i++) {
		output_node_names_char.push_back(session.GetOutputNameAllocated(i, allocator));
	}
	
	// Pre-populate name vectors
	input_names.reserve(input_node_names_char.size());
	output_names.reserve(output_node_names_char.size());
	for (auto& ptr : input_node_names_char) input_names.push_back(ptr.get());
	for (auto& ptr : output_node_names_char) output_names.push_back(ptr.get());
}

NN::~NN()
{
    // No need to manually free, Ort::AllocatedStringPtr handles memory automatically
}

void NN::resize_unscale(const cv::Mat& mat, cv::Mat& mat_rs, float& ratio) const
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

PredictionResults NN::predict(const cv::Mat& image, float conf_threshold, float nms_threshold) const
{
	PredictionResults results;
	if (image.empty()) return results;

	// Validate thresholds
	if (conf_threshold < 0.0f || conf_threshold > 1.0f)
	{
		dm::Log("Warning: Confidence threshold " + std::to_string(conf_threshold) + " is out of range [0,1]. Using 0.3.");
		conf_threshold = 0.3f;
	}
	if (nms_threshold < 0.0f || nms_threshold > 1.0f)
	{
		dm::Log("Warning: NMS threshold " + std::to_string(nms_threshold) + " is out of range [0,1]. Using 0.45.");
		nms_threshold = 0.45f;
	}

	float ratio;
	cv::Mat preprocessed_image;
	resize_unscale(image, preprocessed_image, ratio);

	// HWC to CHW, BGR channel order
	for (int i = 0; i < input_size.height; i++) {
		for (int j = 0; j < input_size.width; j++) {
			for (int c = 0; c < 3; c++) {
				input_tensor_values[c * (input_size.height * input_size.width) + i * input_size.width + j] = (float)preprocessed_image.at<cv::Vec3b>(i, j)[c];
			}
		}
	}

	auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

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
			// Format name with confidence percentage like Darknet does
			int confidence_percentage = static_cast<int>(std::round(scores[idx] * 100.0f));
			res.name = class_names[res.class_idx] + " " + std::to_string(confidence_percentage) + "%";
		}
		else
		{
			int confidence_percentage = static_cast<int>(std::round(scores[idx] * 100.0f));
			res.name = "class_" + std::to_string(res.class_idx) + " " + std::to_string(confidence_percentage) + "%";
		}
		results.push_back(res);
	}

	return results;
}

void NN::set_input_size(const cv::Size& size)
{
	if (!is_dynamic_input)
	{
		return;
	}
	
	// Validate the input size
	if (size.width <= 0 || size.height <= 0)
	{
		dm::Log("Error: Invalid input size " + std::to_string(size.width) + "x" + std::to_string(size.height));
		return;
	}
	
	// Reasonable limits for YOLO models
	if (size.width > 2048 || size.height > 2048)
	{
		dm::Log("Warning: Input size " + std::to_string(size.width) + "x" + std::to_string(size.height) + 
			" is very large. This may cause memory issues.");
	}
	
	// For YOLO models, it's common to use sizes that are multiples of 32
	if (size.width % 32 != 0 || size.height % 32 != 0)
	{
		dm::Log("Warning: Input size " + std::to_string(size.width) + "x" + std::to_string(size.height) + 
			" is not a multiple of 32. This may cause issues with some YOLO models.");
	}
	
	input_size = size;
	
	// Update cached vectors for new size
	input_tensor_values.resize(1 * 3 * input_size.height * input_size.width);
	input_shape = {1, 3, input_size.height, input_size.width};
}

} // namespace OnnxHelp
