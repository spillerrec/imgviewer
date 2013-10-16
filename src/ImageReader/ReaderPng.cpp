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
#include <png.h>
#include <cstring>
#include <cmath>


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
	MemStream& stream = *(MemStream*)png_get_io_ptr( png_ptr );
	if( stream.read( (char*)bytes_out, lenght ) != lenght )
		return; //Error!
}

bool ReaderPng::can_read( const char* data, unsigned lenght ) const{
	return png_sig_cmp( (unsigned char*)data, 0, std::min( 8u, lenght ) ) == 0;
}

static QImage readRGB( png_structp png_ptr, png_infop info_ptr, unsigned width, unsigned height, png_bytepp row_pointers ){
	unsigned color_type = png_get_color_type( png_ptr, info_ptr );
	bool alpha = color_type == PNG_COLOR_TYPE_GRAY_ALPHA
		||	color_type == PNG_COLOR_TYPE_RGB_ALPHA
		;
	unsigned bit_depth = png_get_bit_depth( png_ptr, info_ptr );
	
	//Set image type
	QImage::Format format = QImage::Format_ARGB32;
	if( !alpha ){
		format = QImage::Format_RGB32;
	}
	
	//TODO: support palette images
	if( color_type == PNG_COLOR_TYPE_PALETTE )
		png_set_palette_to_rgb( png_ptr );
	
	//Qt doesn't have gray-scale
	if( color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
		png_set_gray_to_rgb( png_ptr );
	
	//Use the format BGRA
	png_set_filler( png_ptr, 255, PNG_FILLER_AFTER );
	png_set_bgr( png_ptr );
	
	//Always use 8 bits
	if( bit_depth < 8 )
		png_set_packing( png_ptr );
	else
		png_set_strip_16( png_ptr );
	
	
	//Initialize image
	QImage frame( width, height, format );
	for( unsigned iy=0; iy<height; iy++ )
		row_pointers[iy] = (png_bytep)frame.scanLine( iy );
	
	png_read_image( png_ptr, row_pointers );
	
	return frame;
}

static QImage readRGBA( png_structp png_ptr, png_infop info_ptr ){
	unsigned height = png_get_image_height( png_ptr, info_ptr );
	unsigned width = png_get_image_width( png_ptr, info_ptr );
	
	png_bytep* row_pointers = new png_bytep[ height ];
	if( setjmp( png_jmpbuf( png_ptr ) ) ){
		delete[] row_pointers;
		return QImage();
	}
	
	QImage frame = readRGB( png_ptr, info_ptr, width, height, row_pointers );
	delete[] row_pointers;
	
	return frame;
}

static void readAnimated( imageCache &cache, png_structp png_ptr, png_infop info_ptr ){
	png_uint_32 width = png_get_image_width( png_ptr, info_ptr );
	png_uint_32 height = png_get_image_height( png_ptr, info_ptr );
	png_uint_32 x_offset, y_offset;
	png_uint_16 delay_num, delay_den;
	png_byte dispose_op, blend_op;
	
	png_bytep* row_pointers = new png_bytep[ height ];
	if( setjmp( png_jmpbuf( png_ptr ) ) ){
		delete[] row_pointers;
		return;
	}
	
	cache.set_info( png_get_num_frames( png_ptr, info_ptr ), true, -1 );
	
	for( unsigned i=0; i < png_get_num_frames( png_ptr, info_ptr ); ++i ){
		png_read_frame_head( png_ptr, info_ptr );
		
		if( png_get_valid( png_ptr, info_ptr, PNG_INFO_fcTL ) ){
			png_get_next_frame_fcTL( png_ptr, info_ptr
				,	&width, &height
				,	&x_offset, &y_offset
				,	&delay_num, &delay_den
				,	&dispose_op, &blend_op
				);
		}
		else{
			width = png_get_image_width( png_ptr, info_ptr );
			height = png_get_image_height( png_ptr, info_ptr );
		}
		
		
		QImage frame = readRGB( png_ptr, info_ptr, width, height, row_pointers );
		
		qDebug( "timings: %d/%d", delay_num, delay_den );
		cache.add_frame( frame, std::ceil( (double)delay_num / delay_den * 1000 ) );
	}
	
	delete[] row_pointers;
}

AReader::Error ReaderPng::read( imageCache &cache, const char* data, unsigned lenght ) const{
	if( !can_read( data, lenght ) )
		return ERROR_TYPE_UNKNOWN;
	
	// Initialize libpng
	png_structp png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
	if( !png_ptr )
		return ERROR_INITIALIZATION;
	
	png_infop info_ptr = png_create_info_struct( png_ptr );
	if( !info_ptr ){
		png_destroy_read_struct( &png_ptr, NULL, NULL );
		return ERROR_INITIALIZATION;
	}
	
	//Handle errors
	if( setjmp( png_jmpbuf( png_ptr ) ) ){
		png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
		return ERROR_FILE_BROKEN;
	}
	
	//Prepare reading
	MemStream stream = { 8, data, lenght };
	png_set_read_fn( png_ptr, &stream, read_from_mem_stream );
	png_set_sig_bytes( png_ptr, 8 ); //Ignore the first 8 bytes
	
	//Start reading
	png_read_info( png_ptr, info_ptr );
	if( png_get_valid( png_ptr, info_ptr, PNG_INFO_acTL ) )
		readAnimated( cache, png_ptr, info_ptr );
	else{
		cache.set_info( 1 );
		cache.add_frame( readRGBA( png_ptr, info_ptr ), 0 );
	}
	
	//Cleanup and return
	cache.set_fully_loaded();
	png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
	return ERROR_NONE;
}

