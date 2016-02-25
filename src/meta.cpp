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

#include "meta.h"
#include "ImageReader/ReaderJpeg.hpp"
#include "viewer/imageCache.h"

#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>

#include <QFile>
#include <QByteArray>

meta::meta( const uint8_t* file_data, unsigned length ){
	data = exif_data_new_from_data( file_data, length );
}
meta::~meta(){
	if( data )
		exif_data_unref( data );
}

int meta::get_orientation(){
	if( data ){
		ExifEntry *orientation = exif_content_get_entry( data->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION ); //TODO: [0]!
		if( orientation )
			return exif_get_short( orientation->data, exif_data_get_byte_order( data ) );
	}
	
	return 1;
}

unsigned char* meta::get_icc( unsigned &len ){
	if( data ){
		ExifEntry *icc = exif_content_get_entry( data->ifd[EXIF_IFD_0], EXIF_TAG_INTER_COLOR_PROFILE );
		if( icc ){
			len = icc->size;
			return icc->data;
		}
	}
	
	return 0;
}

QImage meta::get_thumbnail(){
	if( data && data->data ){
		imageCache image;
		ReaderJpeg().read( image, reinterpret_cast<const char*>(data->data), data->size, "jpg" );
		if( image.frame_count() >= 1 )
			return image.frame( 0 );
	}
	return {};
}



