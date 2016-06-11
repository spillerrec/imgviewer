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
#include <QCollator>

#include <memory>

#include "imageLoader.h"
#include "FileSystem/ExtensionChecker.hpp"


class imageCache;

class fileManager : public QObject{
	Q_OBJECT
	
	private:
		const QSettings& settings;
		QFileSystemWatcher watcher;
		ExtensionChecker have_ext;
		imageLoader loader;
		
		bool show_hidden;
		bool force_hidden;
		bool extension_hidden;
		bool recursive;
		bool wrap;
		
		QCollator collator;
		struct File{
			QString name; //relative file path
			QCollatorSortKey key;
			std::shared_ptr<imageCache> cache;
			
			File( QString name, const QCollator& c ) : name(name), key(c.sortKey( name )) { }
			//TODO: on win8.1 in release mode, if name are equals, key::compare returns a random value
			bool operator<( const File& other ) const{ return name != other.name && key < other.key; }
			bool operator==( const File& other ) const{ return key.compare(other.key) == 0; }
			bool operator!=( const File& other ) const{ return !(*this == other); }
		};
		template<class T> void clear_files( T& files ){
			files.clear();
		}
		QString dir;       //Current directory (without last '/')
		QList<File> files;
		int current_file;  //Index to currently used file
		
		unsigned buffer_max;
		QLinkedList<File> buffer;
		void unload_image( int index );
		
		//Accessors to 'files'
		QString prefix() const{ return recursive ? "" : dir + "/"; }
		QString file( QString f ) const{ return prefix() + f; }
		QString file( int index ) const{ return file( files[index].name ); }
		int index_of( File file ) const;
		
		void load_image( int pos );
		
		void load_files( QDir dir );
		void clear_cache();
		
		int find_file( File file );
		
	public:
		explicit fileManager( const QSettings& settings );
		virtual ~fileManager(){ clear_cache(); }
		
		void set_show_hidden_files( bool value ){ show_hidden = value; }
		
		void set_files( QString file ){ set_files( QFileInfo( file ) ); }
		void set_files( QFileInfo file );
		
		bool has_file( int index ) const{ return index >= 0 && index < files.size(); }
		bool has_file() const{ return has_file( current_file ); }
		int move( int offset ) const;
		
		bool has_previous() const{ return has_file( move( -1 ) ); }
		bool has_next() const{ return has_file( move( 1 ) ); }
		void goto_file( int index );
		void next_file(){ goto_file( move( 1 ) ); }
		void previous_file(){ goto_file( move( -1 ) ); }
		
		bool supports_extension( QString filename ) const{ return have_ext.matches( filename ); }
		void delete_current_file();
		
		QString get_dir() const{ return dir; }
		std::shared_ptr<imageCache> file() const{ qDebug( "file() : %d, %s", current_file, (has_file() ? "true" : "false" ) );
		return has_file() ? files[current_file].cache : std::shared_ptr<imageCache>(); }
		QString file_name() const;
		QString file_path() const{ return has_file() ? file( current_file ) : ""; }
		
		
	private slots:
		void loading_handler();
		void dir_modified();
		
	signals:
		void file_changed();
		void position_changed();
};


#endif
