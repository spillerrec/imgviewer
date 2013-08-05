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
#include <QSettings>
#include <QLinkedList>

#include "imageLoader.h"


class imageCache;

class fileManager : public QObject{
	Q_OBJECT
	
	private:
		const QSettings& settings;
		QFileSystemWatcher watcher;
		QStringList supported_file_ext;
		imageLoader loader;
		
		bool show_hidden;
		bool force_hidden;
		bool recursive;
		bool locale_aware;
		bool wrap;
		
		QString dir;       ///Current directory (without last '/')
		QString prefix;    ///prefix for 'files' to get full path
		QStringList files; ///relative paths to files
		QList<imageCache*> cache; ///loaded files
		int current_file;  ///Index to currently used file
		
		//Struct for keeping old caches
		struct oldCache{
			QString file;
			imageCache* cache;
		};
		unsigned buffer_max;
		QLinkedList<oldCache> buffer;
		void unload_image( int index );
		
		//Accessers to 'files'
		QString file( QString f ) const{ return prefix + f; }
		QString file( int index ) const{ return file( files[index] ); }
		QFileInfo fileinfo( int index ) const{
			return QFileInfo( file( index ) );
		}
		int index_of( QString file ) const;
		
		void load_image( int pos );
		
		void load_files( QDir dir );
		void init_cache( int start );
		void init_cache( QString start ){ init_cache( index_of( start ) ); }
		void clear_cache();
		
	public:
		explicit fileManager( const QSettings& settings );
		virtual ~fileManager(){ clear_cache(); }
		
		QStringList supported_extensions() const{ return supported_file_ext; }
		
		void set_show_hidden_files( bool value ){ show_hidden = value; }
		
		void set_files( QString file ){ set_files( QFileInfo( file ) ); }
		void set_files( QFileInfo file );
		
		bool has_file( int index ) const{ return index >= 0 && index < cache.count(); }
		bool has_file() const{ return has_file( current_file ); }
		int move( int offset ) const;
		
		bool has_previous() const{ return has_file( move( -1 ) ); }
		bool has_next() const{ return has_file( move( 1 ) ); }
		void goto_file( int index );
		void next_file(){ goto_file( move( 1 ) ); }
		void previous_file(){ goto_file( move( -1 ) ); }
		
		bool supports_extension( QString filename ) const;
		void delete_current_file();
		
		QString get_dir() const{ return dir; }
		imageCache* file() const{ return has_file() ? cache[current_file] : NULL; }
		QString file_name() const;
		
		
	private slots:
		void loading_handler();
		void dir_modified();
		
	signals:
		void file_changed();
};


#endif