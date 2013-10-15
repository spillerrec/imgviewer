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

#ifndef READER_PNG_HPP
#define READER_PNG_HPP

#include "AReader.hpp"
#include <QStringList>

class ReaderPng: public AReader{
	
	public:
		QList<QString> extensions() const{ return QStringList() << "png"; }
		virtual Error read( imageCache &cache, const char* data, unsigned lenght ) const;
		virtual bool can_read( const char* data, unsigned lenght ) const;
	
};


#endif