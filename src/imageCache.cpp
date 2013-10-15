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
#include "color.h"

#include <QSize>
#include <QImage>
#include <QTransform>
#include <QImageReader>
#include <QPainter>
#include <QTime>

color* imageCache::manager = NULL;

void imageCache::init(){
	if( !manager )
		manager = new color();
	
	profile = NULL;
	frames_loaded = 0;
	memory_size = 0;
	current_status = EMPTY;
}

void imageCache::set_info( unsigned total_frames, bool is_animated, int loops ){
	current_status = INFO_READY;
	animate = is_animated;
	frame_amount = total_frames;
	loop_amount = loops;
	emit info_loaded();
}

void imageCache::add_frame( QImage frame, unsigned delay ){
	frames_loaded++;
	frame_amount = std::max( frame_amount, frames_loaded );
	frames.push_back( frame );
	frame_delays.push_back( delay );
	current_status = FRAMES_READY;
	emit frame_loaded( frames_loaded-1 );
}

void imageCache::set_fully_loaded(){
	current_status = LOADED;
}

void imageCache::read( QString filename ){
	//Make sure this cache is unloaded!
	if( current_status != EMPTY )
		return;
	
	QImageReader image_reader( filename );
	
	bool more_frames;
	if( (more_frames = image_reader.canRead()) ){
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
		unsigned len;
		unsigned char *data = rotation.get_icc( len );
		if( data )
			profile = manager->get_profile( data, len );
		
		//Signal that status have changed
		current_status = INFO_READY;
		emit info_loaded();
		
		if( frame_amount > 0 ){
			//We know amount already
			frames.reserve( frame_amount );
			frame_delays.reserve( frame_amount );
		}
		
		//Read every frame and delay
		for( int i=0; more_frames || frame_amount > i; i++, more_frames = image_reader.canRead() ){
			//NOTE: ICO files seem to return false on canRead, even though more frames are available
			frames.push_back( QImage() );
			if( !image_reader.read( &(frames[i]) ) ){
				frames.pop_back();
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
							frames[i] = frames[i].mirrored();
						break;
				}
				
				if( rot > 2 ) //1 and 2 is not rotated, all others are
					frames[i] = frames[i].transformed( trans );
			}
			
			if( animate )
				frame_delays.push_back( image_reader.nextImageDelay() );
			else
				image_reader.jumpToImage( i+1 );
			
			memory_size += frames[i].byteCount();
			/* Increase loading time for debugging
			QTime t;
			t.start();
			while( t.elapsed() < 1000 ); */
			
			current_status = FRAMES_READY;
			frames_loaded++;
			if( frame_amount < i )
				frame_amount = i;
			emit frame_loaded( i );
		}
		
		if( current_status == FRAMES_READY ) //All reads where successful
			current_status = LOADED;
		//TODO: What to do on fail?
	}
}



