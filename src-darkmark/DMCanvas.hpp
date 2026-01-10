// DarkMark (C) 2019-2024 Stephane Charette <stephanecharette@gmail.com>

#pragma once

#include "DarkMark.hpp"


namespace dm
{
	/** This is the actual class that draws the current image and all of the annotations/marks.  Most of the work is performed
	 * in @ref rebuild_cache_image().  Also of importance is the mouse event handling to ensure that marks are created and
	 * stretched correctly.
	 *
	 * This class is one of the children of @ref DMContent.
	 */
	class DMCanvas : public CrosshairComponent
	{
		public:

			DMCanvas(DMContent & c);

			virtual ~DMCanvas();

			virtual void rebuild_cache_image() override;

			virtual void mouseDown(const MouseEvent & event) override;
			virtual void mouseDoubleClick(const MouseEvent & event) override;
			virtual void mouseDrag(const MouseEvent & event) override;
			virtual void mouseDragFinished(juce::Rectangle<int> drag_rect, const MouseEvent & event) override;
			virtual void mouseWheelMove(const MouseEvent & event, const MouseWheelDetails & wheel) override;

			/// Link to the parent which manages the content, including all the marks.
			DMContent & content;

		/// If the CTRL key is held down while zooming, then we'll pan the image instead of creating a bounding box.
		bool is_panning;
		
		/// Track if the mark being resized was selected for merge (to restore selection after resize)
		bool mark_was_selected_for_merge;
		
		/// Track the original position of the mark being resized in the selection vector
		int mark_original_selection_position;
	};
}
