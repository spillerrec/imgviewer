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

#include <QImage>
#include <QPainter>
#include <png.h>
#include <cstring>
#include <cmath>
#include <vector>


struct MemStream{
	unsigned pos;
	const char* data;
	unsigned lenght;
	
	unsigned remaining() const{ return lenght-pos-1; }
	unsigned read( char* out, unsigned amount ){
		if( amount > remaining() )
			amount = remaining();
		std::memcpy( out, data+pos, amount );
		pos += amount;
		return amount;
	}
};
static void read_from_mem_stream( png_structp png_ptr, png_bytep bytes_out, png_size_t lenght ){
	MemStream& stream = *reinterpret_cast<MemStream*>( png_get_io_ptr( png_ptr ) );
	if( stream.read( (char*)bytes_out, lenght ) != lenght )
		return; //Error!
}

bool ReaderPng::can_read( const char* data, unsigned lenght, QString ) const{
	return png_sig_cmp( (unsigned char*)data, 0, std::min( 8u, lenght ) ) == 0;
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

static void readRgb( PngInfo& info, unsigned width, unsigned height, bool update ){
	//Apply transparency information
	bool alpha = info.isGrayAlpha() || info.isRgbAlpha();
	if( png_get_valid( info.png, info.info, PNG_INFO_tRNS ) ){
		png_set_tRNS_to_alpha( info.png );
		alpha = true;
	}
	
	//Convert Grayscale
	if( info.isGray() || info.isGrayAlpha() )
		png_set_gray_to_rgb( info.png );
	
	//Use the format BGRA
	info.force8bit();
	png_set_filler( info.png, 255, PNG_FILLER_AFTER );
	png_set_bgr( info.png );
	
	//Initialize image
	auto format = alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32;
	info.read( width, height, format, update );
}

static void readGray( PngInfo& info, unsigned width, unsigned height, bool update ){
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

static void readPaletted( PngInfo& info, unsigned width, unsigned height, bool update ){
	Q_ASSERT( info.isPalette() );
	
	//TODO: support paletted images
	png_set_palette_to_rgb( info.png );
	readRgb( info, width, height, update );
}

static void readImage( PngInfo& info, unsigned width, unsigned height, bool update=true ){
	if( info.isPalette() )
		readPaletted( info, width, height, update );
	else if( info.isGray() )
		readGray( info, width, height, update );
	else
		readRgb( info, width, height, update );
}

static void readAnimated( imageCache &cache, PngInfo& png ){
	auto width  = png.width();
	auto height = png.height();
	png_uint_32 x_offset=0, y_offset=0;
	png_uint_16 delay_num, delay_den;
	png_byte dispose_op = PNG_DISPOSE_OP_NONE, blend_op = PNG_BLEND_OP_SOURCE;
	QImage canvas( width, height, QImage::Format_ARGB32 );
	canvas.fill( qRgba( 0,0,0,0 ) );
	
	if( setjmp( png_jmpbuf( png.png ) ) )
		return;
	
	unsigned repeats = png_get_num_plays( png.png, png.info );
	unsigned frames = png_get_num_frames( png.png, png.info );
	
	//NOTE: We discard the frame if it is not a part of the animation
	if( png_get_first_frame_is_hidden( png.png, png.info ) ){
		readImage( png, width, height );
		--frames; //libpng appears to tell the total amount of images
	}
	
	cache.set_info( frames, true, repeats>0 ? repeats-1 : -1 );
	
	for( unsigned i=0; i < frames; ++i ){
		png_read_frame_head( png.png, png.info );
		
		if( png_get_valid( png.png, png.info, PNG_INFO_fcTL ) ){
			png_get_next_frame_fcTL( png.png, png.info
				,	&width, &height
				,	&x_offset, &y_offset
				,	&delay_num, &delay_den
				,	&dispose_op, &blend_op
				);
		}
		else{
			width  = png.width();
			height = png.height();
		}
		
		
		readImage( png, width, height, i==0 );
		
		//Compose
		QImage output = canvas;
		QPainter painter( &output );
		
		if( blend_op == PNG_BLEND_OP_SOURCE )
			painter.setCompositionMode( QPainter::CompositionMode_Source );
		painter.drawImage( x_offset, y_offset, png.frame );
		
		delay_den = delay_den==0 ? 100 : delay_den;
		unsigned delay = std::ceil( (double)delay_num / delay_den * 1000 );
		if( delay == 0 )
			delay = 1; //Fastest speed we support
		cache.add_frame( output, delay );
		
		//Dispose
		if( dispose_op == PNG_DISPOSE_OP_NONE )
			canvas = output;
		else if( dispose_op == PNG_DISPOSE_OP_BACKGROUND ){
			QPainter canvas_painter( &canvas );
			canvas_painter.setCompositionMode( QPainter::CompositionMode_Source );
			canvas_painter.fillRect( x_offset, y_offset, width, height, QColor(0,0,0,0) );
		}
		//else dispose previous, which will discard the output
	}
}

AReader::Error ReaderPng::read( imageCache &cache, const char* data, unsigned lenght, QString format ) const{
	if( !can_read( data, lenght, format ) )
		return ERROR_TYPE_UNKNOWN;
	
	// Initialize libpng
	PngInfo png;
	if( !png.isValid() )
		return ERROR_INITIALIZATION;
	
	//Handle errors
	if( setjmp( png_jmpbuf( png.png ) ) )
		return ERROR_FILE_BROKEN;
	
	//Prepare reading
	MemStream stream = { 8, data, lenght };
	png_set_read_fn( png.png, &stream, read_from_mem_stream );
	png_set_sig_bytes( png.png, 8 ); //Ignore the first 8 bytes
	
	//Start reading
	png_read_info( png.png, png.info );
	if( png_get_valid( png.png, png.info, PNG_INFO_acTL ) )
		readAnimated( cache, png );
	else{
		cache.set_info( 1 );
		readImage( png, png.width(), png.height() );
		cache.add_frame( png.frame, 0 );
	}
	
	//Cleanup and return
	cache.set_fully_loaded();
	return ERROR_NONE;
}

