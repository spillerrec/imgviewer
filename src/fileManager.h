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

#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfoList>
#include <QFileSystemWatcher>

#include "imageLoader.h"


class imageCache;

class fileManager : public QObject{
	Q_OBJECT
	
	private:
		QFileSystemWatcher watcher;
		QStringList supported_file_ext;
		imageLoader loader;
		
		
		QFileInfoList files;
		QList<imageCache*> cache;
		int current_file;
		
		void load_image( int pos );
		
		void init_cache( int start );
		void init_cache( QString start ){
			init_cache( files.indexOf( start ) );
		}
		void clear_cache();
		
	public:
		explicit fileManager();
		virtual ~fileManager();
		
		QStringList supported_extensions() const{ return supported_file_ext; }
		
		void set_files( QString file );
		void set_files( QStringList files );
		
		
		bool has_previous() const;
		bool has_next() const;
		void next_file();
		void previous_file();
		
		imageCache* file() const{ return current_file >= 0 ? cache[current_file] : NULL; }
		QString file_name() const{ return current_file >= 0 ? files[current_file].fileName() : "no file!"; }
		
		
	private slots:
		void loading_handler();
		
		void emit_file_changed(){
			qDebug( "emitting file_changed()" );
			emit file_changed();
		}
		
	signals:
		void file_changed();
};


#endif