// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"


namespace dm
{
	/** There is one of these for every tab in the notebook.
	 * Also see @ref dm::DMReviewWnd which contains the window as well as the notebook itself.
	 */
	class DMReviewCanvas : public TableListBox, public TableListBoxModel, public ChangeBroadcaster
	{
		public:

			/// Constructor.
			DMReviewCanvas(DMContent & content, MReviewInfo & m, const MStrSize & md5s);

			/// Destructor.
			virtual ~DMReviewCanvas();

			virtual int getNumRows();
			virtual void cellDoubleClicked(int rowNumber, int columnId, const MouseEvent & event);
			virtual String getCellTooltip(int rowNumber, int columnId);
			virtual void paintRowBackground(Graphics & g, int rowNumber, int width, int height, bool rowIsSelected);
			virtual void paintCell(Graphics & g, int rowNumber, int columnId, int width, int height, bool rowIsSelected);
			virtual void cellClicked(int rowNumber, int columnId, const MouseEvent& event) override;
			virtual void sortOrderChanged(int newSortColumnId, bool isForwards) override;

			void goToAnnotation(int row);
			void removeAnnotations(const SparseSet<int>& selectedRows);
			void removeImage(const SparseSet<int>& selectedRows);

			/** This determines the order in which rows will appear.
			 * E.g., sort_idx[0] is the map index of the review info that must appear as the first row in the table.
			 */
			std::vector<size_t> sort_idx;

			/// Link back to the main content.
			DMContent & content;

			/// Map of review info, where each map record has everything needed to represent a single row in the table.
			MReviewInfo & mri;
			const MStrSize & md5s;
	};
}
