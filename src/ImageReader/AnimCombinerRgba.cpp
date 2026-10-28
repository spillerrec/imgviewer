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

#include "AnimCombinerRgba.hpp"

#include <algorithm>
#include <cstring>

static inline void blendPixel( uint8_t* dst, const uint8_t* src ){
	const uint8_t sa = src[3];
	if( sa == 0 ) return; // Fully transparent source pixel
	if( sa == 255 ){ // Fully opaque source pixel
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		return;
	}
	
	const uint8_t da = dst[3];
	if( da == 0 ){ // Fully transparent dest pixel
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = sa;
		return;
	}
	
	const uint8_t out_alpha = sa + da - (sa * da) / 255;
	if( out_alpha == 0 ) return;
	
	const int inv_out_alpha = 256 - out_alpha;
	dst[0] = static_cast<uint8_t>( ((int)src[0]*sa*256 + (int)dst[0]*da*inv_out_alpha) / (out_alpha*256) );
	dst[1] = static_cast<uint8_t>( ((int)src[1]*sa*256 + (int)dst[1]*da*inv_out_alpha) / (out_alpha*256) );
	dst[2] = static_cast<uint8_t>( ((int)src[2]*sa*256 + (int)dst[2]*da*inv_out_alpha) / (out_alpha*256) );
	dst[3] = out_alpha;
}

AnimCombinerRgba::AnimCombinerRgba( int w, int h )
	: width(w), height(h)
{
	const size_t pixel_count = static_cast<size_t>(width) * height * 4;
	canvas.resize( pixel_count );
	previous.resize( pixel_count );
	std::memset( canvas.data(), 0, pixel_count ); // Transparent black
}

void AnimCombinerRgba::copyRect( const uint8_t* src, int src_stride,
                                 uint8_t* dst, int dst_stride,
                                 int w, int h, int dx, int dy ){
	for( int y = 0; y < h; ++y ){
		const uint8_t* src_row = src + static_cast<size_t>(y) * src_stride;
		uint8_t* dst_row = dst + static_cast<size_t>(dy + y) * dst_stride + dx * 4;
		std::memcpy( dst_row, src_row, static_cast<size_t>(w) * 4 );
	}
}

void AnimCombinerRgba::alphaBlendRect( const uint8_t* src, int src_stride,
                                       uint8_t* dst, int dst_stride,
                                       int w, int h, int dx, int dy ){
	for( int y = 0; y < h; ++y ){
		const uint8_t* src_row = src + static_cast<size_t>(y) * src_stride;
		uint8_t* dst_row = dst + static_cast<size_t>(dy + y) * dst_stride + dx * 4;
		for( int x = 0; x < w; ++x ){
			blendPixel( dst_row, src_row );
			src_row += 4;
			dst_row += 4;
		}
	}
}

void AnimCombinerRgba::clearRect( uint8_t* dst, int stride,
                                  int x, int y, int w, int h ){
	for( int row = 0; row < h; ++row ){
		uint8_t* dst_row = dst + static_cast<size_t>(y + row) * stride + x * 4;
		std::memset( dst_row, 0, static_cast<size_t>(w) * 4 );
	}
}

imageData AnimCombinerRgba::combine( const uint8_t* src_pixels, int src_w, int src_h,
                                     int x_offset, int y_offset,
                                     BlendMode blend, DisposeMode dispose ){
	const int canvas_stride = width * 4;
	
	// Clamp the source rectangle to canvas bounds
	int draw_x = std::max( x_offset, 0 );
	int draw_y = std::max( y_offset, 0 );
	int src_x = std::max( -x_offset, 0 );
	int src_y = std::max( -y_offset, 0 );
	int draw_w = std::min( src_w - src_x, width - draw_x );
	int draw_h = std::min( src_h - src_y, height - draw_y );
	
	if( draw_w <= 0 || draw_h <= 0 ) {
		// Frame is completely outside canvas; return current canvas as-is
		auto pixels = std::make_unique<uint8_t[]>(canvas.size());
		std::memcpy( pixels.get(), canvas.data(), canvas.size() );
		return imageData( width, height, std::move(pixels) );
	}
	
	const uint8_t* src_row = src_pixels + static_cast<size_t>(src_y) * (src_w * 4) + src_x * 4;
	
	// Blend the new frame onto canvas
	if( blend == BlendMode::REPLACE ) {
		copyRect( src_row, src_w * 4, canvas.data(), canvas_stride, draw_w, draw_h, draw_x, draw_y );
	} else { // OVERLAY
		alphaBlendRect( src_row, src_w * 4, canvas.data(), canvas_stride, draw_w, draw_h, draw_x, draw_y );
	}
	
	// Capture the composed frame before dispose modifies previous state
	auto output_pixels = std::make_unique<uint8_t[]>(canvas.size());
	std::memcpy( output_pixels.get(), canvas.data(), canvas.size() );
	
	// Apply dispose mode to prepare for next frame
	switch( dispose ) {
		case DisposeMode::NONE:
			previous = canvas;
			break;
			
		case DisposeMode::BACKGROUND:
			previous = canvas;
			clearRect( canvas.data(), canvas_stride, draw_x, draw_y, draw_w, draw_h );
			break;
			
		case DisposeMode::REVERT:
			if( !previous.empty() ) {
				std::memcpy( canvas.data(), previous.data(), canvas.size() );
			}
			break;
	}
	
	return imageData( width, height, std::move(output_pixels) );
}
