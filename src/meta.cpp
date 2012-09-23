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

#include <libexif/exif-loader.h>
#include <libexif/exif-utils.h>


meta::meta( QString filepath ){
	ExifLoader *l = exif_loader_new();
	exif_loader_write_file( l, filepath.toLocal8Bit().data() );
	data = exif_loader_get_data(l);
	exif_loader_unref(l);
	l = 0;
}
meta::~meta(){
	if( data )
		exif_data_unref( data );
}

int meta::get_orientation(){
	if( data ){
		ExifEntry *orientation = exif_content_get_entry( data->ifd[0], EXIF_TAG_ORIENTATION ); //TODO: [0]!
		ExifShort value = 1;
		if( orientation ){
			value = exif_get_short( orientation->data, exif_data_get_byte_order( data ) );
			//exif_entry_unref( orientation );
		}
		
		return value;
	}
	
	return 1;
}

unsigned char* meta::get_icc( unsigned &len ){
	if( data ){
		ExifEntry *icc = exif_content_get_entry( data->ifd[0], EXIF_TAG_INTER_COLOR_PROFILE ); //TODO: [0]!
		len = icc->size;
		return icc->data;
	}
	
	return 0;
}



