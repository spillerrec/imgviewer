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

AReader::Error ReaderQt::read( imageCache &cache, const char* data, unsigned lenght, QString format ) const{
	QByteArray byte_data( data, lenght );
	QBuffer buffer( &byte_data );
	QImageReader image_reader( &buffer, format.toLocal8Bit() );
	
	bool more_frames;
	if( (more_frames = image_reader.canRead()) ){
		int frame_amount = image_reader.imageCount();
		bool animate = image_reader.supportsAnimation();
		if( animate )
			cache.set_info( frame_amount, animate, image_reader.loopCount() );
		else
			cache.set_info( frame_amount, animate, -1 );
		
		//Try to get orientation info
		meta rotation( data, lenght );
		int rot = rotation.get_orientation();
		QTransform trans;
		//Rotate image
		switch( rot ){
			case 3:
			case 4:
					//Flip upside down
					trans.rotate( 180 );
				break;
			
			case 5:
			case 6:
					//90 degs to the left
					trans.rotate( 90 );
				break;
			
			case 7:
			case 8:
					//90 degs to the right
					trans.rotate( -90 );
				break;
		}
		
		//ICC
		unsigned len;
		unsigned char *data = rotation.get_icc( len );
		if( data )
			cache.set_profile( cache.get_manager()->get_profile( data, len ) );
		
		//Read every frame and delay
		for( int i=0; more_frames || frame_amount > i; i++, more_frames = image_reader.canRead() ){
			QImage frame;
			
			//NOTE: ICO files seem to return false on canRead, even though more frames are available
			if( !image_reader.read( &frame ) ){
				//TODO: ?
				break;
			}
			
			//Orient image
			if( rot != 1 ){
				//Mirror
				switch( rot ){
					case 2:
					case 4:
					case 5:
					case 7:
							frame = frame.mirrored();
						break;
				}
				
				if( rot > 2 ) //1 and 2 is not rotated, all others are
					frame = frame.transformed( trans );
			}
			
			int delay = 0;
			if( animate )
				delay = image_reader.nextImageDelay();
			else
				image_reader.jumpToImage( i+1 );
			cache.add_frame( frame, delay );
			
			/* Increase loading time for debugging
			QTime t;
			t.start();
			while( t.elapsed() < 1000 ); */
		}
		
		cache.set_fully_loaded();
		//TODO: What to do on fail?
	}
	else
		return ERROR_TYPE_UNKNOWN;
	
	return ERROR_NONE;
}

bool ReaderQt::can_read( const char* data, unsigned lenght, QString format ) const{
	QByteArray byte_data( data, lenght );
	QBuffer buffer( &byte_data );
	QImageReader image_reader( &buffer, format.toLocal8Bit() );
	return image_reader.canRead();
}

