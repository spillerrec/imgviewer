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

#include <QSize>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QTime>


void imageCache::mark_as_invalid(){
	frames = NULL;
	frame_delays = NULL;
	frames_loaded = -1;
	memory_size = 0;
}


imageCache::imageCache(){
	mark_as_invalid();
	frames_loaded = 0;
}

imageCache::imageCache( QString filename ){
	frames_loaded = 0;
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
	if( frames_loaded != 0 )
		return;
	
	QImageReader image_reader( filename );
	
	if( image_reader.canRead() ){
		frame_amount = image_reader.imageCount();
		animate = image_reader.supportsAnimation();
		if( animate )
			loop_amount = image_reader.loopCount();
		else
			loop_amount = -1;
		
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
				image_reader.read( frames[i] );
				
				if( animate )
					frame_delays[i] = image_reader.nextImageDelay();
				else
					image_reader.jumpToImage( i+1 );
				
				memory_size += frames[i]->byteCount();
				/* QTime t;
				t.start();
				while( t.elapsed() < 1000 ); */
				
				frames_loaded++;
				emit frame_loaded( i );
			}
		}
	}
	else{
		//If canRead failed, some error must have happened!
		mark_as_invalid();
	}
}



