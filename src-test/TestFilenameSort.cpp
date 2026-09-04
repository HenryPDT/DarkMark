// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#include <gtest/gtest.h>
#include "FilenameSort.hpp"

#include <algorithm>
#include <string>
#include <vector>


namespace
{
	void expect_less(const std::string_view a, const std::string_view b)
	{
		EXPECT_TRUE(dm::compare_filenames_alphabetically(a, b)) << "'" << std::string(a) << "' should sort before '" << std::string(b) << "'";
		EXPECT_FALSE(dm::compare_filenames_alphabetically(b, a)) << "'" << std::string(b) << "' should not sort before '" << std::string(a) << "'";
	}

	void expect_equivalent(const std::string_view a, const std::string_view b)
	{
		EXPECT_FALSE(dm::compare_filenames_alphabetically(a, b));
		EXPECT_FALSE(dm::compare_filenames_alphabetically(b, a));
	}
}


TEST(FilenameSort, Irreflexive)
{
	expect_equivalent("img2.jpg", "img2.jpg");
	expect_equivalent("/data/cam1/frame10.png", "/data/cam1/frame10.png");
	expect_equivalent("", "");
}


TEST(FilenameSort, NaturalNumericOrder)
{
	expect_less("img2.jpg", "img10.jpg");
	expect_less("img10.jpg", "img11.jpg");
	expect_less("frame2.png", "frame02.png");
	expect_less("file1.2.jpg", "file1.10.jpg");
	expect_less("a0.jpg", "a1.jpg");
	expect_less("a0.jpg", "a00.jpg");
}


TEST(FilenameSort, CaseInsensitiveWithByteTieBreak)
{
	expect_less("apple.jpg", "Banana.jpg");
	expect_less("Banana.jpg", "banana.jpg");
	expect_less("IMG2.jpg", "img10.jpg");
}


TEST(FilenameSort, DirectoryThenFilename)
{
	expect_less("/proj/cam1.jpg", "/proj/cam10.jpg");
	expect_less("/proj/cam1.jpg", "/proj/cam1/frame.jpg");
	expect_less("/proj/cam10.jpg", "/proj/cam1/frame.jpg");
	expect_less("/data/cam1/z.jpg", "/data/cam2/a.jpg");
	expect_less("/data/set2/a.jpg", "/data/set10/a.jpg");
}


TEST(FilenameSort, BareFilenames)
{
	expect_less("img2.jpg", "img10.jpg");
	expect_less("a.jpg", "b.jpg");
}


TEST(FilenameSort, SortVector)
{
	std::vector<std::string> files =
	{
		"/proj/cam10/frame.jpg",
		"/proj/cam2/frame.jpg",
		"/proj/cam1/frame10.jpg",
		"/proj/cam1/frame2.jpg",
		"/proj/cam1.jpg",
	};

	dm::sort_filenames_alphabetically(files);

	const std::vector<std::string> expected =
	{
		"/proj/cam1.jpg",
		"/proj/cam1/frame2.jpg",
		"/proj/cam1/frame10.jpg",
		"/proj/cam2/frame.jpg",
		"/proj/cam10/frame.jpg",
	};

	EXPECT_EQ(files, expected);
}


TEST(FilenameSort, StrictWeakOrdering)
{
	const std::vector<std::string> files =
	{
		"",
		"img.jpg",
		"img2.jpg",
		"img10.jpg",
		"IMG2.jpg",
		"img02.jpg",
		"/proj/cam1.jpg",
		"/proj/cam1/frame.jpg",
		"/proj/cam2/frame.jpg",
		"/proj/Cam1/frame.jpg",
		"/proj/cam10/frame.jpg",
		"Banana.jpg",
		"apple.jpg",
		"file1.2.jpg",
		"file1.10.jpg",
	};

	for (const auto & a : files)
	{
		EXPECT_FALSE(dm::compare_filenames_alphabetically(a, a)) << a;
	}

	for (const auto & a : files)
	{
		for (const auto & b : files)
		{
			const bool ab = dm::compare_filenames_alphabetically(a, b);
			const bool ba = dm::compare_filenames_alphabetically(b, a);
			EXPECT_FALSE(ab and ba) << a << " vs " << b;

			if (ab)
			{
				for (const auto & c : files)
				{
					if (dm::compare_filenames_alphabetically(b, c))
					{
						EXPECT_TRUE(dm::compare_filenames_alphabetically(a, c)) << a << " < " << b << " < " << c;
					}
				}
			}
		}
	}
}


TEST(FilenameSort, SortIsDeterministic)
{
	std::vector<std::string> files =
	{
		"/data/set10/b.jpg",
		"/data/set2/a.jpg",
		"/data/Set2/c.jpg",
		"img10.jpg",
		"img2.jpg",
		"IMG2.jpg",
	};

	auto once = files;
	dm::sort_filenames_alphabetically(once);

	std::reverse(files.begin(), files.end());
	dm::sort_filenames_alphabetically(files);

	EXPECT_EQ(files, once);
}
