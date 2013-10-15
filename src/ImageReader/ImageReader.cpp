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

#include "ImageReader.hpp"

#include <QFile>
#include "ReaderPng.hpp"

ImageReader::ImageReader(){
	readers.push_back( new ReaderPng );
}
ImageReader::~ImageReader(){
	for( unsigned i=0; i<readers.size(); ++i )
		delete readers[i];
}

AReader::Error ImageReader::read( imageCache &cache, QString filepath ) const{
	QFile file( filepath );
	QByteArray data;
	if( file.open( QIODevice::ReadOnly ) ){
		data = file.readAll();
		file.close();
	}
	
	return readers[0]->read( cache, data.constData(), data.size() );
}
