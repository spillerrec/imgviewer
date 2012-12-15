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

#include "imageContainer.h"
#include "imageViewer.h"
#include "imageCache.h"
#include "windowManager.h"

#include "ui_controls_ui.h"

#include <QDir>
#include <QTime>
#include <QKeyEvent>


#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>


void imageContainer::dragEnterEvent( QDragEnterEvent *event ){
	if( event->mimeData()->hasUrls() )
		event->acceptProposedAction();
		//TODO: only accept if it includes files we want?
}
void imageContainer::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		
		load_image( event->mimeData()->urls()[0].toLocalFile() );
		
		event->accept();
	}
}



QStringList supported_file_ext =(QStringList)
	   "*.jpg"
	<< "*.png"
	<< "*.gif"
	<< "*.svg"
	<< "*.ico"
	<< "*.bmp"
	<< "*.jpeg"
	<< "*.tif"
	<< "*.tiff"
	<< "*.mng"
	<< "*.pbm"
	<< "*.ppm"
	<< "*.pgm"
	<< "*.xbm"
	<< "*.xpm"
	<< "*.ric"
;



imageContainer::imageContainer( QWidget* parent ): QWidget( parent ), ui( new Ui_controls ){
	viewer = new imageViewer( this );
	ui->setupUi( this );
	
	current_file = -1;
	is_fullscreen = false;
	
	setFocusPolicy( Qt::StrongFocus ); //Why this?
	
	ui->viewer_layout->addWidget( viewer );
	
	connect( viewer, SIGNAL( image_info_read() ), this, SLOT( update_controls() ) );
	connect( viewer, SIGNAL( image_changed() ), this, SLOT( update_controls() ) );
	connect( ui->btn_sub_next, SIGNAL( pressed() ), viewer, SLOT( goto_next_frame() ) );
	connect( ui->btn_sub_prev, SIGNAL( pressed() ), viewer, SLOT( goto_prev_frame() ) );
	connect( ui->btn_pause, SIGNAL( pressed() ), this, SLOT( toogle_animation() ) );
	
	connect( ui->btn_next, SIGNAL( pressed() ), this, SLOT( next_file() ) );
	connect( ui->btn_prev, SIGNAL( pressed() ), this, SLOT( prev_file() ) );
	
	connect( &loader, SIGNAL( image_fetched() ), this, SLOT( loading_handler() ) );
	
	manager = new windowManager( this );
	resize_window = true;
	
	setAcceptDrops( true );
}

imageContainer::~imageContainer(){
	viewer->change_image( NULL, false );
	delete manager;
	clear_cache();
}

void imageContainer::clear_cache(){
	//Warning, we might delete a cache used by imageViewer if not careful!!!
	current_file = -1;
	for( int i=0; i<cache.count(); i++ ){
		loader.delete_image( cache[i] );
		cache[i] = NULL;
	}
	cache.clear();
}

void imageContainer::create_cache( QString loaded_file, imageCache *loaded_image ){
	clear_cache();
	
	for( int i=0; i<files.count(); i++ ){
		if( files[i].fileName() == loaded_file ){
			cache.append( loaded_image );
			current_file = i;
		}
		else
			cache.append( NULL );
	}
	
	if( current_file == -1 ){
		qCritical( "loaded_image not added in cache!!! %s", loaded_file.toLatin1().constData() );
		
		for( int i=0; i<files.count(); i++ )
			qDebug( "file %d: %s", i, files[i].fileName().toLatin1().constData() );
	}
}



void imageContainer::load_image( QString filepath ){
	qDebug( "load_image: %s", filepath.toLatin1().constData() );
	//Load and display the file
	//TODO: check if image is supported!
	QFileInfo img_file( filepath );
	imageCache *img = new imageCache();
	viewer->change_image( img, false );
	viewer->set_auto_scale( true );
	
	//Start loading the image in a seperate thread
	loader.load_image( img, img_file.filePath() );
	qDebug( "load_image: %s", img_file.filePath().toLatin1().constData() );
	
	//Update interface
	setWindowTitle( img_file.fileName() );
	//TODO: update as soon read_info has been run
	
	//Begin caching
	files = img_file.dir().entryInfoList( supported_file_ext , QDir::Files, QDir::Name | QDir::IgnoreCase | QDir::LocaleAware );
	create_cache( img_file.fileName(), img );
}


void imageContainer::next_file(){
	if( current_file + 1 < files.count() && cache[ current_file+1 ] ){
		current_file++;
		viewer->change_image( cache[ current_file ], false );
		viewer->set_auto_scale( true );
		setWindowTitle( files[current_file].fileName() );
		
		loading_handler();
	}
	else
		QApplication::beep();
}

void imageContainer::prev_file(){
	if( current_file - 1 >= 0 && cache[ current_file-1 ] ){
		current_file--;
		viewer->change_image( cache[ current_file ], false );
		viewer->set_auto_scale( true );
		setWindowTitle( files[current_file].fileName() );
		
		loading_handler();
	}
	else
		QApplication::beep();
}

void imageContainer::loading_handler(){
	if( current_file == -1 )
		return;
	
	int loading_lenght = 10;
	for( int i=0; i<loading_lenght; i++ ){
		if( current_file + i < files.count() && !cache[current_file+i] ){
			imageCache *img = new imageCache();
			if( !loader.load_image( img, files[current_file+i].filePath() ) )
				delete img;	//If it is already loading an image
			else
				cache[current_file+i] = img;
			
			break;
		}
		
		if( current_file - i >= 0 && !cache[current_file-i] ){
			imageCache *img = new imageCache();
			if( !loader.load_image( img, files[current_file-i].filePath() ) )
				delete img;	//If it is already loading an image
			else
				cache[current_file-i] = img;
			
			break;
		}
	}
	
	//* Delete everything after loading lenght
	for( int i=current_file+loading_lenght; i<cache.count(); i++ ){
		loader.delete_image( cache[i] );
		cache[i] = NULL;
	}
	for( int i=current_file-loading_lenght; i>=0; i-- ){
		loader.delete_image( cache[i] );
		cache[i] = NULL;
	}
	//*/
	
	
	/* Delete when using too much memory
	int offset = 1;
	long memory = 0;
	long memory_limit = 512*1024*1024;
	while( current_file-offset > 0 || current_file+offset<cache.count() ){
		int pos_idx = current_file + offset;
		int neg_idx = current_file - offset;
		
		if( pos_idx < cache.count() && cache[ pos_idx ] ){
			memory += cache[ pos_idx ]->get_memory_size();
			if( memory > memory_limit ){
				loader.delete_image( cache[pos_idx] );
				cache[pos_idx] = NULL;
			}
		}
		
		if( neg_idx >= 0 && cache[ neg_idx ] ){
			memory += cache[ neg_idx ]->get_memory_size();
			if( memory > memory_limit ){
				loader.delete_image( cache[neg_idx] );
				cache[neg_idx] = NULL;
			}
		}
		
		offset++;
	}
	//*/
}


void imageContainer::update_controls(){
	//Show amount of frames in file
	ui->lbl_image_amount->setText( QString::number( viewer->get_current_frame()+1 ) + "/" + QString::number( viewer->get_frame_amount() ) );
	
	//Disable button when animation is not applyable
	update_toogle_btn(); //Make sure the icon is correct
	ui->btn_pause->setEnabled( viewer->can_animate() );
	
	//Disable sub left and right buttons
	bool frames_exists = viewer->get_frame_amount() > 1;
	if( frames_exists && !viewer->can_animate() ){
		//prevent wrap on non-animated files
		ui->btn_sub_next->setEnabled( viewer->get_current_frame() != viewer->get_frame_amount()-1 );
		ui->btn_sub_prev->setEnabled( viewer->get_current_frame() != 0 );
	}
	else{
		//Only one frame exists, or it is animated
		ui->btn_sub_next->setEnabled( frames_exists );
		ui->btn_sub_prev->setEnabled( frames_exists );
	}
	
	//Disable left or right buttons
	ui->btn_next->setEnabled( current_file != cache.size()-1 );
	ui->btn_prev->setEnabled( current_file != 0 );
	
	//Resize and move window to fit image
	if( resize_window && !is_fullscreen ){ //Buggy behaviour in fullscreen
		QSize wanted = viewer->frame_size();
		if( !wanted.isNull() ){
			manager->resize_content( wanted, viewer->size(), true );
			resize_window = false;
		}
	}
}


void imageContainer::update_toogle_btn(){
	if( viewer->is_animating() )
		ui->btn_pause->setIcon( QIcon( ":/main/pause.png" ) ); //"||" );
	else
		ui->btn_pause->setIcon( QIcon( ":/main/start.png" ) );
}


void imageContainer::toogle_animation(){
	viewer->toogle_animation();
	
	update_toogle_btn();
}


void imageContainer::keyPressEvent( QKeyEvent *event ){
	switch( event->key() ){
		case Qt::Key_Left: prev_file(); break;
		case Qt::Key_Right: next_file(); break;
		case Qt::Key_F11:				
				if( is_fullscreen ){
					viewer->set_background_color( QPalette().color( QPalette::Window ) );
					showNormal();
					ui->control_sub->show();
				}
				else{
					viewer->set_background_color( QColor( Qt::black ) );
					showFullScreen();
					ui->control_sub->hide();
				}
				is_fullscreen = !is_fullscreen;
			break;
		
		default: event->ignore();
	}
}


