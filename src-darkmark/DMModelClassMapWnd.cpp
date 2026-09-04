// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include "DMContent.hpp"
#include "DMModelClassMapWnd.hpp"
#include <memory>
#include <sstream>


namespace
{
	constexpr int kMaxMappedClasses = 32;

	std::string unlabeled_name(size_t idx)
	{
		return "class " + std::to_string(idx);
	}

	void default_mapping(const std::vector<std::string> & project_classes, std::vector<int> & mapping)
	{
		for (size_t i = 0; i < mapping.size(); i++)
		{
			mapping[i] = (i < project_classes.size()) ? static_cast<int>(i) : -1;
		}
	}

	void apply_saved_mapping(
		const std::string & cfg_prefix,
		const std::vector<std::string> & project_classes,
		std::vector<std::string> & model_classes,
		std::vector<int> & mapping)
	{
		const std::string saved_map = dm::cfg().get_str(cfg_prefix + "onnx_class_map", "");
		if (saved_map.empty())
		{
			return;
		}

		std::istringstream ss(saved_map);
		std::string token;
		while (std::getline(ss, token, ','))
		{
			const auto colon = token.find(':');
			if (colon == std::string::npos)
			{
				continue;
			}
			try
			{
				const int model_id = std::stoi(token.substr(0, colon));
				int proj_id = std::stoi(token.substr(colon + 1));
				if (model_id < 0 || model_id >= kMaxMappedClasses)
				{
					continue;
				}
				while (static_cast<size_t>(model_id) >= model_classes.size())
				{
					model_classes.push_back(unlabeled_name(model_classes.size()));
					mapping.push_back(-1);
				}
				if (proj_id < 0 || static_cast<size_t>(proj_id) >= project_classes.size())
				{
					proj_id = -1;
				}
				mapping[static_cast<size_t>(model_id)] = proj_id;
			}
			catch (...) {}
		}
	}
}


void dm::load_class_names_file(const std::string & filename, std::vector<std::string> & out)
{
	out.clear();
	if (filename.empty())
	{
		return;
	}

	const String path(filename);
	const String lower = path.toLowerCase();
	if (!lower.endsWith(".names") && !lower.endsWith(".txt"))
	{
		return;
	}

	File f(filename);
	if (!f.existsAsFile() || f.getSize() > 64 * 1024)
	{
		return;
	}

	std::ifstream ifs(filename);
	std::string line;
	while (std::getline(ifs, line) && out.size() < 256)
	{
		if (line.size() > 256)
		{
			out.clear();
			return;
		}
		line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
		line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
		if (!line.empty())
		{
			out.push_back(line);
		}
	}
}


void dm::show_model_class_mapping_dialog(
	const std::string & cfg_prefix,
	std::vector<std::string> project_classes,
	std::vector<std::string> model_classes,
	DMContent * content)
{
	Log("opening AlertWindow class mapping dialog");

	if (project_classes.empty())
	{
		AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "DarkMark", "No project classes were found. Select a .names file first.");
		return;
	}

	if (project_classes.size() > 256)
	{
		project_classes.resize(256);
	}

	bool has_companion_names = false;
	for (const auto & name : model_classes)
	{
		if (name.rfind("class ", 0) != 0)
		{
			has_companion_names = true;
			break;
		}
	}

	if (model_classes.empty())
	{
		const size_t count = std::min<size_t>(project_classes.size(), static_cast<size_t>(kMaxMappedClasses));
		for (size_t i = 0; i < std::max<size_t>(count, 1); i++)
		{
			model_classes.push_back(unlabeled_name(i));
		}
	}

	if (model_classes.size() > static_cast<size_t>(kMaxMappedClasses))
	{
		model_classes.resize(static_cast<size_t>(kMaxMappedClasses));
	}

	std::vector<int> mapping(model_classes.size(), -1);
	default_mapping(project_classes, mapping);
	apply_saved_mapping(cfg_prefix, project_classes, model_classes, mapping);

	if (model_classes.size() > static_cast<size_t>(kMaxMappedClasses))
	{
		model_classes.resize(static_cast<size_t>(kMaxMappedClasses));
	}
	mapping.resize(model_classes.size(), -1);

	StringArray items;
	items.add("<Ignore>");
	for (size_t j = 0; j < project_classes.size(); j++)
	{
		items.add(String(static_cast<int>(j)) + ": " + String(project_classes[j]));
	}

	const String message = has_companion_names
		? "Map each model class to a project class."
		: "No companion .names file. Map by class ID.";

	auto * aw = new AlertWindow("Model Class Mapping", message, AlertWindow::QuestionIcon);

	const int n = static_cast<int>(model_classes.size());
	for (int i = 0; i < n; i++)
	{
		String label = "Model class " + String(i);
		if (model_classes[static_cast<size_t>(i)].rfind("class ", 0) != 0)
		{
			label += " (" + String(model_classes[static_cast<size_t>(i)]) + ")";
		}

		const String box_name = "m" + String(i);
		aw->addComboBox(box_name, items, label);

		int selected = 0;
		if (mapping[static_cast<size_t>(i)] >= 0
			&& static_cast<size_t>(mapping[static_cast<size_t>(i)]) < project_classes.size())
		{
			selected = mapping[static_cast<size_t>(i)] + 1;
		}
		if (auto * cb = aw->getComboBoxComponent(box_name))
		{
			cb->setSelectedItemIndex(selected, dontSendNotification);
		}
	}

	aw->addButton("Save", 1, KeyPress(KeyPress::returnKey));
	aw->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	const int project_n = static_cast<int>(project_classes.size());
	juce::Component::SafePointer<DMContent> safe_content(content);

	aw->enterModalState(true, ModalCallbackFunction::create(
		[aw, n, cfg_prefix, project_n, model_classes, safe_content](int result)
		{
			std::unique_ptr<AlertWindow> cleanup(aw);

			if (result != 1)
			{
				return;
			}

			std::vector<int> saved(static_cast<size_t>(n), -1);
			std::vector<std::string> names = model_classes;
			names.resize(static_cast<size_t>(n));

			for (int i = 0; i < n; i++)
			{
				auto * cb = aw->getComboBoxComponent("m" + String(i));
				const int sel = cb ? cb->getSelectedItemIndex() : 0;
				int proj_id = (sel <= 0) ? -1 : sel - 1;
				if (proj_id >= project_n)
				{
					proj_id = -1;
				}
				saved[static_cast<size_t>(i)] = proj_id;
			}

			std::string map_str;
			for (int i = 0; i < n; i++)
			{
				if (!map_str.empty())
				{
					map_str += ",";
				}
				map_str += std::to_string(i) + ":" + std::to_string(saved[static_cast<size_t>(i)]);
			}
			cfg().setValue((cfg_prefix + "onnx_class_map").c_str(), map_str.c_str());
			Log("saved class mapping: " + map_str);

			if (safe_content != nullptr)
			{
				safe_content->model_to_project_class_map = saved;
				safe_content->model_class_names = names;
				safe_content->save_model_class_mapping();
				safe_content->load_image(safe_content->image_filename_index);
				safe_content->show_message("Applied model class mapping");
			}
		}), false);
}
