/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ANIM_COMBINER_RGBA_HPP
#define ANIM_COMBINER_RGBA_HPP

#include "../viewer/imageData.h"

#include <vector>

enum class BlendMode{
	REPLACE,
	OVERLAY
};

enum class DisposeMode{
	NONE,       // Do nothing
	BACKGROUND, // Clear the background to transparent
	REVERT      // Revert to previous frame
};

// APNG frame combiner that works directly with RGBA8 pixel data.
// No QImage/QPainter overhead - optimized for Vulkan texture upload.
class AnimCombinerRgba {
	private:
		int width, height;
		
		// Current canvas (what's displayed)
		std::unique_ptr<uint8_t[]> canvas;

	public:
		AnimCombinerRgba( int w, int h );
		
		// Combine a new RGBA8 frame into the canvas.
		// Returns imageData with ownership of the composed frame pixels.
		imageData combine( const uint8_t* src_pixels, int src_w, int src_h,
		                   int x_offset, int y_offset,
		                   BlendMode blend, DisposeMode dispose );

	private:
		void copyRect( const uint8_t* src, int src_stride,
		               uint8_t* dst, int dst_stride,
		               int w, int h, int dx, int dy );
		
		void alphaBlendRect( const uint8_t* src, int src_stride,
		                     uint8_t* dst, int dst_stride,
		                     int w, int h, int dx, int dy );
		
		void clearRect( uint8_t* dst, int stride,
		                int x, int y, int w, int h );

};

#endif // ANIM_COMBINER_RGBA_HPP
