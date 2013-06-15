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

#include "imageCache.h"
#include "meta.h"

#include <QSize>
#include <QImage>
#include <QTransform>
#include <QImageReader>
#include <QPainter>
#include <QTime>

color* imageCache::manager = NULL;

void imageCache::init(){
	if( !manager )
		manager = new color( "" );
	
	frames = NULL;
	frame_delays = NULL;
	frames_loaded = 0;
	memory_size = 0;
	current_status = EMPTY;
}


imageCache::imageCache(){
	init();
}

imageCache::imageCache( QString filename ){
	init();
	read( filename );
}


imageCache::imageCache( QImage *preloaded ){
	frame_amount = 1;
	frames = new QImage*[ frame_amount ];
	frames[0] = preloaded;
	frames_loaded = 1;
	
	if( preloaded )
		memory_size = preloaded->byteCount();
	else
		memory_size = 0;
	
	animate = false;
	frame_delays = new int[ frame_amount ];
	frame_delays[0] = -1;
	loop_amount = -1;
	
	current_status = LOADED;
	emit frame_loaded( 0 );
}


imageCache::~imageCache(){
	if( frames ){
		for( int i=0; i<frame_amount; i++ )
			delete frames[i];
		delete[] frames;
	}
	
	if( frame_delays )
		delete[] frame_delays;
}


void imageCache::read( QString filename ){
	//Make sure this cache is unloaded!
	if( current_status != EMPTY )
		return;
	
	QImageReader image_reader( filename );
	
	if( image_reader.canRead() ){
		frame_amount = image_reader.imageCount();
		animate = image_reader.supportsAnimation();
		if( animate )
			loop_amount = image_reader.loopCount();
		else
			loop_amount = -1;
		
		//Try to get orientation info
		meta rotation( filename );
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
		cmsHTRANSFORM transform = 0;
		unsigned len;
		unsigned char *data = rotation.get_icc( len );
		if( data ){
			transform = manager->get_transform( data, len );
		//	qDebug( "Tried to get transform: %d", (int) transform );
		}
		
		//Signal that status have changed
		current_status = INFO_READY;
		emit info_loaded();
		
		if( frame_amount > 0 ){	//Make sure frames are readable
			//Init arrays
			frames = new QImage*[ frame_amount ];
			if( animate )
				frame_delays = new int[ frame_amount ];
			else
				frame_delays = NULL;
			
			//Read every frame and delay
			for( int i=0; i<frame_amount; i++ ){
				frames[i] = new QImage();
				if( !image_reader.read( frames[i] ) ){
					current_status = INVALID;
					break;
				}
				
				manager->transform( frames[i], transform );
				
				//Orient image
				if( rot != 1 ){
					//Mirror
					switch( rot ){
						case 2:
						case 4:
						case 5:
						case 7:
								*(frames[i]) = frames[i]->mirrored();
							break;
					}
					
					if( rot > 2 ) //1 and 2 is not rotated, all others are
						*(frames[i]) = frames[i]->transformed( trans );
				}
				
				if( animate )
					frame_delays[i] = image_reader.nextImageDelay();
				else
					image_reader.jumpToImage( i+1 );
				
				memory_size += frames[i]->byteCount();
				/* Increase loading time for debugging
				QTime t;
				t.start();
				while( t.elapsed() < 1000 ); */
				
				current_status = FRAMES_READY;
				frames_loaded++;
				emit frame_loaded( i );
			}
			
			if( current_status == FRAMES_READY ) //All reads where successful
				current_status = LOADED;
			//TODO: What to do on fail?
		}
	}
	else{
		//If canRead failed, some error must have happened!
		current_status = INVALID;
	}
}



