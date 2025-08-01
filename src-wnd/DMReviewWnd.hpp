// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"


namespace dm
{
	struct ReviewInfo
	{
		cv::Mat mat;
		std::string filename;
		size_t class_idx;
		cv::Rect r;
		double overlap_sum; // the total amount of overlap between this mark and all other marks in this image
		std::string mime_type;
		std::string md5;
		VStr warnings;
		VStr errors;
	};

	/** Key is a sequential counter that starts at zero, value is the review info structure.  Was done this way instead of
	 * std::vector because there is no reason why the entries need to be in a single large block of memory, yet we need to
	 * easily retrieve an object with operator[] when we draw the table in the review.
	 *
	 * 2021-05-12: ...and yet here I am two years later trying to shoehorn sorting into the tables!
	 */
	typedef std::map<size_t, ReviewInfo> MReviewInfo;

	/** Multiple MReviewInfo objects, typically one for each class defined.  The key is the class index, the value is the
	 * review info map.
	 */
	typedef std::map<size_t, MReviewInfo> MMReviewInfo;

	/// Also see the class @ref DMReviewCanvas which is the content shown in the notebook.
	class DMReviewWnd : public DocumentWindow, public ChangeListener
	{
		public:

			DMReviewWnd(DMContent & c);

			virtual ~DMReviewWnd();

			virtual void closeButtonPressed();
			virtual void userTriedToCloseWindow();

			void rebuild_notebook();
			virtual void changeListenerCallback(ChangeBroadcaster* source) override;

			DMContent & content;
			Notebook notebook;
			MMReviewInfo m;
			MStrSize md5s;
	};
}
