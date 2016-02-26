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

#include "viewer/imageCache.h"
#include <QMutexLocker>
#include <QFileInfo>

#include "ImageReader/ImageReader.hpp"

void imageLoader::run(){
	mutex.lock();
	while( image ){
		//Load data
		QString filepath = file;
		auto loading = std::move( image );
		
		mutex.unlock();
		emit image_fetched();
		
		ImageReader reader; //TODO: initialize in constructor?
		reader.read( *loading, filepath );
		emit image_loaded( loading.get() );
		
		mutex.lock();	//Make sure to lock it again, as wee need it at the while loop check
	}
	mutex.unlock(); //Make sure to lock it when the while loop exits
}

/* Attempts to start loading an image. Returns true on success, false on failure. */
std::shared_ptr<imageCache> imageLoader::load_image( QString filepath ){
	mutex.lock();
	if( image ){
		//An image is already in the queue, can't add this one
		mutex.unlock();
		return {};
	}
	
	image = std::make_shared<imageCache>();
	file = filepath;
	
	mutex.unlock();
	
	if( !isRunning() )
		start();
	
	return image;
}



