// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"
#include <algorithm>

#include "json.hpp"
using json = nlohmann::json;


std::string format_bytes(double bytes)
{
	const std::vector<std::string> suffix = {"", "K", "M", "G", "T", "P"};
	size_t idx;
	for (idx = 0; idx < suffix.size() - 1; idx ++)
	{
		if (bytes < 512)
		{
			break;
		}
		bytes /= 1024.0;
	}

	std::stringstream ss;
	if (bytes < 0.0)
	{
		// do nothing, return a blank string
	}
	else if (idx == 0)
	{
		ss << static_cast<int>(bytes) << " Bytes";
	}
	else
	{
		ss << std::fixed << std::setprecision(1) << bytes << " " << suffix[idx] << "iB";
	}

	return ss.str();
}


std::string format_timestamp(const std::time_t timestamp)
{
	std::stringstream ss;

	if (timestamp > 0)
	{
		auto tm = std::localtime(&timestamp);
		char buffer[50] = "";
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm);
		ss << buffer;

		const std::time_t now = std::time(nullptr);
		if (now > timestamp)
		{
			const double second	= 1.0;
			const double minute	= 60.0	* second;
			const double hour	= 60.0	* minute;
			const double day	= 24.0	* hour;
			const double week	= 7.0	* day;
			const double year	= day	* 365.24;
			const double month	= year	/ 12.0;
			const double diff	= now - timestamp;

			if (diff > 2.0 * year)			{ ss << " (" << std::round(diff / year	) << " years ago)";		}
			else if (diff > 2.0 * month)	{ ss << " (" << std::round(diff / month	) << " months ago)";	}
			else if (diff > 2.0 * week)		{ ss << " (" << std::round(diff / week	) << " weeks ago)";		}
			else if (diff > 2.0 * day)		{ ss << " (" << std::round(diff / day	) << " days ago)";		}
			else if (diff > 2.0 * hour)		{ ss << " (" << std::round(diff / hour	) << " hours ago)";		}
			else if (diff > 2.0 * minute)	{ ss << " (" << std::round(diff / minute) << " minutes ago)";	}
		}
	}

	return ss.str();
}


std::string format_timestamp(const Time t)
{
	const std::time_t timestamp = t.toMilliseconds() / 1000;

	return format_timestamp(timestamp);
}


dm::StartupCanvas::StartupCanvas(const std::string & key, const std::string & dir) :
	Component("Startup Notebook Canvas"),
	cfg_key(key),
	hide_some_weight_files("hide extra .weights files"),
	onnx_input_size_property(nullptr),
	onnx_model_type_property(nullptr),
	darknet_template_property(nullptr),
	darknet_config_property(nullptr),
	applying_filter(true),
	done(false),
	initializing(true)
{
	addAndMakeVisible(pp);
	addAndMakeVisible(table);
	addAndMakeVisible(thumbnail);
	addAndMakeVisible(hide_some_weight_files);

	table.getHeader().addColumn("filename"	, 1, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("type"		, 2, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("size"		, 3, 100, 30, -1, TableHeaderComponent::notSortable);
	table.getHeader().addColumn("timestamp"	, 4, 100, 30, -1, TableHeaderComponent::notSortable);
	// if changing columns, also update paintCell() below

	table.getHeader().setStretchToFitActive(true);
	table.getHeader().setPopupMenuActive(false);
	table.setModel(this);

	tab_name = cfg().getValue("project_" + key + "_name", File(dir).getFileNameWithoutExtension());
	project_directory = dir.c_str();

	tab_name						.addListener(this);
	inclusion_regex					.addListener(this);
	exclusion_regex					.addListener(this);
	onnx_input_size					.addListener(this);
	darknet_configuration_template	.addListener(this);
	darknet_configuration_filename	.addListener(this);
	darknet_weights_filename		.addListener(this);
	darknet_names_filename			.addListener(this);

	Array<PropertyComponent *> properties;

	properties.add(new TextPropertyComponent(tab_name						, "name"					, 1000, false, true));
	properties.add(new TextPropertyComponent(project_directory				, "project directory"		, 1000, false, false));
	properties.add(new TextPropertyComponent(size_of_directory				, "size of directory"		, 1000, false, false));
	properties.add(new TextPropertyComponent(last_used						, "last opened"				, 1000, false, false));
	properties.add(new TextPropertyComponent(number_of_images				, "image files"				, 1000, false, false));
	properties.add(new TextPropertyComponent(number_of_json					, "markup files"			, 1000, false, false));
	properties.add(new TextPropertyComponent(number_of_classes				, "number of classes"		, 1000, false, false));
	properties.add(new TextPropertyComponent(number_of_marks				, "number of marks"			, 1000, false, false));
	properties.add(new TextPropertyComponent(newest_markup					, "newest markup"			, 1000, false, false));
	properties.add(new TextPropertyComponent(oldest_markup					, "oldest markup"			, 1000, false, false));

	auto tmp = new TextPropertyComponent(inclusion_regex					, "inclusion regex"			, 1000, false, true);
	tmp->setTooltip("Inclusion regex is used to temporarily include a subset of images from the project. The regex will be applied individually to each image path and filename.\n\nFor example, set to \"car|truck\" to include only the images that contains either \"car\" or \"truck\" anywhere in the path or filename.\n\nNormally, this field should be left blank.\n\nYou cannot set both an inclusion regex and an exclusion regex.");
	properties.add(tmp);

	tmp = new TextPropertyComponent(exclusion_regex							, "exclusion regex"			, 1000, false, true);
	tmp->setTooltip("Exclusion regex is used to temporarily exclude a subset of images from the project. The regex will be applied individually to each image path and filename.\n\nFor example, set to \"car|truck\" to exclude all images that contains either the text \"car\" or \"truck\" anywhere in the path or filename.\n\nNormally, this field should be left blank.\n\nYou cannot set both an inclusion regex and an exclusion regex.");
	properties.add(tmp);

	tmp = new TextPropertyComponent(darknet_network_dimensions				, "network dimensions"		, 1000, false, false);
	properties.add(tmp);

	tmp = new TextPropertyComponent(onnx_input_size							, "ONNX input size"			, 1000, false, true);
	tmp->setTooltip("Input size for ONNX models. For models with dynamic height/width dimensions, this can be changed. For models with static height/width dimensions, this is automatically detected and cannot be modified.");
	properties.add(tmp);
	
	// Store reference to the ONNX input size property for later modification
	onnx_input_size_property = tmp;

	tmp = new TextPropertyComponent(onnx_is_dynamic							, "ONNX model type"			, 1000, false, false);
	properties.add(tmp);
	onnx_model_type_property = tmp;

	tmp = new TextPropertyComponent(darknet_configuration_template			, "darknet template"		, 1000, false, true);
	tmp->setTooltip("The configuration template will be filled in once you select a configuration file within the Darknet Options window. It indicates which Darknet configuration file is used as a template when creating the project's configuration.");
	properties.add(tmp);
	darknet_template_property = tmp;

	properties.add(new TextPropertyComponent(darknet_configuration_filename	, "darknet configuration"	, 1000, false, true));
	darknet_config_property = properties.getLast();
	properties.add(new TextPropertyComponent(darknet_weights_filename		, "weights"					, 1000, false, true));
	properties.add(new TextPropertyComponent(darknet_names_filename			, "classes/names"			, 1000, false, true));

	pp.addProperties(properties);
	properties.clear();

	hide_some_weight_files.setToggleState(true, NotificationType::sendNotification);
	hide_some_weight_files.addListener(this);

	// Set initial state of Darknet fields based on weights file type
	if (!darknet_weights_filename.toString().isEmpty())
	{
		String weights_filename_str = darknet_weights_filename.toString();
		if (weights_filename_str.length() >= 5)
		{
			String lower_weights_filename = weights_filename_str.toLowerCase();
			if (lower_weights_filename.substring(lower_weights_filename.length() - 5) == ".onnx")
			{
				// Disable Darknet fields for ONNX models
				if (darknet_template_property)
				{
					darknet_template_property->setEnabled(false);
					darknet_template_property->setTooltip("Not applicable for ONNX models");
				}
				if (darknet_config_property)
				{
					darknet_config_property->setEnabled(false);
					darknet_config_property->setTooltip("Not applicable for ONNX models");
				}
				
				// Enable ONNX fields for ONNX models
				if (onnx_model_type_property)
				{
					onnx_model_type_property->setEnabled(true);
					onnx_model_type_property->setTooltip("Shows whether the ONNX model has static or dynamic input dimensions");
				}
			}
			else
			{
				// Disable ONNX fields for Darknet models
				if (onnx_input_size_property)
				{
					onnx_input_size_property->setEnabled(false);
					onnx_input_size_property->setTooltip("Not applicable for Darknet models");
				}
				if (onnx_model_type_property)
				{
					onnx_model_type_property->setEnabled(false);
					onnx_model_type_property->setTooltip("Not applicable for Darknet models");
				}
			}
		}
	}

	refresh();

	return;
}


dm::StartupCanvas::~StartupCanvas()
{
	done = true;
	t.join();

	return;
}


void dm::StartupCanvas::resized()
{
	const int margin_size		= 5;
	const int number_of_lines	= 19;
	const int height_per_line	= 25;
	const int total_pp_height	= number_of_lines * height_per_line;

	FlexBox bottom;
	bottom.flexDirection	= FlexBox::Direction::row;
	bottom.alignContent		= FlexBox::AlignContent::stretch;
	bottom.justifyContent	= FlexBox::JustifyContent::spaceBetween;
	bottom.items.add(FlexItem(thumbnail				).withFlex(3.0));
	bottom.items.add(FlexItem(hide_some_weight_files).withFlex(1.0));

	FlexBox fb;
	fb.flexDirection = FlexBox::Direction::column;
	fb.items.add(FlexItem(pp		).withHeight(total_pp_height));
	fb.items.add(FlexItem(/*spacer*/).withHeight(margin_size));
	fb.items.add(FlexItem(table		).withFlex(3.0));
	fb.items.add(FlexItem(/*spacer*/).withHeight(margin_size));
	fb.items.add(FlexItem(bottom	).withFlex(1.0));

	auto r = getLocalBounds();
	r.reduce(margin_size, margin_size);
	fb.performLayout(r);

	return;
}


void dm::StartupCanvas::initialize_on_thread()
{
	DarkMarkApplication::setup_signal_handling();

	bool initialize_everything = true;
	if (dmapp().cli_options.count("project_key"))
	{
		// CLI option is used to bypass the launcher, so we can skip the long parts of the init
		initialize_everything = false;
	}

	applying_filter = true;
	v.clear();
	table.updateContent();
	table.repaint();

	// give the main window a chance to start up completely before we start pounding the drive looking for files
	std::this_thread::sleep_for(std::chrono::milliseconds(100 + std::rand() % 250));

	File dir(project_directory.toString());

	VStr image_filenames;
	VStr json_filenames;
	VStr images_without_json;

	if (initialize_everything)
	{
		find_files(dir, image_filenames, json_filenames, images_without_json, done);
	}

	if (not done)
	{
		const size_t image_counter	= image_filenames.size();
		const size_t json_counter	= json_filenames.size();

		number_of_images	= String(image_counter);
		number_of_json		= String(json_counter) + " (" + String(std::round(100.0 * json_counter / (image_counter == 0 ? 1 : image_counter))) + "%)";
	}

	if (initialize_everything and image_filenames.empty() == false)
	{
		auto image = ImageCache::getFromFile(File(image_filenames[std::rand() % image_filenames.size()]));
		thumbnail.setImage(image, RectanglePlacement::xLeft);
	}

	find_all_darknet_files();

	if (initialize_everything)
	{
		calculate_size_of_directory();

		try
		{
			// look for newest and oldest timestamp
			std::size_t empty_images = 0;
			std::time_t oldest = 0;
			std::time_t newest = 0;
			std::size_t count = 0;

			for (const auto & filename : json_filenames)
			{
				if (done)
				{
					break;
				}

				json j = json::parse(File(filename).loadFileAsString().toStdString());
				count += j["mark"].size();
				if (j.value("completely_empty", false) == true)
				{
					// count empty images as well...but not the same way as marks
					empty_images ++;
				}
				std::time_t timestamp = j["timestamp"].get<std::time_t>();
				if (oldest == 0 || timestamp < oldest)
				{
					oldest = timestamp;
				}
				if (newest == 0 || timestamp > newest)
				{
					newest = timestamp;
				}
			}

			if (empty_images)
			{
				// since we have some empty images, update the text counter to include those stats as well
				const int percentage = std::round(100.0 * empty_images / json_filenames.size());
				auto str = number_of_json.toString();
				str += " of which " + String(empty_images) + " (" + String(percentage) + "%) are negative samples";
				number_of_json = str;
			}

			oldest_markup = format_timestamp(oldest).c_str();
			newest_markup = format_timestamp(newest).c_str();

			const int classes = number_of_classes.getValue();
			std::stringstream ss;
			ss << std::fixed << std::setprecision(1);
			ss << count;
			if (classes > 0 and count > 0 and json_filenames.size() > empty_images)
			{
				const double average_marks_per_class = static_cast<double>(count) / static_cast<double>(classes);
				const double average_marks_per_image = static_cast<double>(count) / static_cast<double>(json_filenames.size() - empty_images);

				ss	<< " ("
					<< average_marks_per_class << " mark" << (average_marks_per_class == 1.0 ? "" : "s") << " per class, "
					<< average_marks_per_image << " mark" << (average_marks_per_image == 1.0 ? "" : "s") << " per image, "
					<< empty_images << " negative sample" << (empty_images == 1.0 ? "" : "s") << ")";
			}
			number_of_marks = ss.str().c_str();
		}
		catch (const std::exception & e)
		{
			Log(dir.getFullPathName().toStdString() + ": error while reading JSON: " + e.what());
			oldest_markup	= "(error reading markup .json file)";
			newest_markup	= oldest_markup.toString();
			number_of_marks	= oldest_markup.toString();
		}
	}

	return;
}


void dm::StartupCanvas::find_all_darknet_files()
{
	// find all the .cfg, .weight, and .names files

	DarkMarkApplication::setup_signal_handling();

	applying_filter = true;
	v.clear();
	table.updateContent();
	table.repaint();

	extra_weights_files.clear();

	for (const auto type : {DarknetFileInfo::EType::kCfg, DarknetFileInfo::EType::kWeight, DarknetFileInfo::EType::kNames})
	{
		if (done)
		{
			break;
		}

		const String extension =
				type == DarknetFileInfo::EType::kCfg	?	"*.cfg"		:
				type == DarknetFileInfo::EType::kWeight	?	"*.weights, *.onnx" :
															"*.name*"	;

		File dir(project_directory.toString());
		for (auto dir_entry : RangedDirectoryIterator(dir, false, extension))
		{
			if (done)
			{
				break;
			}

			File f = dir_entry.getFile();

			DarknetFileInfo dfi;
			dfi.type		= type;
			dfi.full_name	= f.getFullPathName().toStdString();
			dfi.short_name	= f.getFileName().toStdString();
			dfi.file_size	= format_bytes(f.getSize());
			dfi.timestamp	= format_timestamp(f.getLastModificationTime());
			v.push_back(dfi);
		}
	}

	/* Sort by type, then within each type sort by timestamp.
	 * ...except if the name contains "_best.weights" in which case we want that to appear first!
	 */
	std::sort(v.begin(), v.end(),
			[](const DarknetFileInfo & lhs, const DarknetFileInfo & rhs)
			{
				if (lhs.type < rhs.type)
				{
					return true;
				}
				if (lhs.type == rhs.type)
				{
					if (lhs.short_name.find("_best.weights") != std::string::npos)
					{
						return true;
					}
					if (rhs.short_name.find("_best.weights") != std::string::npos)
					{
						return false;
					}

					// reverse this test so the NEWER files appear at the top prior to the older files
					if (rhs.timestamp < lhs.timestamp)
					{
						return true;
					}

					if (lhs.timestamp == rhs.timestamp)
					{
						return rhs.short_name < lhs.short_name;
					}
				}
				return false;
			} );

	if (hide_some_weight_files.getToggleState())
	{
		filter_out_extra_weight_files();
	}
	else
	{
		applying_filter = false;
		table.updateContent();
	}

	// if we don't have a .cfg, .weights, or .names file to use, then look through the list of files and choose the most recent one
	if (darknet_configuration_filename.toString().isEmpty())
	{
		for (DarknetFileInfo & info : v)
		{
			if (info.type == DarknetFileInfo::EType::kCfg)
			{
				darknet_configuration_filename = info.full_name.c_str();
				break;
			}
		}
	}
	if (darknet_weights_filename.toString().isEmpty())
	{
		for (DarknetFileInfo & info : v)
		{
			if (info.type == DarknetFileInfo::EType::kWeight)
			{
				darknet_weights_filename = info.full_name.c_str();
				break;
			}
		}
	}
	if (darknet_names_filename.toString().isEmpty())
	{
		for (DarknetFileInfo & info : v)
		{
			if (info.type == DarknetFileInfo::EType::kNames)
			{
				darknet_names_filename = info.full_name.c_str();
				break;
			}
		}
	}

	const String fn = darknet_names_filename.toString();
	if (File(fn).existsAsFile())
	{
		VStr vstr;
		std::ifstream ifs(fn.toStdString());
		std::string line;
		while (std::getline(ifs, line))
		{
			if (line.empty())
			{
				break;
			}
			vstr.push_back(line);
		}
		number_of_classes = String(vstr.size());
	}
	else
	{
		number_of_classes = "?";
	}

	return;
}


void dm::StartupCanvas::refresh()
{
	done = true;
	if (t.joinable())
	{
		t.join();
	}

	size_of_directory	= "...";
	number_of_images	= "...";
	number_of_json		= "...";
	number_of_classes	= "...";
	number_of_marks		= "...";
	newest_markup		= "...";
	oldest_markup		= "...";

	last_used = format_timestamp(cfg().getIntValue("project_" + cfg_key + "_timestamp")).c_str();

	String dims =
			String(cfg().getIntValue("project_" + cfg_key + "_darknet_image_width"	)) + " x " +
			String(cfg().getIntValue("project_" + cfg_key + "_darknet_image_height"	));
	if (dims == "0 x 0")
	{
		dims = "";
	}

	inclusion_regex					= cfg().getValue("project_" + cfg_key + "_inclusion_regex"		);
	exclusion_regex					= cfg().getValue("project_" + cfg_key + "_exclusion_regex"		);
	darknet_network_dimensions		= dims;
	
	// Initialize ONNX fields
	String onnx_dims = String(cfg().getIntValue("project_" + cfg_key + "_onnx_input_width")) + " x " +
					   String(cfg().getIntValue("project_" + cfg_key + "_onnx_input_height"));
	if (onnx_dims == "0 x 0")
	{
		onnx_dims = "Not set";  // Use "Not set" for initial state when no model is selected
	}
	onnx_input_size = onnx_dims;
	
	bool is_dynamic = cfg().get_bool("project_" + cfg_key + "_onnx_is_dynamic");
	onnx_is_dynamic = is_dynamic ? "Dynamic" : "Static";
	
	darknet_configuration_template	= cfg().getValue("project_" + cfg_key + "_darknet_cfg_template"	);
	darknet_configuration_filename	= cfg().getValue("project_" + cfg_key + "_cfg"					);
	darknet_weights_filename		= cfg().getValue("project_" + cfg_key + "_weights"				);
	darknet_names_filename			= cfg().getValue("project_" + cfg_key + "_names"				);

	// Now check if the loaded weights file is an ONNX model
	if (!darknet_weights_filename.toString().isEmpty())
	{
		String weights_filename_str = darknet_weights_filename.toString();
		if (weights_filename_str.length() >= 5)
		{
			String lower_weights_filename = weights_filename_str.toLowerCase();
			if (lower_weights_filename.substring(lower_weights_filename.length() - 5) == ".onnx")
			{
				// This is an ONNX model, keep the detected input size
				// Darknet fields will be disabled when the UI is set up
			}
			else
			{
				// This is not an ONNX model, clear ONNX fields
				onnx_input_size = "";
				onnx_is_dynamic = "";
				
				// Disable ONNX-specific fields for Darknet models
				if (onnx_input_size_property)
				{
					onnx_input_size_property->setEnabled(false);
					onnx_input_size_property->setTooltip("Not applicable for Darknet models");
				}
				if (onnx_model_type_property)
				{
					onnx_model_type_property->setEnabled(false);
					onnx_model_type_property->setTooltip("Not applicable for Darknet models");
				}
			}
		}
	}
	else
	{
		// No weights file selected, clear ONNX fields
		onnx_input_size = "";
		onnx_is_dynamic = "";
		
		// Disable ONNX-specific fields
		if (onnx_input_size_property)
		{
			onnx_input_size_property->setEnabled(false);
			onnx_input_size_property->setTooltip("Not applicable for Darknet models");
		}
		if (onnx_model_type_property)
		{
			onnx_model_type_property->setEnabled(false);
			onnx_model_type_property->setTooltip("Not applicable for Darknet models");
		}
	}

	done = false;
	initializing = false;  // End initialization phase
	t = std::thread(&StartupCanvas::initialize_on_thread, this);

	return;
}


int dm::StartupCanvas::getNumRows()
{
	if (applying_filter)
	{
		return 1;
	}

	return v.size();
}


void dm::StartupCanvas::paintRowBackground(Graphics & g, int rowNumber, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0				or
		rowNumber >= (int)v.size()	)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	Colour colour = Colours::white;
	if (rowIsSelected)
	{
		colour = Colours::lightblue; // selected rows will have a blue background
	}
	else if (applying_filter)
	{
		colour = Colours::lightyellow;
	}
	else
	{
		// does this row match ones of the key selected lines (cfg, .weights, or .names)?
		const DarknetFileInfo & info = v.at(rowNumber);
		const String full_name = info.full_name;
		if (full_name == darknet_configuration_filename	or
			full_name == darknet_weights_filename		or
			full_name == darknet_names_filename			)
		{
			colour = Colours::lightgreen;
		}
	}

	g.fillAll( colour );

	// draw the cell bottom divider between rows
	g.setColour( Colours::black.withAlpha( 0.5f ) );
	g.drawLine( 0, height, width, height );

	return;
}


void dm::StartupCanvas::paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
	if (rowNumber < 0				or
		rowNumber >= (int)v.size()	or
		columnId < 1				or
		columnId > 4				)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	std::string str;
	if (applying_filter)
	{
		if (columnId == 1)
		{
			str = "Calculating MD5 checksums...";
		}
	}
	else
	{
		const DarknetFileInfo & info = v.at(rowNumber);

		/* columns:
		 *		1: filename
		 *		2: type
		 *		3: size
		 *		4: timestamp
		 */
		switch (columnId)
		{
			case 2: str = (	info.type == DarknetFileInfo::EType::kCfg		?	"configuration"		:
							info.type == DarknetFileInfo::EType::kWeight	?	"weights"			:
																				"names"				);	break;
			case 1: str = info.short_name;																break;
			case 3: str = info.file_size;																break;
			case 4: str = info.timestamp;																break;
		}
	}

	// draw the text and the right-hand-side dividing line between cells
	g.setColour( Colours::black );
	Rectangle<int> r(0, 0, width, height);
	g.drawFittedText(str, r.reduced(2), Justification::centredLeft, 1 );

	// draw the divider on the right side of the column
	g.setColour( Colours::black.withAlpha( 0.5f ) );
	g.drawLine( width, 0, width, height );

	return;
}


void dm::StartupCanvas::selectedRowsChanged(int rowNumber)
{
	if (rowNumber < 0				or
		rowNumber >= (int)v.size()	)
	{
		// rows are 0-based, columns are 1-based
		return;
	}

	const DarknetFileInfo & info = v.at(rowNumber);
	bool onnx_processed = false;  // Flag to track if ONNX model was successfully processed

	switch (info.type)
	{
		case DarknetFileInfo::EType::kCfg:
		{
			darknet_configuration_filename = info.full_name.c_str();
			break;
		}
		case DarknetFileInfo::EType::kWeight:
		{
			darknet_weights_filename = info.full_name.c_str();
			
			// Check if this is an ONNX model and update input size (case-insensitive)
			std::string lower_filename = info.full_name;
			std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
			
			if (lower_filename.size() >= 5 && lower_filename.substr(lower_filename.size() - 5) == ".onnx")
			{
				try
				{
					// Try to load the ONNX model to detect its input size
					VStr names;
					const String names_filename = darknet_names_filename.toString();
					
					if (File(names_filename).existsAsFile())
					{
						std::ifstream ifs(names_filename.toStdString());
						std::string line;
						while (std::getline(ifs, line))
						{
							line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
							line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
							if (!line.empty()) names.push_back(line);
						}
					}
					
					// Create a temporary ONNX model to detect input size
					std::unique_ptr<OnnxHelp::NN> temp_nn;
					temp_nn.reset(new OnnxHelp::NN(info.full_name, names));
					
					// Update the input size fields
					cv::Size input_size = temp_nn->get_input_size();
					bool is_dynamic = temp_nn->is_dynamic();
					
					// Temporarily disable validation
					initializing = true;
					
					onnx_input_size = String(input_size.width) + " x " + String(input_size.height);
					onnx_is_dynamic = is_dynamic ? "Dynamic" : "Static";
					
					// Save to configuration
					cfg().setValue("project_" + cfg_key + "_onnx_input_width", input_size.width);
					cfg().setValue("project_" + cfg_key + "_onnx_input_height", input_size.height);
					cfg().setValue("project_" + cfg_key + "_onnx_is_dynamic", is_dynamic);
					
					// Control editability of input size field
					if (onnx_input_size_property)
					{
						onnx_input_size_property->setEnabled(is_dynamic);
						if (!is_dynamic)
						{
							onnx_input_size_property->setTooltip("Static model - input size is fixed and cannot be changed");
						}
						else
						{
							onnx_input_size_property->setTooltip("Model has dynamic height/width dimensions - input size can be customized");
						}
					}
					
					// Enable ONNX model type field for ONNX models
					if (onnx_model_type_property)
					{
						onnx_model_type_property->setEnabled(true);
						onnx_model_type_property->setTooltip("Shows whether the ONNX model has static or dynamic input dimensions");
					}
					
					// Disable Darknet-specific fields for ONNX models
					if (darknet_template_property)
					{
						darknet_template_property->setEnabled(false);
						darknet_template_property->setTooltip("Not applicable for ONNX models");
					}
					if (darknet_config_property)
					{
						darknet_config_property->setEnabled(false);
						darknet_config_property->setTooltip("Not applicable for ONNX models");
					}
					
					// Re-enable validation
					initializing = false;
					onnx_processed = true;  // Mark that ONNX model was successfully processed
				}
				catch (const std::exception& e)
				{
					// Temporarily disable validation
					initializing = true;
					
					// If we can't load the ONNX model, set default values
					onnx_input_size = "640 x 640";
					onnx_is_dynamic = "Unknown";
					cfg().setValue("project_" + cfg_key + "_onnx_input_width", 640);
					cfg().setValue("project_" + cfg_key + "_onnx_input_height", 640);
					cfg().setValue("project_" + cfg_key + "_onnx_is_dynamic", true);
					
					// Disable the input size field since we can't determine if it's dynamic
					if (onnx_input_size_property)
					{
						onnx_input_size_property->setEnabled(false);
						onnx_input_size_property->setTooltip("Failed to load ONNX model - input size cannot be determined");
					}
					if (onnx_model_type_property)
					{
						onnx_model_type_property->setEnabled(false);
						onnx_model_type_property->setTooltip("Failed to load ONNX model - model type cannot be determined");
					}
					
					// Re-enable Darknet-specific fields since ONNX loading failed
					if (darknet_template_property)
					{
						darknet_template_property->setEnabled(true);
						darknet_template_property->setTooltip("The configuration template will be filled in once you select a configuration file within the Darknet Options window. It indicates which Darknet configuration file is used as a template when creating the project's configuration.");
					}
					if (darknet_config_property)
					{
						darknet_config_property->setEnabled(true);
						darknet_config_property->setTooltip("Darknet configuration file");
					}
					
					// Disable ONNX-specific fields since ONNX loading failed
					if (onnx_input_size_property)
					{
						onnx_input_size_property->setEnabled(false);
						onnx_input_size_property->setTooltip("Failed to load ONNX model - input size cannot be determined");
					}
					if (onnx_model_type_property)
					{
						onnx_model_type_property->setEnabled(false);
						onnx_model_type_property->setTooltip("Failed to load ONNX model - model type cannot be determined");
					}
					
					// Re-enable validation
					initializing = false;
				}
			}
			break;
		}
		case DarknetFileInfo::EType::kNames:
		{
			darknet_names_filename = info.full_name.c_str();
			break;
		}
	}

	// If a non-ONNX weights file is selected and no ONNX model was processed, clear ONNX fields
	if (!onnx_processed)
	{
		String weights_filename_str = darknet_weights_filename.toString();
		if (weights_filename_str.length() >= 8)
		{
			String lower_weights_filename = weights_filename_str.toLowerCase();
			if (lower_weights_filename.substring(lower_weights_filename.length() - 8) != ".onnx")
			{
				// Temporarily disable validation
				initializing = true;
				
				onnx_input_size = "";
				onnx_is_dynamic = "";
				if (onnx_input_size_property)
				{
					onnx_input_size_property->setEnabled(false);
					onnx_input_size_property->setTooltip("Not an ONNX model");
				}
				
				// Re-enable Darknet-specific fields for non-ONNX models
				if (darknet_template_property)
				{
					darknet_template_property->setEnabled(true);
					darknet_template_property->setTooltip("The configuration template will be filled in once you select a configuration file within the Darknet Options window. It indicates which Darknet configuration file is used as a template when creating the project's configuration.");
				}
				if (darknet_config_property)
				{
					darknet_config_property->setEnabled(true);
					darknet_config_property->setTooltip("Darknet configuration file");
				}
				
				// Disable ONNX-specific fields for Darknet models
				if (onnx_input_size_property)
				{
					onnx_input_size_property->setEnabled(false);
					onnx_input_size_property->setTooltip("Not applicable for Darknet models");
				}
				if (onnx_model_type_property)
				{
					onnx_model_type_property->setEnabled(false);
					onnx_model_type_property->setTooltip("Not applicable for Darknet models");
				}
				
				// Re-enable validation
				initializing = false;
			}
		}
	}

	table.repaint();

	return;
}


void dm::StartupCanvas::cellClicked(int rowNumber, int columnId, const MouseEvent & event)
{
	if (event.mods.isPopupMenu())
	{
		PopupMenu m;
		m.addItem("delete unused and duplicate .weights files", (extra_weights_files.size() > 0), false, std::function<void()>( [&]{ delete_extra_weights_files(); }));
		m.showMenuAsync(PopupMenu::Options());
	}

	return;
}


void dm::StartupCanvas::delete_extra_weights_files()
{
	for (const auto & filename : extra_weights_files)
	{
		Log("deleting extra weights file: " + filename);
		File(filename).deleteFile();
	}

	calculate_size_of_directory();

	if (extra_weights_files.empty() == false)
	{
		const size_t count = extra_weights_files.size();
		AlertWindow::showMessageBox(AlertWindow::AlertIconType::InfoIcon, "DarkMark", "Deleted " + std::to_string(count) + " unused or duplicate .weights file" + (count == 1 ? "" : "s") + " from " + project_directory.toString().toStdString() + ".");
		extra_weights_files.clear();
	}

	return;
}


void dm::StartupCanvas::valueChanged(Value & value)
{
	if (value.refersToSameSourceAs(tab_name))
	{
		// this is called once the user moves focus away from the editable field, or when they press ENTER
		auto & nb = dmapp().startup_wnd->notebook;
		auto name = value.toString().trim();
		if (name.isEmpty())
		{
			name = File(project_directory.toString()).getFileNameWithoutExtension();
		}

		nb.setTabName(nb.getCurrentTabIndex(), name);
		cfg().setValue("project_" + cfg_key + "_name", name);
		value = name;
	}
	else if (value.refersToSameSourceAs(onnx_input_size))
	{
		// Skip validation during initialization
		if (initializing)
		{
			return;
		}
		
		String input_size_str = onnx_input_size.toString().trim();
		
		// Allow empty strings and special values
		if (input_size_str.isEmpty())
		{
			return;
		}
		
		// Parse the input size string (format: "width x height")
		int width = 0, height = 0;
		if (input_size_str.contains("x"))
		{
			StringArray parts = StringArray::fromTokens(input_size_str, "x", "");
			if (parts.size() == 2)
			{
				width = parts[0].trim().getIntValue();
				height = parts[1].trim().getIntValue();
			}
		}
		
		if (width > 0 && height > 0)
		{
			// Validate that the size is reasonable
			if (width < 32 || height < 32 || width > 4096 || height > 4096)
			{
				AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "DarkMark", 
					"Input size " + String(width) + "x" + String(height) + " is outside the recommended range (32-4096). This may cause issues.");
			}
			
			// Check if it's a multiple of 32 (recommended for YOLO models)
			if (width % 32 != 0 || height % 32 != 0)
			{
				AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "DarkMark", 
					"Input size " + String(width) + "x" + String(height) + " is not a multiple of 32. This may cause issues with some YOLO models.");
			}
			
			// Save to configuration
			cfg().setValue("project_" + cfg_key + "_onnx_input_width", width);
			cfg().setValue("project_" + cfg_key + "_onnx_input_height", height);
			
			// Update the display value
			value = String(width) + " x " + String(height);
		}
		else
		{
			// Invalid format, revert to previous value
			String current_width = String(cfg().getIntValue("project_" + cfg_key + "_onnx_input_width"));
			String current_height = String(cfg().getIntValue("project_" + cfg_key + "_onnx_input_height"));
			if (current_width == "0") current_width = "640";
			if (current_height == "0") current_height = "640";
			value = current_width + " x " + current_height;
			
			AlertWindow::showMessageBox(AlertWindow::AlertIconType::WarningIcon, "DarkMark", 
				"Invalid input size format. Please use format: width x height (e.g., 640 x 640)");
		}
	}
	else
	{
		table.repaint();
	}

	return;
}


void dm::StartupCanvas::buttonClicked(Button * button)
{
	if (button == &hide_some_weight_files)
	{
		// this can take a while, so start it on a thread
		// (and in the case where this toggle is set to FALSE, we have no way to get back the entries we deleted)
		done = false;
		t.join();
		t = std::thread(&StartupCanvas::find_all_darknet_files, this);
	}

	return;
}


void dm::StartupCanvas::filter_out_extra_weight_files()
{
	/* Darknet spits out many .weights files when learning ends, such as:
	 *
	 *		mailboxes_yolov3-tiny_40000.weights
	 *		mailboxes_yolov3-tiny_final.weights
	 *		mailboxes_yolov3-tiny_best.weights
	 *		mailboxes_yolov3-tiny_last.weights
	 *
	 * ...all of which may actually be exactly the same.  We want to filter out duplicates.
	 * So we'll store the MD5 hash of each .weights file, that way we'll know if this is
	 * a duplicate.
	 */
	SStr md5s;
	extra_weights_files.clear();

	VDarknetFileInfo new_files;

	applying_filter = true;
	table.updateContent();

	for (auto & info : v)
	{
		if (done)
		{
			break;
		}

		if (info.type != DarknetFileInfo::EType::kWeight)
		{
			new_files.push_back(info);
			continue;
		}

		// Apply filtering logic to both .weights and .onnx files
		bool is_weights = false, is_onnx = false;
		if (info.full_name.size() >= 8 && info.full_name.substr(info.full_name.size() - 8) == ".weights")
			is_weights = true;
		else if (info.full_name.size() >= 5 && info.full_name.substr(info.full_name.size() - 5) == ".onnx")
			is_onnx = true;

		if (is_weights || is_onnx)
		{
			if (md5s.empty() ||
				info.short_name.find("_best" + std::string(is_weights ? ".weights" : ".onnx")) != std::string::npos ||
				info.short_name.find("_last" + std::string(is_weights ? ".weights" : ".onnx")) != std::string::npos ||
				info.short_name.find("_final" + std::string(is_weights ? ".weights" : ".onnx")) != std::string::npos)
			{
				Log("calculating MD5 checksum for " + info.full_name);
				const std::string md5 = MD5(File(info.full_name)).toHexString().toStdString();
				if (md5s.count(md5) == 0)
				{
					Log("keeping the file " + info.full_name + " (md5=" + md5 + ")");
					new_files.push_back(info);
					md5s.insert(md5);
				}
				else
				{
					Log("skipping the file due to duplicate MD5 sum: " + info.full_name);
					extra_weights_files.insert(info.full_name);
				}
			}
			else
			{
				Log("skipping the intermediate file " + info.full_name);
				extra_weights_files.insert(info.full_name);
			}
			continue;
		}
	}

	applying_filter = false;

	v.swap(new_files);
	table.updateContent();

	Log(project_directory.toString().toStdString() + ": number of .weights files to skip: " + std::to_string(extra_weights_files.size()));

	return;
}


void dm::StartupCanvas::calculate_size_of_directory()
{
	int64_t total_bytes = 0;
	int64_t image_cache_bytes = 0;

#ifdef WIN32
	const String dm_image_cache = "\\darkmark_image_cache\\";
#else
	const String dm_image_cache = "/darkmark_image_cache/";
#endif

	File dir(project_directory.toString());
	for (auto dir_entry : RangedDirectoryIterator(dir, true))
	{
		if (done)
		{
			break;
		}

		total_bytes += dir_entry.getFileSize();


		if (dir_entry.getFile().getFullPathName().contains(dm_image_cache))
		{
			image_cache_bytes += dir_entry.getFileSize();
		}
	}

	if (not done)
	{
		String str = format_bytes(total_bytes);
		if (image_cache_bytes > 0)
		{
			str += " (" + String(format_bytes(image_cache_bytes)) + " of which is in the cache)";
		}

		size_of_directory = str;
	}

	return;
}
