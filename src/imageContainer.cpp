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
#include "fileManager.h"
#include "windowManager.h"

#include "ui_controls_ui.h"

#include <QDir>
#include <QTime>
#include <QKeyEvent>


#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>
#include <QCursor>
#include <QImageReader>
#include <QMessageBox>



void imageContainer::dragEnterEvent( QDragEnterEvent *event ){
	QList<QUrl> urls = event->mimeData()->urls();
	
	if( urls.count() == 1 ){	//Do we have exactly one url to accept?
		QString url = urls[0].toLocalFile();
		
		//Only accept the file if it has the correct extension
		if( files->supports_extension( url ) )
			event->acceptProposedAction();
	}
}
void imageContainer::dropEvent( QDropEvent *event ){
	if( event->mimeData()->hasUrls() ){
		event->setDropAction( Qt::CopyAction );
		
		load_image( event->mimeData()->urls()[0].toLocalFile() );
		
		event->accept();
	}
}

imageContainer::imageContainer( QWidget* parent ): QWidget( parent ), ui( new Ui_controls ){
	viewer = new imageViewer( this );
	ui->setupUi( this );

	files = new fileManager();
	
	is_fullscreen = false;
	
	setFocusPolicy( Qt::StrongFocus ); //Why this?
	
	ui->viewer_layout->addWidget( viewer );
	
	connect( viewer, SIGNAL( image_info_read() ), this, SLOT( update_controls() ) );
	connect( viewer, SIGNAL( image_changed() ), this, SLOT( update_controls() ) );
	connect( viewer, SIGNAL( double_clicked() ), this, SLOT( toogle_fullscreen() ) );
	connect( viewer, SIGNAL( rocker_left() ), this, SLOT( prev_file() ) );
	connect( viewer, SIGNAL( rocker_right() ), this, SLOT( next_file() ) );
	connect( ui->btn_sub_next, SIGNAL( pressed() ), viewer, SLOT( goto_next_frame() ) );
	connect( ui->btn_sub_prev, SIGNAL( pressed() ), viewer, SLOT( goto_prev_frame() ) );
	connect( ui->btn_pause, SIGNAL( pressed() ), this, SLOT( toogle_animation() ) );
	connect( files, SIGNAL( file_changed() ), this, SLOT( update_file() ) );
	
	connect( ui->btn_next, SIGNAL( pressed() ), this, SLOT( next_file() ) );
	connect( ui->btn_prev, SIGNAL( pressed() ), this, SLOT( prev_file() ) );
	
	

	
	manager = new windowManager( this );
	resize_window = true;
	//Center window on mouse cursor on start
	QSize half_size( frameGeometry().size() / 2 );
	move( QCursor::pos() - QPoint( half_size.width(), half_size.height() ) );
	
	update_controls();
	
	setAcceptDrops( true );
}


void imageContainer::load_image( QString filepath ){
	files->set_files( filepath );
	update_controls();
}

imageContainer::~imageContainer(){
	viewer->change_image( NULL, false );
	delete manager;
	delete files;
}


void imageContainer::update_file(){
	qDebug( "updating file: %s", files->file_name().toLocal8Bit().constData() );
	setWindowTitle( files->file_name() );
	update_controls();
	viewer->change_image( files->file(), false );
}


void imageContainer::next_file(){
	if( files->has_next() ){
		files->next_file();
		viewer->set_auto_scale( true );
	}
	else
		QApplication::beep();
}

void imageContainer::prev_file(){
	if( files->has_previous() ){
		files->previous_file();
		viewer->set_auto_scale( true );
	}
	else
		QApplication::beep();
}

void imageContainer::update_controls(){
	//Show amount of frames in file
	ui->lbl_image_amount->setText( QString::number( viewer->get_current_frame()+1 ) + "/" + QString::number( viewer->get_frame_amount() ) );
	
	//Disable button when animation is not playable
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
	ui->btn_next->setEnabled( files->has_next() );
	ui->btn_prev->setEnabled( files->has_previous() );
	
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


void imageContainer::toogle_fullscreen(){
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
}


void imageContainer::keyPressEvent( QKeyEvent *event ){
	Qt::KeyboardModifiers mods = event->modifiers();
	
	switch( event->key() ){
		case Qt::Key_Escape: close(); break;
		case Qt::Key_Left:
				if( mods & Qt::ControlModifier )
					viewer->goto_prev_frame();
				else
					prev_file();
			break;
		case Qt::Key_Right:
				if( mods & Qt::ControlModifier )
					viewer->goto_next_frame();
				else
					next_file();
			break;
		case Qt::Key_Space:
				if( mods & Qt::ControlModifier ){
					viewer->restart_animation();
					update_toogle_btn();
				}
				else
					toogle_animation();
			break;
		case Qt::Key_A:
				if( mods & Qt::ControlModifier ){
					resize_window = true;
					update_controls();
				}
				else
					event->ignore();
			break;
		case Qt::Key_F11:
				toogle_fullscreen();
			break;
			
		case Qt::Key_Delete:
				if( QMessageBox::question( this, "Delete?", tr( "Do you want to permanently delete the following file?\n" ) + files->file_name() ) == QMessageBox::Yes )
					files->delete_current_file();
			break;
		
		default: event->ignore();
	}
}


