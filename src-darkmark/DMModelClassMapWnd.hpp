// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"

namespace dm
{
	class DMContent;

	void load_class_names_file(const std::string & filename, std::vector<std::string> & out);

	/** Modal class-mapping dialog. Does not open the weights/ONNX file. */
	void show_model_class_mapping_dialog(
		const std::string & cfg_prefix,
		std::vector<std::string> project_classes,
		std::vector<std::string> model_classes,
		DMContent * content = nullptr);
}
