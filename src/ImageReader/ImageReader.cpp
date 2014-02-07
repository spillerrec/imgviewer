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
#include <QFileInfo>
#include "ReaderPng.hpp"
#include "ReaderQt.hpp"

ImageReader::ImageReader(){
	add_reader( new ReaderPng );
	add_reader( new ReaderQt );
}
ImageReader::~ImageReader(){
	for( unsigned i=0; i<readers.size(); ++i )
		delete readers[i];
}

void ImageReader::add_reader( AReader* reader ){
	readers.push_back( reader );
	for( auto ext : reader->extensions() )
		formats.insert( std::make_pair( ext.toLower(), reader ) );
}

AReader::Error ImageReader::read( imageCache &cache, QString filepath ) const{
	QString ext = QFileInfo(filepath).suffix().toLower();
	
	AReader* reader = nullptr;
	auto it = formats.find( ext );
	if( it != formats.end() )
		reader = it->second;
	else
		return AReader::ERROR_TYPE_UNKNOWN;
	
	QFile file( filepath );
	QByteArray data;
	if( file.open( QIODevice::ReadOnly ) ){
		data = file.readAll();
		file.close();
	}
	else
		return AReader::ERROR_NO_FILE;
	
	return reader->read( cache, data.constData(), data.size(), ext );
}
