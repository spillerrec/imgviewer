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

#include "ReaderPng.hpp"
#include "AnimFrameCombinerWorker.hpp"
#include "../viewer/colorManager.h"

#include <tracy/Tracy.hpp>

#include <png.h>
#include <cstring>
#include <cmath>
#include <vector>


struct MemStream{
	unsigned pos;
	const uint8_t* data;
	unsigned length;
	
	unsigned remaining() const{ return length-pos-1; }
	unsigned read( uint8_t* out, unsigned amount ){
		if( amount > remaining() )
			amount = remaining();
		std::memcpy( out, data+pos, amount );
		pos += amount;
		return amount;
	}
};
static void read_from_mem_stream( png_structp png_ptr, png_bytep bytes_out, png_size_t length ){
	MemStream& stream = *static_cast<MemStream*>( png_get_io_ptr( png_ptr ) );
	if( stream.read( bytes_out, length ) != length )
		return; //Error!
}

bool ReaderPng::can_read( const uint8_t* data, unsigned length, QString ) const{
	return png_sig_cmp( data, 0, std::min( 8u, length ) ) == 0;
}


class PngInfo{
	public: //NOTE: for now...
		png_structp png{ nullptr };
		png_infop  info{ nullptr };
		QImage frame;
		std::vector<png_bytep> row_pointers;
		
	public:
		PngInfo(){
			png = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
			if( png )
				info = png_create_info_struct( png );
		}
		PngInfo( const PngInfo& copy ) = delete;
		~PngInfo(){ png_destroy_read_struct( &png, &info, NULL ); }
		
		bool isValid() const{ return png && info; }
		
	public:
		void read( unsigned w, unsigned h, QImage::Format f, bool update ){
			frame = QImage( w, h, f );
			row_pointers.clear();
			row_pointers.reserve( h );
			for( unsigned i=0; i<h; i++ )
				row_pointers.push_back( (png_bytep)frame.scanLine( i ) );
			
			if( update )
				png_read_update_info( png, info );
			static_assert( sizeof(png_byte) == sizeof(uint8_t), "png_byte must be 8bit" );
			png_read_image( png, row_pointers.data() );
		}
		
	public:
		auto width(){  return png_get_image_width(  png, info ); }
		auto height(){ return png_get_image_height( png, info ); }
		
		auto colorType(){ return png_get_color_type( png, info ); }
		auto bitDepth(){  return png_get_bit_depth(  png, info ); }
		
		bool isPalette(){   return colorType() == PNG_COLOR_TYPE_PALETTE;    }
		bool isGray(){      return colorType() == PNG_COLOR_TYPE_GRAY;       }
		bool isGrayAlpha(){ return colorType() == PNG_COLOR_TYPE_GRAY_ALPHA; }
		bool isRgb(){       return colorType() == PNG_COLOR_TYPE_RGB;        }
		bool isRgbAlpha(){  return colorType() == PNG_COLOR_TYPE_RGB_ALPHA;  }
		
		void force8bit(){
			if( bitDepth() < 8 )
				png_set_packing( png );
			else
				png_set_strip_16( png );
		}
};

// Configure libpng to output RGBA8888 (with alpha if present)
static void configurePngForRgba( PngInfo& info, bool bgr ){
	bool alpha = info.isGrayAlpha() || info.isRgbAlpha();

	if ( info.isPalette() )
		png_set_palette_to_rgb( info.png ); // Expand palette to 24-bit RGB
	
	if( png_get_valid( info.png, info.info, PNG_INFO_tRNS ) ){
		png_set_tRNS_to_alpha( info.png );
		alpha = true;
	}
	
	//Convert Grayscale to RGB
	if( info.isGray() || info.isGrayAlpha() )
		png_set_gray_to_rgb( info.png );
	
	info.force8bit();
	
	//Ensure alpha channel exists (fill with opaque if missing)
	if( !alpha )
		png_set_filler( info.png, 255, PNG_FILLER_AFTER );
	
	//Convert BGR to RGB order
	if ( bgr )
		png_set_bgr( info.png );
	
	png_read_update_info( info.png, info.info );
}

static void readRgb( PngInfo& info, unsigned width, unsigned height, bool update ){
	ZoneScoped;
	configurePngForRgba( info, true );
	info.read( width, height, QImage::Format_ARGB32, update );
}

#if QT_VERSION >= 0x050500
static void readGray( PngInfo& info, unsigned width, unsigned height, bool update ){
	ZoneScoped;
	Q_ASSERT( info.isGray() );
	
	if( png_get_valid( info.png, info.info, PNG_INFO_tRNS ) ){
		//TODO: make it paletted!
		readRgb( info, width, height, update );
	}
	else{
		info.force8bit();
		info.read( width, height, QImage::Format_Grayscale8, update );
	}
}
#endif

static void readPaletted( PngInfo& info, unsigned width, unsigned height, bool update ){
	ZoneScoped;
	Q_ASSERT( info.isPalette() );
	
	//TODO: support paletted images
	png_set_palette_to_rgb( info.png );
	readRgb( info, width, height, update );
}

// Read PNG directly into raw RGBA8888 pixels (no QImage)
// Assumes configurePngForRgba has already been called
static std::unique_ptr<uint8_t[]> readPngRawRgba( PngInfo& info, unsigned width, unsigned height ){
	ZoneScoped;
	const size_t row_bytes = width * 4;
	std::vector<png_bytep> row_pointers(height);
	row_pointers.reserve(height);
	
	//Allocate pixel buffer and set up row pointers
	auto pixels = std::make_unique<uint8_t[]>(row_bytes * height);
	for( unsigned i=0; i<height; ++i )
		row_pointers[i] = pixels.get() + (i * row_bytes);
	
	static_assert( sizeof(png_byte) == sizeof(uint8_t), "png_byte must be 8bit" );
	png_read_image( info.png, row_pointers.data() );
	return pixels;
}

static void readImage( PngInfo& info, unsigned width, unsigned height, bool update=true ){
	ZoneScoped;
	if( info.isPalette() )
		readPaletted( info, width, height, update );
#if QT_VERSION >= 0x050500
	else if( info.isGray() )
		readGray( info, width, height, update );
#endif
	else
		readRgb( info, width, height, update );
}

static CicpInfo readCicp( PngInfo& png ){
	CicpInfo cicp;
#ifdef PNG_cICP_SUPPORTED
	png_byte primaries, transfer, matrix, range;
	if( png_get_cICP( png.png, png.info, &primaries, &transfer, &matrix, &range ) & PNG_INFO_cICP ){
		cicp.primaries = static_cast<ColorPrimaries>( primaries );
		cicp.transfer  = static_cast<TransferFunction>( transfer );
		cicp.matrix    = static_cast<MatrixCoefficients>( matrix );
		cicp.full_range = range != 0;
	}
#endif
	return cicp;
}

#ifdef PNG_APNG_SUPPORTED
static void readAnimated( imageCache &cache, PngInfo& png ){
	const unsigned canvas_width  = png.width();
	const unsigned canvas_height = png.height();
	png_uint_32 x_offset=0, y_offset=0;
	png_uint_16 delay_num=0, delay_den=0;
	png_byte dispose_op = PNG_DISPOSE_OP_NONE, blend_op = PNG_BLEND_OP_SOURCE;
	
	//Configure transforms once before the frame loop
	configurePngForRgba( png, false );
	
	if( setjmp( png_jmpbuf( png.png ) ) )
		return;
	
	unsigned repeats = png_get_num_plays( png.png, png.info );
	unsigned frames = png_get_num_frames( png.png, png.info );
	
	//NOTE: We discard the frame if it is not a part of the animation
	if( png_get_first_frame_is_hidden( png.png, png.info ) ){
		readImage( png, canvas_width, canvas_height );
		--frames; //libpng appears to tell the total amount of images
	}
	
	cache.set_info( frames, true, repeats>0 ? repeats-1 : -1 );
	
	//Background worker handles combining so decoding isn't blocked
	AnimFrameCombinerWorker combiner_worker( cache, (int)canvas_width, (int)canvas_height );
	
	for( unsigned i=0; i < frames; ++i ){
		png_read_frame_head( png.png, png.info );
		
		unsigned frame_width  = canvas_width;
		unsigned frame_height = canvas_height;
		
		if( png_get_valid( png.png, png.info, PNG_INFO_fcTL ) ){
			png_get_next_frame_fcTL( png.png, png.info
				,	&frame_width, &frame_height
				,	&x_offset, &y_offset
				,	&delay_num, &delay_den
				,	&dispose_op, &blend_op
				);
		}
		
		//Read frame directly into raw RGBA8 pixels (no QImage)
		//libpng already called png_read_update_info after png_read_frame_head
		auto frame_pixels = readPngRawRgba( png, frame_width, frame_height );
		
		//Calculate delay
		delay_den = delay_den==0 ? 100 : delay_den;
		unsigned delay = std::ceil( (double)delay_num / delay_den * 1000 );
		if( delay == 0 )
			delay = 1; //Fastest speed we support
		
		//Submit to background worker (non-blocking)
		auto blend_mode = blend_op == PNG_BLEND_OP_SOURCE ? BlendMode::REPLACE : BlendMode::OVERLAY;
		auto dispose_mode = [=](){ switch( dispose_op ){
			case PNG_DISPOSE_OP_NONE:       return DisposeMode::NONE;
			case PNG_DISPOSE_OP_BACKGROUND: return DisposeMode::BACKGROUND;
			case PNG_DISPOSE_OP_PREVIOUS:   return DisposeMode::REVERT;
			default: return DisposeMode::NONE; //TODO: add error
			} }();
		combiner_worker.submit( PendingFrame{
			std::move(frame_pixels),
			(int)frame_width, (int)frame_height,
			(int)x_offset, (int)y_offset,
			blend_mode, dispose_mode,
			delay
		} );
	}
	
	//Wait for all frames to be combined before marking fully loaded
	combiner_worker.wait_for_completion();
}
#endif

AReader::Error ReaderPng::read( imageCache &cache, const uint8_t* data, unsigned length, QString format ) const{
	ZoneScoped;
	if( !can_read( data, length, format ) )
		return ERROR_TYPE_UNKNOWN;
	
	// Initialize libpng
	PngInfo png;
	if( !png.isValid() )
		return ERROR_INITIALIZATION;
	
	//Handle errors
	if( setjmp( png_jmpbuf( png.png ) ) )
		return ERROR_FILE_BROKEN;
	
	//Prepare reading
	MemStream stream = { 8, data, length };
	png_set_read_fn( png.png, &stream, read_from_mem_stream );
	png_set_sig_bytes( png.png, 8 ); //Ignore the first 8 bytes
	
	//Start reading
	png_read_info( png.png, png.info );

	//Read cICP chunk if present
	CicpInfo cicp = readCicp( png );
#ifdef PNG_APNG_SUPPORTED
	if( png_get_valid( png.png, png.info, PNG_INFO_acTL ) )
		readAnimated( cache, png );
	else
#endif
	{
		cache.set_info( 1 );
		unsigned w = png.width();
		unsigned h = png.height();
		
		//For static images: read directly into raw RGBA8888 pixels, no QImage needed
		configurePngForRgba( png, false );
		auto pixels = readPngRawRgba( png, w, h );
		cache.add_frame( imageData( (int)w, (int)h, std::move(pixels) ), 0 );
	}

	//Apply cICP if present
	if( cicp.is_valid() ){
		auto& profile = cache.get_profile();
		profile.cicp.primaries  = cicp.primaries;
		profile.cicp.transfer   = cicp.transfer;
		profile.cicp.matrix     = cicp.matrix;
		profile.cicp.full_range = cicp.full_range;
	}
	
	//Cleanup and return
	cache.set_fully_loaded();
	return ERROR_NONE;
}

