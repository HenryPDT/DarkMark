// DarkMark (C) 2019-2026 Stephane Charette <stephanecharette@gmail.com>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

#ifndef DARKMARK_SOURCE_DIR
#define DARKMARK_SOURCE_DIR "../.."
#endif

TEST(CodebaseInvariants, NoRecursiveLayoutInResized)
{
	const fs::path root_dir(DARKMARK_SOURCE_DIR);
	ASSERT_TRUE(fs::exists(root_dir)) << "DarkMark source directory does not exist at: " << root_dir;

	const std::vector<std::string> src_subdirs = {
		"src-darkmark",
		"src-darknet",
		"src-classid",
		"src-launcher",
		"src-wnd",
		"src-tools",
		"src-main"
	};

	std::vector<std::string> violations;

	for (const auto & subdir : src_subdirs)
	{
		const fs::path full_subdir = root_dir / subdir;
		if (!fs::exists(full_subdir))
		{
			continue;
		}

		for (const auto & entry : fs::recursive_directory_iterator(full_subdir))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".cpp")
			{
				continue;
			}

			std::ifstream file(entry.path());
			if (!file.is_open())
			{
				continue;
			}

			std::string line;
			int line_num = 0;
			bool in_resized = false;
			bool is_document_window_resized = false;
			int brace_depth = 0;
			int resized_start_line = 0;
			std::vector<std::pair<int, std::string>> pending_warnings;

			while (std::getline(file, line))
			{
				line_num++;

				if (!in_resized)
				{
					// Match definition of any ::resized() method
					if (line.find("::resized()") != std::string::npos && line.find("void ") != std::string::npos)
					{
						in_resized = true;
						is_document_window_resized = false;
						brace_depth = 0;
						resized_start_line = line_num;
						pending_warnings.clear();
					}
				}

				if (in_resized)
				{
					for (char c : line)
					{
						if (c == '{') brace_depth++;
						if (c == '}') brace_depth--;
					}

					if (line.find("DocumentWindow::resized()") != std::string::npos)
					{
						is_document_window_resized = true;
					}

					// Check for canvas.setBounds inside resized()
					if (line.find("canvas.setBounds(") != std::string::npos)
					{
						pending_warnings.push_back({line_num, line});
					}

					// Method ended when opening brace has been encountered and closed
					if (brace_depth == 0 && line_num > resized_start_line)
					{
						if (is_document_window_resized && !pending_warnings.empty())
						{
							for (const auto & w : pending_warnings)
							{
								violations.push_back(
									entry.path().string() + ":" + std::to_string(w.first) +
									": Infinite layout recursion hazard! Calling 'canvas.setBounds' inside DocumentWindow::resized() causes X11/KWin freeze."
								);
							}
						}
						in_resized = false;
						pending_warnings.clear();
					}
				}
			}
		}
	}

	EXPECT_TRUE(violations.empty()) << "Found " << violations.size() << " layout recursion violations:\n"
		<< [&]() {
			std::string msg;
			for (const auto & v : violations) msg += "  - " + v + "\n";
			return msg;
		}();
}

TEST(CodebaseInvariants, NoModalLoopOnSecondaryWindows)
{
	const fs::path root_dir(DARKMARK_SOURCE_DIR);
	ASSERT_TRUE(fs::exists(root_dir)) << "DarkMark source directory does not exist at: " << root_dir;

	const std::vector<std::string> target_subdirs = {
		"src-darkmark",
		"src-darknet",
		"src-classid",
		"src-wnd"
	};

	std::vector<std::string> violations;

	for (const auto & subdir : target_subdirs)
	{
		const fs::path full_subdir = root_dir / subdir;
		if (!fs::exists(full_subdir))
		{
			continue;
		}

		for (const auto & entry : fs::recursive_directory_iterator(full_subdir))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".cpp")
			{
				continue;
			}

			std::ifstream file(entry.path());
			if (!file.is_open())
			{
				continue;
			}

			std::string line;
			int line_num = 0;

			while (std::getline(file, line))
			{
				line_num++;

				// Disallow runModalLoop on DMModelClassMapWnd or DMContentBatchAutoLabel
				if (line.find(".runModalLoop()") != std::string::npos || line.find("->runModalLoop()") != std::string::npos)
				{
					if (line.find("model_class_map") != std::string::npos ||
						line.find("batch_autolabel") != std::string::npos ||
						line.find("BatchAutoLabel") != std::string::npos ||
						line.find("DMModelClassMapWnd") != std::string::npos)
					{
						violations.push_back(
							entry.path().string() + ":" + std::to_string(line_num) +
							": Modal loop deadlock hazard! Use fake-modal pattern instead of runModalLoop."
						);
					}
				}
			}
		}
	}

	EXPECT_TRUE(violations.empty()) << "Found " << violations.size() << " modal loop violations:\n"
		<< [&]() {
			std::string msg;
			for (const auto & v : violations) msg += "  - " + v + "\n";
			return msg;
		}();
}
