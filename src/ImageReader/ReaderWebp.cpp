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
#include <memory>

extern "C"{
#include <webp/decode.h>
#include <webp/demux.h>
}


bool ReaderWebp::can_read( const uint8_t* data, unsigned length, QString ) const{
	// WebP files start with RIFF....WEBP magic bytes
	if( length < 12 )
		return false;

	return std::memcmp( data, "RIFF", 4 ) == 0 &&
	       std::memcmp( data + 8, "WEBP", 4 ) == 0;
}


AReader::Error ReaderWebp::read( imageCache &cache, const uint8_t* data, unsigned length, QString format ) const{
	if( !can_read( data, length, format ) )
		return ERROR_TYPE_UNKNOWN;
	ZoneScoped;

	WebPData webp_data;
	webp_data.bytes = data;
	webp_data.size = length;

	// Create decoder for all WebPs (static or animated)
	struct WebPAnimDecoderDeleter {
		void operator()( WebPAnimDecoder* dec ) const { WebPAnimDecoderDelete( dec ); }
	};

	std::unique_ptr<WebPAnimDecoder, WebPAnimDecoderDeleter> dec(WebPAnimDecoderNew( &webp_data, nullptr ));
	if( !dec ) 
		return ERROR_INITIALIZATION;

	// Get global info (works for single-frame images too)
	WebPAnimInfo info;
	if( !WebPAnimDecoderGetInfo( dec.get(), &info ) )
		return ERROR_FILE_BROKEN;

	int width = static_cast<int>( info.canvas_width );
	int height = static_cast<int>( info.canvas_height );

	if( width <= 0 || height <= 0 || info.frame_count <= 0 )
		return ERROR_FILE_BROKEN;

	bool is_animated = (info.frame_count > 1);
	cache.set_info( info.frame_count, is_animated, info.loop_count > 0 ? int(info.loop_count) : -1 );

	int last_timestamp = 0;
	for( uint32_t i = 0; i < info.frame_count; i++ ){
		ZoneScopedN("Frame");
		uint8_t* rgba = nullptr;
		int timestamp = 0;

		if( !WebPAnimDecoderGetNext( dec.get(), &rgba, &timestamp ) )
			return ERROR_FILE_BROKEN;

		// WebPAnimDecoder reuses an internal buffer; we must copy each frame.
		size_t frame_size = width * height * 4;
		auto pixels = std::make_unique_for_overwrite<uint8_t[]>( frame_size );
		std::memcpy( pixels.get(), rgba, frame_size );

		// Convert cumulative timestamp to per-frame delay in ms
		int delay = timestamp - last_timestamp;
		last_timestamp = timestamp;
		if( delay == 0 )
			delay = 100; // default for zero-delay frames

		cache.add_frame( imageData( width, height, std::move(pixels) ), delay );
	}

	return ERROR_NONE;
}
