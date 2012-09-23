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

#ifndef IMAGECACHE_H
#define IMAGECACHE_H

#include <QObject>
#include "color.h"

class QImage;

class imageCache: public QObject{
	Q_OBJECT
	
	private:
		void init();
		static color* manager;
		
	private:
	//Variables containing info about the image(s)
		int frame_amount;
		QImage **frames;
		int frames_loaded;
		
		bool animate;
		int *frame_delays;
		int loop_amount;	//Amount of times the loop should continue looping
		
		long memory_size;
		
	//Info about loading
	public:
		enum status{
			EMPTY,	//Nothing loaded
			INVALID,	//Attempted loading, but failed
			INFO_READY,	//Info is valid
			FRAMES_READY,	//Some frames have been loaded
			LOADED	//All frames have been loaded
		};
	private:
		status current_status;
	public:
		status get_status(){ return current_status; } //Current status
		int loaded(){ return frames_loaded; }	//Amount of currently loaded frames
		
	public:
		explicit imageCache();
		explicit imageCache( QString filename );
		explicit imageCache( QImage *preloaded );
		~imageCache();
		
		void read( QString filename );
		long get_memory_size(){ return memory_size; }	//Notice, this is a rough number, not accurate!
		
		//Animation info
		bool is_animated(){ return animate; }
		int loop_count(){ return loop_amount; }
		
		//Frame info
		int frame_count(){ return frame_amount; }
		QImage* frame( unsigned int idx ){ return frames[ idx ]; }
		int frame_delay( unsigned int idx ){ return frame_delays[ idx ]; } //How long a frame should be shown
	
	signals:
		void info_loaded();
		void frame_loaded( unsigned int idx );
};


#endif