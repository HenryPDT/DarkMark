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

	// Check if height and width dimensions are dynamic (-1 indicates dynamic dimension)
	// Only consider the model dynamic for resolution changes if height (index 2) or width (index 3) are dynamic
	// Batch size (index 0) being dynamic should not allow resolution changes
	is_dynamic = false;
	if (input_shape[2] == -1 || input_shape[3] == -1)
	{
		is_dynamic = true;
	}
	
	// Log if only batch size is dynamic (for informational purposes)
	if (!is_dynamic && input_shape[0] == -1)
	{
		dm::Log("ONNX model has dynamic batch size only - resolution cannot be customized");
	}

	if (is_dynamic)
	{
		dm::Log("ONNX model has dynamic height/width dimensions - resolution can be customized");
		
		// For models with dynamic height/width dimensions, we'll use a default size but allow it to be overridden
		// Common default sizes are 640x640, 416x416, or 320x320
		int default_width = 640;
		int default_height = 640;
		
		// If height or width is not dynamic, use the specified value
		if (input_shape[2] != -1) default_height = static_cast<int>(input_shape[2]);
		if (input_shape[3] != -1) default_width = static_cast<int>(input_shape[3]);
		
		dm::Log("Using default input size for model with dynamic height/width: " + std::to_string(default_width) + "x" + std::to_string(default_height));
		return cv::Size(default_width, default_height);
	}
	else
	{
		// Static dimensions
		dm::Log("ONNX model has static height/width dimensions - resolution is fixed");
		
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

void NN::ValidateOutputFormat(Ort::Session& session)
{
	// Validate that the output has the expected DeepStream-compatible format:
	// [batch, num_detections, 6] where 6 = [x1, y1, x2, y2, score, class_id]
	// This format is compatible with models exported for DeepStream-YOLO, including:
	// - YOLOX, YOLOv7, YOLOv8, YOLOv9, YOLOv10, YOLO11
	// - D-FINE, RT-DETR, and other transformer-based detectors
	// - Any model using the DeepStream output format
	
	size_t num_output_nodes = session.GetOutputCount();
	if (num_output_nodes == 0)
	{
		throw std::runtime_error("ONNX model has no output nodes");
	}
	
	auto output_shape = session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();
	
	// Expected shape: [batch, num_detections, 6]
	if (output_shape.size() != 3)
	{
		throw std::runtime_error("ONNX model output shape is not 3D as expected for DeepStream format. "
			"Got " + std::to_string(output_shape.size()) + " dimensions. "
			"Expected shape: [batch, num_detections, 6] where 6 = [x1, y1, x2, y2, score, class_id]");
	}
	
	// The last dimension should be 6 (or -1 for dynamic, which we'll check at runtime)
	int64_t last_dim = output_shape[2];
	if (last_dim != 6 && last_dim != -1)
	{
		throw std::runtime_error("ONNX model output last dimension is " + std::to_string(last_dim) + 
			", expected 6 for DeepStream format [x1, y1, x2, y2, score, class_id]");
	}
	
	// Log the detected output format
	std::string batch_str = (output_shape[0] == -1) ? "dynamic" : std::to_string(output_shape[0]);
	std::string detections_str = (output_shape[1] == -1) ? "dynamic" : std::to_string(output_shape[1]);
	std::string attrs_str = (output_shape[2] == -1) ? "dynamic" : std::to_string(output_shape[2]);
	
	dm::Log("ONNX output format: [" + batch_str + ", " + detections_str + ", " + attrs_str + 
		"] (DeepStream-compatible: [batch, num_detections, 6])");
}

NN::NN(const std::string & onnx_filename, const std::vector<std::string>& class_names) :
	env(ORT_LOGGING_LEVEL_WARNING, "ONNX-DarkMark"),
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
	
	// Validate output format
	ValidateOutputFormat(session);
}

NN::~NN()
{
    // No need to manually free, Ort::AllocatedStringPtr handles memory automatically
}

void NN::preprocess_image(const cv::Mat& mat, cv::Mat& mat_rs, float& scale_x, float& scale_y) const
{
	if (mat.empty()) return;
	int img_height = mat.rows;
	int img_width = mat.cols;

	if (preprocess_config.maintain_aspect_ratio)
	{
		// Letterbox mode (YOLOX-style): maintain aspect ratio with padding
		mat_rs = cv::Mat(input_size.height, input_size.width, CV_8UC3, cv::Scalar(114, 114, 114));

		float ratio = std::min((float)input_size.width / (float)img_width, (float)input_size.height / (float)img_height);
		scale_x = ratio;
		scale_y = ratio;

		int new_unpad_w = static_cast<int>((float)img_width * ratio);
		int new_unpad_h = static_cast<int>((float)img_height * ratio);

		cv::Mat new_unpad_mat;
		cv::resize(mat, new_unpad_mat, cv::Size(new_unpad_w, new_unpad_h));
		new_unpad_mat.copyTo(mat_rs(cv::Rect(0, 0, new_unpad_w, new_unpad_h)));
	}
	else
	{
		// Direct resize mode (D-FINE/RT-DETR-style): resize to exact input size, ignore aspect ratio
		cv::resize(mat, mat_rs, cv::Size(input_size.width, input_size.height));
		scale_x = (float)input_size.width / (float)img_width;
		scale_y = (float)input_size.height / (float)img_height;
	}
}

void NN::set_preprocess_config(const PreprocessConfig& config)
{
	preprocess_config = config;
	dm::Log("ONNX preprocessing config: maintain_aspect_ratio=" + std::string(config.maintain_aspect_ratio ? "true" : "false") +
		" scale_factor=" + std::to_string(config.scale_factor) +
		" bgr_to_rgb=" + std::string(config.bgr_to_rgb ? "true" : "false"));
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

	float scale_x, scale_y;
	cv::Mat preprocessed_image;
	preprocess_image(image, preprocessed_image, scale_x, scale_y);

	// HWC to CHW conversion with optional BGR to RGB and pixel scaling
	const float pixel_scale = preprocess_config.scale_factor;
	const bool swap_rb = preprocess_config.bgr_to_rgb;
	
	for (int i = 0; i < input_size.height; i++) {
		for (int j = 0; j < input_size.width; j++) {
			const cv::Vec3b& pixel = preprocessed_image.at<cv::Vec3b>(i, j);
			if (swap_rb)
			{
				// BGR to RGB: swap channels 0 and 2
				input_tensor_values[0 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[2] * pixel_scale;
				input_tensor_values[1 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[1] * pixel_scale;
				input_tensor_values[2 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[0] * pixel_scale;
			}
			else
			{
				// Keep BGR order
				input_tensor_values[0 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[0] * pixel_scale;
				input_tensor_values[1 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[1] * pixel_scale;
				input_tensor_values[2 * (input_size.height * input_size.width) + i * input_size.width + j] = pixel[2] * pixel_scale;
			}
		}
	}

	auto input_tensor = Ort::Value::CreateTensor<float>(memory_info, input_tensor_values.data(), input_tensor_values.size(), input_shape.data(), input_shape.size());

	auto output_tensors = session.Run(Ort::RunOptions{nullptr}, input_names.data(), &input_tensor, 1, output_names.data(), output_names.size());

	// DeepStream-compatible output format: [batch, N, 6] where N is number of detections
	// and 6 is [x1, y1, x2, y2, score, class_id]
	// Coordinates are in pixel space relative to the model's input resolution
	auto* raw_output = output_tensors[0].GetTensorMutableData<float>();
	auto output_shape = output_tensors[0].GetTensorTypeAndShapeInfo().GetShape();
	
	// Runtime validation of output shape
	if (output_shape.size() != 3 || output_shape[2] != 6)
	{
		dm::Log("Error: Unexpected ONNX output shape. Expected [batch, N, 6], got [" +
			std::to_string(output_shape.size() > 0 ? output_shape[0] : 0) + ", " +
			std::to_string(output_shape.size() > 1 ? output_shape[1] : 0) + ", " +
			std::to_string(output_shape.size() > 2 ? output_shape[2] : 0) + "]");
		return results;
	}
	
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

			// Scale coordinates back to original image size
			// For letterbox (maintain_aspect_ratio=true): scale_x == scale_y
			// For direct resize (maintain_aspect_ratio=false): scale_x and scale_y may differ
			x1 /= scale_x;
			y1 /= scale_y;
			x2 /= scale_x;
			y2 /= scale_y;

			// Ensure x1 <= x2 and y1 <= y2 (some models may output reversed coordinates)
			if (x1 > x2) std::swap(x1, x2);
			if (y1 > y2) std::swap(y1, y2);

			// Clamp to image boundaries
			x1 = std::max(0.0f, std::min(x1, (float)image.cols - 1.0f));
			y1 = std::max(0.0f, std::min(y1, (float)image.rows - 1.0f));
			x2 = std::max(0.0f, std::min(x2, (float)image.cols - 1.0f));
			y2 = std::max(0.0f, std::min(y2, (float)image.rows - 1.0f));

			// Calculate width and height
			int ix1 = static_cast<int>(x1);
			int iy1 = static_cast<int>(y1);
			int ix2 = static_cast<int>(x2);
			int iy2 = static_cast<int>(y2);
			int width = ix2 - ix1;
			int height = iy2 - iy1;

			// Skip boxes with zero or negative dimensions
			if (width <= 0 || height <= 0)
			{
				continue;
			}

			boxes.push_back(cv::Rect(ix1, iy1, width, height));
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
	
	// Reasonable limits for object detection models
	if (size.width > 2048 || size.height > 2048)
	{
		dm::Log("Warning: Input size " + std::to_string(size.width) + "x" + std::to_string(size.height) + 
			" is very large. This may cause memory issues.");
	}
	
	// For most detection models (YOLO, DETR variants, etc.), it's common to use sizes that are multiples of 32
	if (size.width % 32 != 0 || size.height % 32 != 0)
	{
		dm::Log("Warning: Input size " + std::to_string(size.width) + "x" + std::to_string(size.height) + 
			" is not a multiple of 32. This may cause issues with some models.");
	}
	
	input_size = size;
	
	// Update cached vectors for new size
	input_tensor_values.resize(1 * 3 * input_size.height * input_size.width);
	input_shape = {1, 3, input_size.height, input_size.width};
}

} // namespace OnnxHelp
