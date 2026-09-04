// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include "FilenameSort.hpp"

#include <algorithm>


namespace
{
#ifdef WIN32
	constexpr const char * k_path_separators = "/\\";
#else
	constexpr const char * k_path_separators = "/";
#endif

	constexpr bool is_ascii_digit(const char c) noexcept
	{
		return c >= '0' and c <= '9';
	}

	constexpr char ascii_tolower(const char c) noexcept
	{
		if (c >= 'A' and c <= 'Z')
		{
			return static_cast<char>(c + ('a' - 'A'));
		}

		return c;
	}

	int natural_string_compare(const std::string_view s1, const std::string_view s2) noexcept
	{
		size_t i1 = 0;
		size_t i2 = 0;
		const size_t n1 = s1.size();
		const size_t n2 = s2.size();

		while (i1 < n1 and i2 < n2)
		{
			if (is_ascii_digit(s1[i1]) and is_ascii_digit(s2[i2]))
			{
				size_t z1 = i1;
				while (z1 < n1 and s1[z1] == '0')
				{
					++z1;
				}

				size_t z2 = i2;
				while (z2 < n2 and s2[z2] == '0')
				{
					++z2;
				}

				size_t d1 = z1;
				while (d1 < n1 and is_ascii_digit(s1[d1]))
				{
					++d1;
				}

				size_t d2 = z2;
				while (d2 < n2 and is_ascii_digit(s2[d2]))
				{
					++d2;
				}

				const size_t zeros1 = z1 - i1;
				const size_t zeros2 = z2 - i2;
				const size_t len1 = d1 - z1;
				const size_t len2 = d2 - z2;

				if (len1 != len2)
				{
					return (len1 < len2) ? -1 : 1;
				}

				const int digit_cmp = s1.compare(z1, len1, s2, z2, len2);
				if (digit_cmp != 0)
				{
					return digit_cmp;
				}

				if (zeros1 != zeros2)
				{
					return (zeros1 < zeros2) ? -1 : 1;
				}

				i1 = d1;
				i2 = d2;
				continue;
			}

			const char c1 = ascii_tolower(s1[i1]);
			const char c2 = ascii_tolower(s2[i2]);
			if (c1 != c2)
			{
				return (c1 < c2) ? -1 : 1;
			}

			++i1;
			++i2;
		}

		if (i1 < n1)
		{
			return 1;
		}
		if (i2 < n2)
		{
			return -1;
		}

		return 0;
	}

	struct PathParts
	{
		std::string_view dir;
		std::string_view file;
	};

	PathParts split_dir_and_file(const std::string_view path) noexcept
	{
		const auto sep = path.find_last_of(k_path_separators);
		if (sep == std::string_view::npos)
		{
			return {std::string_view{}, path};
		}

		return {path.substr(0, sep), path.substr(sep + 1)};
	}

	bool filenames_less(const std::string_view a, const std::string_view b) noexcept
	{
		const PathParts pa = split_dir_and_file(a);
		const PathParts pb = split_dir_and_file(b);

		if (pa.dir != pb.dir)
		{
			const int dir_cmp = natural_string_compare(pa.dir, pb.dir);
			if (dir_cmp != 0)
			{
				return dir_cmp < 0;
			}

			return pa.dir < pb.dir;
		}

		const int file_cmp = natural_string_compare(pa.file, pb.file);
		if (file_cmp != 0)
		{
			return file_cmp < 0;
		}

		return a < b;
	}
}


bool dm::compare_filenames_alphabetically(const std::string_view a, const std::string_view b)
{
	return filenames_less(a, b);
}


void dm::sort_filenames_alphabetically(std::vector<std::string> & filenames)
{
	std::sort(filenames.begin(), filenames.end(),
		[](const std::string & lhs, const std::string & rhs)
		{
			return filenames_less(lhs, rhs);
		});
}
