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
		
		bool show_hidden;
		bool force_hidden;
		
		QString dir;
		QFileInfoList files;
		QList<imageCache*> cache;
		int current_file;
		
		void load_image( int pos );
		
		void load_files( QDir dir );
		void init_cache( int start );
		void init_cache( QString start ){
			init_cache( files.indexOf( start ) );
		}
		void clear_cache();
		
	public:
		explicit fileManager();
		virtual ~fileManager();
		
		QStringList supported_extensions() const{ return supported_file_ext; }
		
		void set_show_hidden_files( bool value ){ show_hidden = value; }
		
		void set_files( QString file ){ set_files( QFileInfo( file ) ); }
		void set_files( QFileInfo file );
		
		bool has_file() const{ return current_file >= 0 && current_file < cache.count(); }
		
		bool has_previous() const;
		bool has_next() const;
		void next_file();
		void previous_file();
		
		bool supports_extension( QString filename ) const;
		void delete_current_file();
		
		QString get_dir() const{ return dir; }
		imageCache* file() const{ return has_file() ? cache[current_file] : NULL; }
		QString file_name() const;
		
		
	private slots:
		void loading_handler();
		
		void emit_file_changed(){
			qDebug( "emitting file_changed()" );
			emit file_changed();
		}
		void dir_modified( QString dir );
		
	signals:
		void file_changed();
};


#endif