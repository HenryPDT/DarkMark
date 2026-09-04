// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"

namespace dm
{
	class DMContent;

	void load_class_names_file(const std::string & filename, std::vector<std::string> & out);

	class DMModelClassMapWnd : public DocumentWindow, public Button::Listener
	{
		public:

			DMModelClassMapWnd(
				const std::string & cfg_prefix,
				const std::vector<std::string> & project_classes,
				const std::vector<std::string> & model_classes,
				Component * parent_component = nullptr,
				DMContent * content = nullptr);

			virtual ~DMModelClassMapWnd();

			virtual void closeButtonPressed() override;
			virtual void userTriedToCloseWindow() override;
			virtual void resized() override;
			virtual void buttonClicked(Button * button) override;

		private:

			std::string cfg_prefix;
			std::vector<std::string> project_classes;
			std::vector<std::string> model_classes;
			Component::SafePointer<Component> safe_parent;
			Component::SafePointer<DMContent> safe_content;

			Component canvas;
			Label header_message;
			PropertyPanel pp_classes;
			Array<PropertyComponent*> class_props;
			std::vector<Value> class_choice_values;

			TextButton btn_reset;
			TextButton btn_save;
			TextButton btn_cancel;
	};

	/** Class-mapping dialog. Does not open the weights/ONNX file. */
	void show_model_class_mapping_dialog(
		const std::string & cfg_prefix,
		std::vector<std::string> project_classes,
		std::vector<std::string> model_classes,
		Component * parent_component = nullptr,
		DMContent * content = nullptr);
}
