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

#ifndef IMAGELOADER_H
#define IMAGELOADER_H

/*
	This class loads a imageCache in a separate thread.
	It can only load one at a time but it can store a single imageCache
	until it is ready to start loading it. The signal image_fetched()
	is emitted when it is possible to store a new one.
	The signal image_loaded() is emitted when it is done loading an
	imageCache.
	
	Use the function bool load_image( imageCache*, QString ) to attempt
	to add an imageCache for loading, returns true on success, false on
	falure.
	
	Use delete_image( imageCache* ) to delete imageCache* which have been
	loaded with the same object safely.
*/

#include <QThread>
#include <QMutex>

class imageCache;

class imageLoader: public QThread{
	Q_OBJECT
	
	private:
		QMutex mutex;
		imageCache *image;
		imageCache *loading;
		bool delete_after_load;
		QString file;	//Path to file which shall be loaded
	
	protected:
		void run();
	
	public:
		explicit imageLoader();
		bool load_image( imageCache *img, QString filepath );
		void delete_image( imageCache *img );
		
	signals:
		void image_fetched();
		void image_loaded( imageCache *img );
};


#endif