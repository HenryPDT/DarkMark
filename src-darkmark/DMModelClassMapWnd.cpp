// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include "DMContent.hpp"
#include "DMModelClassMapWnd.hpp"
#include <memory>
#include <sstream>


namespace
{
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
				if (model_id < 0)
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


dm::DMModelClassMapWnd::DMModelClassMapWnd(
	const std::string & prefix,
	const std::vector<std::string> & proj_classes,
	const std::vector<std::string> & mdl_classes,
	Component * parent_component,
	DMContent * content) :
	DocumentWindow("Model Class Mapping", Colours::darkgrey, TitleBarButtons::closeButton),
	cfg_prefix(prefix),
	project_classes(proj_classes),
	model_classes(mdl_classes),
	safe_parent(parent_component),
	safe_content(content),
	btn_reset("Reset Defaults"),
	btn_save("Save"),
	btn_cancel("Cancel")
{
	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(false);
	}

	setContentNonOwned(&canvas, false);
	setUsingNativeTitleBar(true);
	setResizable(true, true);
	setDropShadowEnabled(true);

	setIcon(DarkMarkLogo());
	ComponentPeer * peer = getPeer();
	if (peer)
	{
		peer->setIcon(DarkMarkLogo());
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
		const size_t count = project_classes.empty() ? 1 : project_classes.size();
		for (size_t i = 0; i < count; i++)
		{
			model_classes.push_back(unlabeled_name(i));
		}
	}

	std::vector<int> mapping(model_classes.size(), -1);
	default_mapping(project_classes, mapping);
	apply_saved_mapping(cfg_prefix, project_classes, model_classes, mapping);
	mapping.resize(model_classes.size(), -1);

	const String msg = has_companion_names
		? "Map each model class to a project class. Click Save when done."
		: "No companion .names file found for model. Map each model class ID to a project class.";
	header_message.setText(msg, dontSendNotification);
	header_message.setJustificationType(Justification::centred);
	header_message.setColour(Label::textColourId, Colours::white);
	canvas.addAndMakeVisible(header_message);

	StringArray choice_names;
	Array<var> choice_values;

	choice_names.add("<Ignore>");
	choice_values.add(var(-1));

	for (size_t j = 0; j < project_classes.size(); j++)
	{
		choice_names.add(String(static_cast<int>(j)) + ": " + String(project_classes[j]));
		choice_values.add(var(static_cast<int>(j)));
	}

	class_choice_values.reserve(model_classes.size());
	for (size_t i = 0; i < model_classes.size(); i++)
	{
		String label = "Model class " + String(static_cast<int>(i));
		if (model_classes[i].rfind("class ", 0) != 0)
		{
			label += " (" + String(model_classes[i]) + ")";
		}

		const int cur_val = mapping[i];
		class_choice_values.emplace_back(var(cur_val));

		auto prop = new ChoicePropertyComponent(class_choice_values.back(), label, choice_names, choice_values);
		class_props.add(prop);
	}
	pp_classes.addProperties(class_props);
	canvas.addAndMakeVisible(pp_classes);

	canvas.addAndMakeVisible(btn_reset);
	canvas.addAndMakeVisible(btn_save);
	canvas.addAndMakeVisible(btn_cancel);

	btn_reset.addListener(this);
	btn_save.addListener(this);
	btn_cancel.addListener(this);

	const int win_w = 560;
	const int win_h = 520;
	setResizeLimits(480, 400, 1600, 1200);
	centreAroundComponent(safe_parent, win_w, win_h);
	setVisible(true);
	toFront(true);

	// Re-assert proper positioning and sizing after window manager maps the window
	MessageManager::callAsync([safe_this = Component::SafePointer<DMModelClassMapWnd>(this), parent = safe_parent, win_w, win_h]()
	{
		if (safe_this != nullptr)
		{
			safe_this->centreAroundComponent(parent, win_w, win_h);
			safe_this->toFront(true);
		}
	});

	Log("DMModelClassMapWnd successfully created and displayed");
}


dm::DMModelClassMapWnd::~DMModelClassMapWnd()
{
	Log("DMModelClassMapWnd destroyed");
	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(true);
		safe_parent->toFront(true);
	}
}


void dm::DMModelClassMapWnd::closeButtonPressed()
{
	Log("DMModelClassMapWnd closing");
	setVisible(false);

	if (safe_parent != nullptr)
	{
		safe_parent->setEnabled(true);
		safe_parent->toFront(true);
	}

	MessageManager::callAsync([]()
	{
		dmapp().model_class_map_wnd.reset(nullptr);
	});
}


void dm::DMModelClassMapWnd::userTriedToCloseWindow()
{
	closeButtonPressed();
}


void dm::DMModelClassMapWnd::resized()
{
	DocumentWindow::resized();

	const int margin = 12;
	const int button_h = 28;
	const auto bounds = canvas.getLocalBounds();
	const int bottom_y = bounds.getHeight() - margin - button_h;

	header_message.setBounds(margin, margin, bounds.getWidth() - 2 * margin, 24);

	const int pp_y = margin + 32;
	const int pp_h = bottom_y - pp_y - 10;
	pp_classes.setBounds(margin, pp_y, bounds.getWidth() - 2 * margin, std::max(pp_h, 60));

	btn_reset.setBounds(margin, bottom_y, 130, button_h);
	btn_cancel.setBounds(bounds.getWidth() - margin - 100, bottom_y, 100, button_h);
	btn_save.setBounds(bounds.getWidth() - margin - 220, bottom_y, 110, button_h);
}


void dm::DMModelClassMapWnd::buttonClicked(Button * button)
{
	if (button == &btn_cancel)
	{
		closeButtonPressed();
	}
	else if (button == &btn_reset)
	{
		std::vector<int> def_map(model_classes.size(), -1);
		default_mapping(project_classes, def_map);
		for (size_t i = 0; i < class_choice_values.size(); i++)
		{
			class_choice_values[i].setValue(var(def_map[i]));
		}
		pp_classes.refreshAll();
	}
	else if (button == &btn_save)
	{
		const size_t n = model_classes.size();
		std::vector<int> saved(n, -1);
		for (size_t i = 0; i < n; i++)
		{
			saved[i] = static_cast<int>(class_choice_values[i].getValue());
			if (saved[i] >= static_cast<int>(project_classes.size()))
			{
				saved[i] = -1;
			}
		}

		std::string map_str;
		for (size_t i = 0; i < n; i++)
		{
			if (!map_str.empty())
			{
				map_str += ",";
			}
			map_str += std::to_string(i) + ":" + std::to_string(saved[i]);
		}
		cfg().setValue((cfg_prefix + "onnx_class_map").c_str(), map_str.c_str());
		Log("saved class mapping: " + map_str);

		if (safe_content != nullptr)
		{
			safe_content->model_to_project_class_map = saved;
			safe_content->model_class_names = model_classes;
			safe_content->save_model_class_mapping();
			safe_content->load_image(safe_content->image_filename_index);
			safe_content->show_message("Applied model class mapping");
		}

		closeButtonPressed();
	}
}


void dm::show_model_class_mapping_dialog(
	const std::string & cfg_prefix,
	std::vector<std::string> project_classes,
	std::vector<std::string> model_classes,
	Component * parent_component,
	DMContent * content)
{
	Log("opening model class mapping window");

	if (project_classes.empty())
	{
		AlertWindow::showMessageBoxAsync(AlertWindow::AlertIconType::InfoIcon, "DarkMark", "No project classes were found. Select a .names file first.");
		return;
	}

	if (dmapp().model_class_map_wnd)
	{
		dmapp().model_class_map_wnd->toFront(true);
		return;
	}

	dmapp().model_class_map_wnd.reset(new DMModelClassMapWnd(
		cfg_prefix,
		project_classes,
		model_classes,
		parent_component,
		content));
	dmapp().model_class_map_wnd->toFront(true);
}
