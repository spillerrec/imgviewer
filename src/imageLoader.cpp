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

#include "imageLoader.h"

#include "imageCache.h"
#include <QMutexLocker>
#include <QFileInfo>

#include "ImageReader/ImageReader.hpp"

imageLoader::imageLoader(){
	image = NULL;
	loading = NULL;
	delete_after_load = false;
}

void imageLoader::run(){
	mutex.lock();
	while( image ){
		//Load data
		imageCache *img = loading = image;
		QString filepath = file;
		image = NULL;
		
		mutex.unlock();
		emit image_fetched();
		
		if( QFileInfo(filepath).suffix().toLower() == "png" ){
			ImageReader reader;
			reader.read( *img, filepath );
		}
		else
			img->read( filepath );
		
		mutex.lock();	//Make sure to lock it again, as wee need it at the while loop check
		loading = NULL;
		if( delete_after_load ){
			delete img;
			delete_after_load = false;
		}
		else
			emit image_loaded( img );
	}
	mutex.unlock(); //Make sure to lock it when the while loop exits
}

/* Attempts to start loading an image. Returns true on success, false on failure. */
bool imageLoader::load_image( imageCache *img, QString filepath ){
	mutex.lock();
	if( image ){
		//An image is already in the queue, can't add this one
		mutex.unlock();
		return false;
	}
	
	image = img;
	file = filepath;
	
	mutex.unlock();
	
	if( !isRunning() )
		start();
	
	return true;
}


/*	Deletes [img] safely. If [img] is about to be loaded, prevent this
	and delete [img].	If [img] is being loaded, make sure it is deleted
	as soon as this is completed. Otherwise [img] is just deleted.
	[img] will be NULL when this function returns, however the actual
	delete might not have been performed yet.
*/
void imageLoader::delete_image( imageCache *img ){
	if( !img )
		return;
	
	QMutexLocker locker( &mutex );
	
	if( img == image ){
		delete image;
		image = NULL;
		img = NULL;
		return;
	}
	
	if( img == loading ){
		delete_after_load = true;
		img = NULL;
		return;
	}
	
	//img isn't affected by this loader, delete
	delete img;
	img = NULL;
}



