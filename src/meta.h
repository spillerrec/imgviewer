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

#ifndef META_H
#define META_H

#include <QString>

#include <libexif/exif-data.h>

class meta{
	private:
		ExifData *data;
	
	public:
		meta( const uint8_t* file_data, unsigned lenght );
		~meta();
		
		int get_orientation();
		uint8_t* get_icc( unsigned &len);
		class QImage get_thumbnail();
};


#endif