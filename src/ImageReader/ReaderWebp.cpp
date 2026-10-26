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

#include "ReaderWebp.hpp"

#include <QDebug>
#include <qlogging.h>

#include <tracy/Tracy.hpp>

#include "../viewer/imageData.h"

#include <cstring>

extern "C"{
#include <webp/decode.h>
#include <webp/demux.h>
}


bool ReaderWebp::can_read( const uint8_t* data, unsigned length, QString ) const{
	// WebP files start with RIFF....WEBP magic bytes
	if( length < 12 )
		return false;

	return true;
	return std::memcmp( data, "RIFF", 4 ) == 0 &&
	       std::memcmp( data + 8, "WEBP", 4 ) == 0;
}


AReader::Error ReaderWebp::read( imageCache &cache, const uint8_t* data, unsigned length, QString format ) const{
	if( !can_read( data, length, format ) )
		return ERROR_TYPE_UNKNOWN;
	ZoneScoped;

	// Try demuxing to detect animation and get metadata
	WebPData webp_data;
	webp_data.bytes = data;
	webp_data.size = length;

	WebPDemuxer* demux = WebPDemux( &webp_data );
	if( !demux )
		return ERROR_FILE_BROKEN;

	// Use iterator API since WebPDemuxer is opaque in newer libwebp
	WebPIterator iter;
	if( !WebPDemuxGetFrame( demux, 1, &iter ) ) {
		WebPDemuxDelete( demux );
		return ERROR_FILE_BROKEN;
	}

	int width = iter.width;
	int height = iter.height;
	int num_frames = iter.num_frames;
	// loop_count is not exposed in the opaque API; use 0 (infinite) as default for animated WebP
	int loop_count = 0;

	WebPDemuxReleaseIterator( &iter );

	if( width <= 0 || height <= 0 || num_frames <= 0 ) {
		WebPDemuxDelete( demux );
		return ERROR_FILE_BROKEN;
	}

	bool is_animated = (num_frames > 1);

	qDebug() << "[ReaderWebp] Detected:" << width << "x" << height << ", frames:" << num_frames << ", animated:" << is_animated;

	cache.set_info( num_frames, is_animated, loop_count );

	if( !is_animated ) {
		// Static WebP: decode directly
		WebPDemuxDelete( demux );

		uint8_t* rgba = WebPDecodeRGBA( data, length, &width, &height );
		if( !rgba )
			return ERROR_FILE_BROKEN;

		std::unique_ptr<uint8_t[]> pixels( rgba );
		cache.add_frame( imageData( width, height, std::move(pixels) ), 0 );
		return ERROR_NONE;
	}

	// Animated WebP: use the decoder for proper frame composition
	// WebPAnimDecoder returns full-canvas composited frames with disposal already applied
	WebPDemuxDelete( demux );

	qDebug() << "[ReaderWebp] Attempting animated decode:" << width << "x" << height << num_frames << "frames, data size:" << length;

	// Try with NULL options first (use defaults)
	WebPAnimDecoder* dec = WebPAnimDecoderNew( &webp_data, nullptr );
	if( !dec ) {
		qDebug() << "[ReaderWebp] WebPAnimDecoderNew returned NULL even with default options";
		return ERROR_INITIALIZATION;
	}

	int last_timestamp = 0;
	size_t frame_size = width * height * 4; // RGBA bytes per full-canvas frame

	for( int i = 0; i < num_frames; i++ ){
		ZoneScoped;
		uint8_t* rgba = nullptr;
		int timestamp = 0;

		if( !WebPAnimDecoderGetNext( dec, &rgba, &timestamp ) ) {
			WebPAnimDecoderDelete( dec );
			return ERROR_FILE_BROKEN;
		}

		// WebPAnimDecoder reuses an internal buffer; we must copy each frame.
		std::unique_ptr<uint8_t[]> pixels( new uint8_t[frame_size] );
		std::memcpy( pixels.get(), rgba, frame_size );

		// Convert cumulative timestamp to per-frame delay in ms
		int delay = timestamp - last_timestamp;
		last_timestamp = timestamp;
		if( delay == 0 )
			delay = 100; // default for zero-delay frames

		cache.add_frame( imageData( width, height, std::move(pixels) ), delay );
	}

	WebPAnimDecoderDelete( dec );
	return ERROR_NONE;
}
