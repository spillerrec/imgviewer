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

#include "viewer/imageCache.h"
#include "ImageReader/ImageReader.hpp"

#include <QDir>
#include <QStringList>
#include <QCoreApplication>
#include <QTime>
#include <QDirIterator>
#include <QtAlgorithms>

#include <QMutex>
#include <QMutexLocker>

#include <qglobal.h>
#ifdef Q_OS_WIN
	#include <qt_windows.h>
	#include <Shlobj.h>
#endif


fileManager::fileManager( const QSettings& settings ) : settings( settings ), have_ext( ImageReader().supportedExtensions() ){
	connect( &loader, SIGNAL( image_fetched() ), this, SLOT( loading_handler() ) );
	connect( &watcher, SIGNAL( directoryChanged( QString ) ), this, SLOT( dir_modified() ) );
	
	bool hidden_default = false;
	bool extension_default = false;
#ifdef Q_OS_WIN
	//Set show_hidden on Windows to match Windows Explorer setting
	SHELLSTATE lpss;
	SHGetSetSettings( &lpss, SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS, false );
	
	hidden_default = lpss.fShowAllObjects;
	extension_default = !lpss.fShowExtensions;
#endif
	
	current_file = -1;
	show_hidden = settings.value( "loading/show-hidden-files", hidden_default ).toBool();
	extension_hidden = settings.value( "loading/hide-extensions", extension_default ).toBool();
	force_hidden = false;
	recursive = settings.value( "loading/recursive", false ).toBool();
	wrap = settings.value( "loading/wrap", true ).toBool();
	buffer_max = settings.value( "loading/buffer-max", 3 ).toInt();
	
	//Set collation settings
	collator.setNumericMode( settings.value( "loading/natural-number-order", false ).toBool() );
	bool case_sensitivity = settings.value( "loading/case-sensitive", false ).toBool();
	collator.setCaseSensitivity( case_sensitivity ? Qt::CaseSensitive : Qt::CaseInsensitive );
	bool punctuation = settings.value( "loading/ignore-punctuation", collator.ignorePunctuation() ).toBool();
	collator.setIgnorePunctuation( punctuation );
}

int fileManager::index_of( File file ) const{
	auto it = qBinaryFind( files.begin(), files.end(), file );
	return it != files.end() ? it - files.begin() : -1;
}

void fileManager::set_files( QFileInfo file ){
	//Stop if it does not support file
	if( !supports_extension( file.fileName() ) ){
		clear_cache();
		emit file_changed();
		return;
	}
	
	//If hidden, include hidden files
	force_hidden = file.isHidden();
	
	//Old filename
	QString old_name = has_file() ? files[current_file].name : "";
	
	//Begin caching
	if( dir == file.dir().absolutePath() )
		dir_modified();
	else{
		//Start loading image instantly
		auto img = loader.load_image( file.absoluteFilePath() );
		
		load_files( file );
		
		current_file = index_of( {	recursive ? file.filePath() : file.fileName(), collator });
		files[current_file].cache = std::move(img);
		emit position_changed();
		emit file_changed();
		
		if( !files[current_file].cache )
			load_image( current_file );
		loading_handler();
	}
}

void fileManager::load_files( QFileInfo file ){
	QDir current_dir( file.dir().absolutePath() );
	//If hidden, include hidden files
	QDir::Filters filters = QDir::Files;
	if( show_hidden || force_hidden )
		filters |= QDir::Hidden;
	
	//Begin caching
	clear_cache();
	
	//This folder, or all sub-folders as well
	prefix = recursive ? "" : current_dir.path() + "/";
	QDirIterator it( current_dir.path(), {}, filters
		,	recursive ? QDirIterator::Subdirectories : QDirIterator::NoIteratorFlags
		);
	while( it.hasNext() ){
		it.next();
		auto file = recursive ? it.filePath() : it.fileName();
		if( supports_extension( file ) )
			files.push_back( {file, collator} );
	}
	
	qSort( files.begin(), files.end() );
	
	QString new_dir = current_dir.absolutePath();
	if( new_dir != dir ){
		dir = new_dir;
		watcher.addPath( dir );
	}
}

void fileManager::load_image( int pos ){
	if( files[pos].cache )
		return;
	
	//Check buffer first
	QLinkedList<File>::iterator it = buffer.begin();
	for( ; it != buffer.end(); ++it )
		if( *it == files[pos] ){
			files[pos] = *it;
			buffer.erase( it );
			if( pos == current_file )
				emit file_changed();
			return;
		}
	
	//Load image
	files[pos].cache = loader.load_image( file( pos ) );
	if( files[pos].cache )
		emit file_changed();
}

int fileManager::move( int offset ) const{
	int wanted = current_file + offset;
	
	if( !wrap || files.size() <= 0 )
		return wanted; //empty list would cause infinite loop
	
	//Keep warping until we reached a valid index
	while( wanted < 0 )
		wanted += files.size();
	while( wanted >= files.size() )
		wanted -= files.size();
	
	return wanted;
}

void fileManager::goto_file( int index ){
	if( has_file( index ) ){
		current_file = index;
		emit file_changed();
		emit position_changed();
		loading_handler();
	}
}


void fileManager::unload_image( int index ){
	if( !has_file(index) || !files[index].cache )
		return;
	
	//Save cache in buffer
	buffer << std::move( files[index] );
	
	//Remove if there becomes too many
	while( (unsigned)buffer.size() > buffer_max )
		auto buf = buffer.takeFirst();
}

void fileManager::loading_handler(){
	if( current_file == -1 )
		return;
	
	int loading_lenght = settings.value( "loading/length", 2 ).toInt();
	for( int i=0; i<=loading_lenght; i++ ){
		int next = move( i );
		if( has_file(next) && !files[next].cache ){
			load_image( next );
			break;
		}
		
		int prev = move( -i );
		if( has_file(prev) && !files[prev].cache ){
			load_image( prev );
			break;
		}
	}
	
	// Unload everything after loading length
	int last = move( loading_lenght+1 );
	int first = move( -loading_lenght-1 );
	if( last > first ){
		for( int i=last; i<files.size(); i++ )
			unload_image( i );
		for( int i=first; i>=0; i-- )
			unload_image( i );
	}
	else
		for( int i=last; i<=first; i++ )
			unload_image( i );
}


void fileManager::clear_cache(){
	if( watcher.directories().size() > 0 )
		watcher.removePaths( watcher.directories() );
	dir = "";
	if( current_file != -1 ){
		current_file = -1;
		emit file_changed();
	}
	
	//Delete any images in the buffer and cache
	clear_files( files );
	clear_files( buffer );
}

static QMutex mutex;
void fileManager::dir_modified(){
	//Make absolutely sure this is not called again before it finish loading
	disconnect( &watcher, SIGNAL( directoryChanged( QString ) ), this, SLOT( dir_modified() ) );
	QMutexLocker locker(&mutex);
	
	//Wait shortly to ensure files have been updated
	//Solution by kshark27: http://stackoverflow.com/a/11487434/2248153
	QTime wait = QTime::currentTime().addMSecs( 200 );
	while( QTime::currentTime() < wait )
		QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
	
	connect( &watcher, SIGNAL( directoryChanged( QString ) ), this, SLOT( dir_modified() ) );
	
	//processEvents can modify this object, so check this afterwards
	if( !has_file() )
		return;
	
	//Save imageCache's which might still be valid
	QList<File> old;
	for( auto& file : files )
		if( file.cache ){
			old << file;
			file.cache = nullptr;
		}
	
	//Keep the name of the old file, for restoring position
	File old_file = files[current_file];
	
	//Prepare the QLists
	current_file = -1; //TODO: we need this to avoid clear_cache() to notify the viewer. FIX
	load_files( QFileInfo( file( old_file.name ) ) );
	
	//Restore old elements
	for( auto& elem : old ){
		int new_index = index_of( elem );
		if( new_index != -1 ){
			files[new_index] = elem;
			elem.cache = nullptr;
		}
	}
	
	if( files.size() == 0 ){
		if( settings.value( "loading/quit-on-empty", false ).toBool() )
			QCoreApplication::quit();
		current_file = -1;
		emit file_changed();
		return; //TODO: can't do this!
	}
	
	//Set image position, using lower bound to find the closest
	auto it = qLowerBound( files.begin(), files.end(), old_file );
	current_file = (it != files.end()) ? it - files.begin() : files.size()-1;
	qDebug( "current_file: %d", current_file );
	emit position_changed();
	
	if( files[current_file] != old_file )
		emit file_changed();
	
	//Now delete images which are no longer here
	//We can't do it earlier than emit file_changed(), as imageViewer needs to disconnect first
	clear_files( old );
		
	//Start loading the new files
	if( !files[ current_file ].cache ){
		emit file_changed();
		load_image( current_file );
	}
	loading_handler();
}

void fileManager::delete_current_file(){
	//QFileWatcher will ensure that the list will be updated
	QFile::remove( file_path() );
}


QString fileManager::file_name() const{
	if( !has_file() )
		return "No file!";
	
	//File name
	QString name = files[current_file].name;
	int dot = name.lastIndexOf( '.' );
	if( extension_hidden && dot != -1 )
		name = name.left( dot );
	
	//TODO: once we have a meta-data system, check if it contains a title
	return QString( "%1 - [%2/%3]" )
		.arg( name )
		.arg( QString::number( current_file+1 ) )
		.arg( QString::number( files.size() ) )
		;
}

