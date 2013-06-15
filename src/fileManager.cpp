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


fileManager::fileManager(){
	connect( &loader, SIGNAL( image_fetched() ), this, SLOT( loading_handler() ) );
//	connect( &loader, SIGNAL( image_fetched() ), this, SLOT( emit_file_changed() ) );
	
	current_file = -1;
	
	//Initialize all supported image formats
	if( supported_file_ext.count() == 0 ){
		QList<QByteArray> supported = QImageReader::supportedImageFormats();
		for( int i=0; i<supported.count(); i++ )
			supported_file_ext << "*." + QString( supported.at( i ) );
	}
}

fileManager::~fileManager(){
	clear_cache();
}


void fileManager::set_files( QString file ){
	qDebug( "load_image: %s", file.toLatin1().constData() );
	//Load and display the file
	//TODO: check if image is supported!
	QFileInfo img_file( file );
	
	//TODO: if hidden, include hidden files
	
	//Begin caching
	files = img_file.dir().entryInfoList( supported_file_ext , QDir::Files, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware );
	qDebug( "files.count(): %d", files.count() );
	init_cache( file );
}

void fileManager::load_image( int pos ){
	if( cache[pos] )
		return;
	
	imageCache *img = new imageCache();
	if( loader.load_image( img, files[pos].filePath() ) ){
	qDebug( "loading image: %s", files[pos].filePath().toLocal8Bit().constData() );
		cache[pos] = img;
		if( pos == current_file )
			emit_file_changed();
	}
	else
		delete img;	//If it is already loading an image
}

void fileManager::init_cache( int start ){
	clear_cache();
	cache.clear();
	current_file = start;
	
	qDebug( "current_file: %d", current_file );
	if( current_file == -1 )
		return;
	
	cache.reserve( files.count() );
	for( int i=0; i<files.count(); i++ )
		cache.append( NULL );
	
	load_image( current_file );
	loading_handler();
}

void fileManager::set_files( QStringList files ){
	
}


bool fileManager::has_previous() const{
	qDebug( "has_previous: %d", current_file );
	return current_file > 0;
}

bool fileManager::has_next() const{
	qDebug( "has_next: %d", current_file );
	return current_file + 1 < files.count();
}


void fileManager::next_file(){
	if( has_next() ){
		current_file++;
		emit_file_changed();
		loading_handler();
	}
}

void fileManager::previous_file(){
	if( has_previous() ){
		current_file--;
		emit_file_changed();
		loading_handler();
	}
}



void fileManager::loading_handler(){
	if( current_file == -1 )
		return;
	
	int loading_lenght = 10;
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
	
	//* Delete everything after loading length
	for( int i=current_file+loading_lenght; i<cache.count(); i++ ){
		loader.delete_image( cache[i] );
		cache[i] = NULL;
	}
	for( int i=current_file-loading_lenght; i>=0; i-- ){
		loader.delete_image( cache[i] );
		cache[i] = NULL;
	}
}


void fileManager::clear_cache(){
	current_file = -1;
	emit_file_changed();
	for( int i=0; i<cache.count(); i++ )
		loader.delete_image( cache[i] );
	cache.clear();
}

