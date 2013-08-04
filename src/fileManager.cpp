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

#include "fileManager.h"
#include "imageCache.h"


#include <QDir>
#include <QImageReader>
#include <QStringList>
#include <QCoreApplication>
#include <QTime>
#include <QDirIterator>
#include <QtAlgorithms>

#include <qglobal.h>
#ifdef Q_OS_WIN
	#include <qt_windows.h>
	#include <Shlobj.h>
#endif


fileManager::fileManager( const QSettings& settings ) : settings( settings ){
	connect( &loader, SIGNAL( image_fetched() ), this, SLOT( loading_handler() ) );
	connect( &watcher, SIGNAL( directoryChanged( QString ) ), this, SLOT( dir_modified() ) );
	
	bool hidden_default = false;
#ifdef Q_OS_WIN
	//Set show_hidden on Windows to match Windows Explorer setting
	SHELLSTATE lpss;
	SHGetSetSettings( &lpss, SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS, false );
	
	if( lpss.fShowAllObjects )
		hidden_default = true;
	//TODO: also match "show extensions"? It is stored in: fShowExtensions
#endif
	
	current_file = -1;
	show_hidden = settings.value( "loading/show-hidden-files", hidden_default ).toBool();
	force_hidden = false;
	recursive = settings.value( "loading/recursive", false ).toBool();
	locale_aware = settings.value( "loading/locale-sorting", true ).toBool();
	buffer_max = settings.value( "loading/buffer-max", 3 ).toInt();
	
	
	//Initialize all supported image formats
	if( supported_file_ext.count() == 0 ){
		QList<QByteArray> supported = QImageReader::supportedImageFormats();
		for( int i=0; i<supported.count(); i++ )
			supported_file_ext << "*." + QString( supported.at( i ) );
	}
}

int fileManager::index_of( QString file ) const{
	if( !locale_aware )
		return qBinaryFind( files.begin(), files.end(), file ) - files.begin();
	
	return files.indexOf( file );
}

void fileManager::set_files( QFileInfo file ){
	//Stop if it does not support file
	if( !supports_extension( file.fileName() ) ){
		clear_cache();
		return;
	}
	
	//If hidden, include hidden files
	force_hidden = file.isHidden();
	
	//Begin caching
	dir = file.dir().absolutePath();
	load_files( dir );
	init_cache( recursive ? file.filePath() : file.fileName() );
	
	watcher.addPath( dir );
}


struct localecomp{
	bool operator()( const QString& f1, const QString& f2 ) const{
		return QString::localeAwareCompare( f1, f2 ) < 0;
	}
};
void fileManager::load_files( QDir dir ){
	//If hidden, include hidden files
	QDir::Filters filters = QDir::Files | QDir::Readable;
	if( show_hidden || force_hidden )
		filters |= QDir::Hidden;
	
	//Begin caching
	files.clear();
	
	//This folder, or all sub-folders as well
	if( recursive ){
		prefix = "";
		
		QDirIterator it( dir.path()
			,	supported_file_ext, filters
			,	QDirIterator::Subdirectories
			);
		while( it.hasNext() ){
			it.next();
			files.append( it.filePath() );
		}
	}
	else{
		prefix = dir.path() + "/";
		
		QDirIterator it( dir.path(), supported_file_ext, filters );
		while( it.hasNext() ){
			it.next();
			files.append( it.fileName() );
		}
	}
	
	//Sort using unicode or normal
	if( locale_aware )
		qSort( files.begin(), files.end(), localecomp() );
	else
		qSort( files );
}

void fileManager::load_image( int pos ){
	if( cache[pos] )
		return;
	
	//Check buffer first
	QLinkedList<oldCache>::iterator it = buffer.begin();
	for( ; it != buffer.end(); ++it )
		if( (*it).file == files[pos] ){
			cache[pos] = (*it).cache;
			buffer.erase( it );
			if( pos == current_file )
				emit file_changed();
			return;
		}
	
	//Load image
	imageCache *img = new imageCache();
	if( loader.load_image( img, file( pos ) ) ){
		qDebug( "loading image: %s", file( pos ).toLocal8Bit().constData() );
		cache[pos] = img;
		if( pos == current_file )
			emit file_changed();
	}
	else
		delete img;	//If it is already loading an image
}

void fileManager::init_cache( int start ){
	clear_cache();
	current_file = start;
	
	qDebug( "current_file: %d", current_file );
	if( current_file == -1 )
		return;
	
	cache.reserve( files.count() );
	for( int i=0; i<files.count(); i++ )
		cache << NULL;
	
	load_image( current_file );
	loading_handler();
}


bool fileManager::has_previous() const{
	return current_file > 0;
}

bool fileManager::has_next() const{
	return current_file + 1 < files.count();
}


void fileManager::next_file(){
	if( has_next() ){
		current_file++;
		emit file_changed();
		loading_handler();
	}
}

void fileManager::previous_file(){
	if( has_previous() ){
		current_file--;
		emit file_changed();
		loading_handler();
	}
}


void fileManager::unload_image( int index ){
	if( !cache[index] )
		return;
	
	//Save cache in buffer
	oldCache abandoned = { files[index], cache[index] };
	buffer << abandoned;
	cache[index] = NULL;
	
	//Remove if there becomes too many
	while( (unsigned)buffer.count() > buffer_max )
		loader.delete_image( buffer.takeFirst().cache );
}

void fileManager::loading_handler(){
	if( current_file == -1 )
		return;
	
	int loading_lenght = settings.value( "loading/lenght", 5 ).toInt();
	for( int i=0; i<loading_lenght; i++ ){
		if( current_file + i < files.count() && !cache[current_file+i] ){
			load_image( current_file+i );
			break;
		}
		
		if( current_file - i >= 0 && !cache[current_file-i] ){
			load_image( current_file-i );
			break;
		}
	}
	
	//* Unload everything after loading length
	for( int i=current_file+loading_lenght; i<cache.count(); i++ )
		unload_image( i );
	for( int i=current_file-loading_lenght; i>=0; i-- )
		unload_image( i );
}


void fileManager::clear_cache(){
	if( watcher.directories().count() > 0 )
		watcher.removePaths( watcher.directories() );
	current_file = -1;
	emit file_changed();
	
	//Delete any images in the buffer and cache
	for( int i=0; i<cache.count(); ++i )
		loader.delete_image( cache[i] );
	cache.clear();
	
	while( !buffer.isEmpty() )
		loader.delete_image( buffer.takeFirst().cache );
}

//Make sure that this is done without caching
static bool file_exists( QFileInfo file ){
	file.setCaching( false );
	return file.exists();
}

void fileManager::dir_modified(){
	if( !has_file() )
		return;
	
	//Wait shortly to ensure files have been updated
	//Solution by kshark27: http://stackoverflow.com/a/11487434/2248153
	QTime wait = QTime::currentTime().addMSecs( 200 );
	while( QTime::currentTime() < wait )
		QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
	
	//Find the file to show now, considering it might have disappeared
	QString new_file = files[current_file];
	if( !file_exists( fileinfo( current_file ) ) ){
		//Keep trying until we find a file which still exists
		int prev = current_file-1;
		int next = current_file+1;
		while( prev >= 0 || next < files.count() ){
			if( next < files.count() ){
				if( file_exists( fileinfo( next ) ) ){
					new_file = files[next];
					break;
				}
				else
					next++;
			}
			
			if( prev >= 0 ){
				if( file_exists( fileinfo( prev ) ) ){
					new_file = files[prev];
					break;
				}
				else
					prev--;
			}
		}
	}
	
	//Save imageCache's which might still be valid
	QList<oldCache> old;
	for( int i=0; i<cache.count(); i++ )
		if( cache[i] ){
			oldCache temp = { files[i], cache[i] };
			old << temp;
		}
	
	//Prepare the QLists
	cache.clear();
	load_files( QFileInfo( file( new_file ) ).dir() );
	cache.reserve( files.count() );
	for( int i=0; i<files.count(); i++ )
		cache << NULL;
	
	//Restore old elements
	for( int i=0; i<old.count(); i++ ){
		int new_index = index_of( old[i].file );
		if( new_index != -1 ){
			cache[new_index] = old[i].cache;
			old[i].cache = NULL;
		}
	}
	
	//Set image position
	current_file = index_of( new_file );
	emit file_changed(); //The file and position could have changed
	
	if( current_file == -1 )
		return;
	
	//Now delete images which are no longer here
	//We can't do it earlier than emit file_changed(), as imageViewer needs to disconnect first
	for( int i=0; i<old.count(); i++ )
		if( old[i].cache )
			loader.delete_image( old[i].cache );
		
	//Start loading the new files
	if( !cache[ current_file ] )
		load_image( current_file );
	loading_handler();
}


bool fileManager::supports_extension( QString filename ) const{
	filename = filename.toLower();
	for( int i=0; i<supported_file_ext.count(); i++ )
		if( filename.endsWith( QString( supported_file_ext[i] ).remove( 0, 1 ) ) ) //Remove "*" from "*.ext"
			return true;
	return false;
}

void fileManager::delete_current_file(){
	if( !has_file() )
		return;
	
	//QFileWatcher will ensure that the list will be updated
	QFile::remove( file( current_file ) );
}


QString fileManager::file_name() const{
	if( !has_file() )
		return "No file!";
	
	//TODO: once we have a meta-data system, check if it contains a title
	return QString( "%1 - [%2/%3]" )
		.arg( files[current_file] )
		.arg( QString::number( current_file+1 ) )
		.arg( QString::number( files.count() ) )
		;
}

