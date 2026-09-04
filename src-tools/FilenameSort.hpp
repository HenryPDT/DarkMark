// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace dm
{
	/// Case-insensitive natural (numerical) less-than for filenames and paths, matching typical file managers.
	bool compare_filenames_alphabetically(const std::string_view a, const std::string_view b);

	/// Sort a vector of filenames using @ref compare_filenames_alphabetically.
	void sort_filenames_alphabetically(std::vector<std::string> & filenames);
}
