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

#include "ReaderQt.hpp"
#include "../meta.h"
#include "../viewer/colorManager.h"

#include <QImageReader>
#include <QBuffer>

QList<QString> ReaderQt::extensions() const{
	QList<QString> exts;
	for( auto extension : QImageReader::supportedImageFormats() )
		exts << extension;
	return exts;
}

static QByteArray fromData( const uint8_t* data, unsigned length )
	{ return QByteArray( reinterpret_cast<const char*>( data ), length ); }

AReader::Error ReaderQt::read( imageCache &cache, const uint8_t* data, unsigned length, QString format ) const{
	QByteArray byte_data = fromData( data, length );
	QBuffer buffer( &byte_data );
	QImageReader image_reader( &buffer, format.toLocal8Bit() );
	
	if( image_reader.canRead() ){
		//Read first image
		QImage frame;
		if( !image_reader.read( &frame ) )
			return ERROR_TYPE_UNKNOWN;
		
		int frame_amount = image_reader.imageCount();
		auto isAnim = image_reader.supportsAnimation();
		cache.set_info( frame_amount, isAnim, isAnim ? image_reader.loopCount() : -1 );
		
		int current_frame = 1;
		do{
			cache.add_frame( frame, image_reader.nextImageDelay() );
			if( frame_amount > 0 && current_frame >= frame_amount )
				break;
			current_frame++;
			image_reader.jumpToNextImage();
		}
		while( image_reader.read( &frame ) );
		
		cache.set_fully_loaded();
		//TODO: What to do on fail?
	}
	else
		return ERROR_TYPE_UNKNOWN;
	
	return ERROR_NONE;
}

bool ReaderQt::can_read( const uint8_t* data, unsigned length, QString format ) const{
	QByteArray byte_data = fromData( data, length );
	QBuffer buffer( &byte_data );
	QImageReader image_reader( &buffer, format.toLocal8Bit() );
	return image_reader.canRead();
}

