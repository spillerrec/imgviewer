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

#include "ReaderJpeg.hpp"

#include "jpeglib.h"

#include <QImage>
#include <memory>

bool ReaderJpeg::can_read( const char* data, unsigned lenght, QString ) const{
	return true;//png_sig_cmp( (unsigned char*)data, 0, std::min( 8u, lenght ) ) == 0;
}

class JpegDecompress{
	public: //TODO:
		jpeg_decompress_struct cinfo;
		jpeg_error_mgr jerr;
		
	public:
		JpegDecompress( const char* data, unsigned lenght ) {
			jpeg_create_decompress( &cinfo );
			jpeg_mem_src( &cinfo, (unsigned char*)data, lenght );
			cinfo.err = jpeg_std_error( &jerr );
		}
		~JpegDecompress(){ jpeg_destroy_decompress( &cinfo ); }
		
		void readHeader( bool what=true )
			{ jpeg_read_header( &cinfo, what ); }
		
		unsigned bytesPerLine() const
			{ return cinfo.output_width * cinfo.output_components; }
};


AReader::Error ReaderJpeg::read( imageCache &cache, const char* data, unsigned lenght, QString format ) const{
	//jpeg_save_markers(cinfo, marker_code, length_limit)
	//JPEG_APP0+n
	//If you want to save all the data, set length_limit to 0xFFFF; that is enough since marker lengths are only 16 bits.
	//jpeg_save_markers(cinfo, JPEG_APP0+1, 0xFFFF)
	
	if( !can_read( data, lenght, format ) )
		return ERROR_TYPE_UNKNOWN;
	
	
	JpegDecompress jpeg( data, lenght );
	jpeg.readHeader();
	
	
	jpeg_start_decompress( &jpeg.cinfo );
	
	//TODO: Check details of jpeg image?
	cache.set_info( 1 );
	//TODO: on failure: return ERROR_INITIALIZATION;
	QImage frame( jpeg.cinfo.output_width, jpeg.cinfo.output_height, QImage::Format_RGB32 );
	
	auto buffer = std::make_unique<JSAMPLE[]>( jpeg.bytesPerLine() );
	JSAMPLE* arr[1] = { buffer.get() };
	while( jpeg.cinfo.output_scanline < jpeg.cinfo.output_height ){
		auto out = (QRgb*)frame.scanLine( jpeg.cinfo.output_scanline );
		jpeg_read_scanlines( &jpeg.cinfo, arr, 1 );
		
		for( unsigned ix=0; ix<frame.width(); ix++ )
			out[ix] = qRgb( buffer[ix*3+0], buffer[ix*3+1], buffer[ix*3+2] );
	}
	jpeg_finish_decompress( &jpeg.cinfo );
	
	//TODO: on error: return ERROR_FILE_BROKEN;
	
	cache.add_frame( frame, 0 );
	
	//Cleanup and return
	cache.set_fully_loaded();
	return ERROR_NONE;
}

