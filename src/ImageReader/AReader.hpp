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

#ifndef A_READER_HPP
#define A_READER_HPP

#include "../imageCache.h"

#include <QString>
#include <QList>


class AReader{
	public:
		enum Error{
			ERROR_NONE,
			ERROR_NO_FILE,
			ERROR_TYPE_UNKNOWN,	//The file format is not supported
			ERROR_FILE_BROKEN,	//Reading initially seemed to be fine, but contained errors
			ERROR_INITIALIZATION,
			
			ERROR_CUSTOM,	//Further details in QString list?
			ERROR_UNKNOWN
		};
		
		virtual ~AReader(){ }
		
		
		virtual QList<QString> extensions() const = 0; // List of extensions files of this type can have
		
		
		virtual bool can_read( const char* data, unsigned lenght ) const = 0; //Test if this file can be read
		
		//virtual bool read( imageCache &cache, QString filepath ){ return false; }
		virtual Error read( imageCache &cache, const char* data, unsigned lenght ) const = 0;
		
};


#endif