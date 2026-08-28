// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "DarkMark.hpp"


dm::DMContentRemoveDuplicateMarks::DMContentRemoveDuplicateMarks(dm::DMContent & c) :
	ThreadWithProgressWindow("Removing duplicate marks from every image...", true, true),
	content(c),
	total_removed(0)
{
	return;
}


dm::DMContentRemoveDuplicateMarks::~DMContentRemoveDuplicateMarks()
{
	return;
}


void dm::DMContentRemoveDuplicateMarks::run()
{
	DarkMarkApplication::setup_signal_handling();

	const auto previous_scrollfield_width = content.scrollfield_width;
	if (previous_scrollfield_width > 0)
	{
		content.scrollfield_width = 0;
		juce::MessageManager::callAsync([safe_content = juce::Component::SafePointer<dm::DMContent>(&content)]()
		{
			if (safe_content != nullptr)
			{
				safe_content->resized();
			}
		});
	}

	const double max_work = content.image_filenames.size();
	double work_completed = 0.0;

	const auto previous_predictions = content.show_predictions;
	content.show_predictions = EToggle::kOff;

	for (size_t idx = 0; idx < content.image_filenames.size(); idx ++)
	{
		if (threadShouldExit())
		{
			break;
		}

		setProgress(work_completed / max_work);
		work_completed ++;

		const std::string fn = content.image_filenames.at(idx);
		File f1 = File(fn).withFileExtension(".json");
		File f2 = f1.withFileExtension(".txt");
		if (f1.existsAsFile() or f2.existsAsFile())
		{
			content.load_image(idx);
			total_removed += content.duplicates_removed_last_load;
			Thread::yield();
		}
	}

	content.scrollfield_width = previous_scrollfield_width;
	content.show_predictions = previous_predictions;
	juce::MessageManager::callAsync([safe_content = juce::Component::SafePointer<dm::DMContent>(&content), removed = total_removed]()
	{
		if (safe_content != nullptr)
		{
			safe_content->load_image(0);
			safe_content->scrollfield.rebuild_entire_field_on_thread();
			if (removed > 0)
			{
				safe_content->show_message("removed " + std::to_string(removed) + " duplicate mark(s) from the dataset");
			}
			else
			{
				safe_content->show_message("no duplicate marks were found");
			}
		}
	});

	return;
}
