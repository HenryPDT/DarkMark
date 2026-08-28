// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"


namespace dm
{
	class DMContentRemoveDuplicateMarks : public ThreadWithProgressWindow
	{
		public:

			DMContentRemoveDuplicateMarks(dm::DMContent & c);

			virtual ~DMContentRemoveDuplicateMarks();

			virtual void run();

			DMContent & content;
			size_t total_removed;
	};
}
