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

#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>

#include <QFileDialog>
#include <QMessageBox>
#include <QCursor>
#include <QKeyEvent>
#include <QMenu>
#include <QStandardPaths>


void imageContainer::dragEnterEvent( QDragEnterEvent *event ){
	QList<QUrl> urls = event->mimeData()->urls();
	
	if( urls.count() == 1 ){	//Do we have exactly one url to accept?
		//Only accept the file if it has the correct extension
		if( files->supports_extension( urls[0].toLocalFile() ) )
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

imageContainer::imageContainer( QWidget* parent ) : QWidget( parent )
	,	ui( new Ui_controls )
#ifdef PORTABLE //Portable settings
	,	settings( QCoreApplication::applicationDirPath() + "/settings.ini", QSettings::IniFormat )
#else
	,	settings( "spillerrec", "imgviewer" )
#endif
	{
	//Init properties
	menubar = NULL;
	context = NULL;
	menubar_autohide = settings.value( "autohide-menubar", false ).toBool();
	is_fullscreen = false;
	setAcceptDrops( true ); //Drag&Drop
	setContextMenuPolicy( Qt::PreventContextMenu );
	
	//Init components
	viewer = new imageViewer( settings, this );
	files = new fileManager( settings );
	manager = new windowManager( this );
	ui->setupUi( this );

	//Add and refresh widgets
	create_menubar();
	ui->viewer_layout->insertWidget( 1, viewer );
	update_controls();
	create_context();
	
	//Center window on mouse cursor on start
	QSize half_size( frameGeometry().size() / 2 );
	move( QCursor::pos() - QPoint( half_size.width(), half_size.height() ) );
	
	//Connect signals
	connect( viewer, SIGNAL( clicked() ),         this, SLOT( hide_menubar() ) );
	connect( viewer, SIGNAL( image_info_read() ), this, SLOT( update_controls() ) );
	connect( viewer, SIGNAL( image_changed() ),   this, SLOT( update_controls() ) );
	connect( viewer, SIGNAL( double_clicked() ),  this, SLOT( toogle_fullscreen() ) );
	connect( viewer, SIGNAL( resize_wanted() ),  this, SLOT( resize_window() ) );
	connect( viewer, SIGNAL( rocker_left() ),     this, SLOT( prev_file() ) );
	connect( viewer, SIGNAL( rocker_right() ),    this, SLOT( next_file() ) );
	connect( viewer, SIGNAL( context_menu(QContextMenuEvent) )
	       , this,     SLOT( context_menu(QContextMenuEvent) ) );
	connect( ui->btn_sub_next, SIGNAL( pressed() ), viewer, SLOT( goto_next_frame() ) );
	connect( ui->btn_sub_prev, SIGNAL( pressed() ), viewer, SLOT( goto_prev_frame() ) );
	connect( ui->btn_pause,    SIGNAL( pressed() ), this, SLOT( toogle_animation() ) );
	connect( ui->btn_next,     SIGNAL( pressed() ), this, SLOT( next_file() ) );
	connect( ui->btn_prev,     SIGNAL( pressed() ), this, SLOT( prev_file() ) );
	connect( files, SIGNAL( file_changed() ), this, SLOT( update_file() ) );
}


void imageContainer::create_menubar(){
	if( menubar )
		return;
	
	//Create and add
	menubar = new QMenuBar( this );
	menubar->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
	ui->viewer_layout->insertWidget( 0, menubar );
	
	//Init auto-hide
	connect( menubar, SIGNAL( triggered(QAction*) ), this, SLOT( hide_menubar() ) );
	if( menubar_autohide )
		menubar->hide();
	
	//Top-level menus
	QMenu* file_menu = menubar->addMenu( tr( "&File" ) );
	anim_menu = menubar->addMenu( tr( "&Animation" ) );
	QMenu* view_menu = menubar->addMenu( tr( "&View" ) );
	
	//General actions
	file_menu->addAction( "&Open", this, SLOT( open_file() ) );
	file_menu->addSeparator();
	file_menu->addAction( "&Delete", this, SLOT( delete_file() ) );
	//TODO: file_menu->addAction( "&Properties", this, SLOT( "show_properties() ) );
	file_menu->addSeparator();
	//TODO: file_menu->addAction( "&Settings", this, SLOT( show_settings() ) );
	file_menu->addAction( "E&xit", qApp, SLOT( quit() ) );
	
	//Animation actions
	anim_menu->addAction( "&Pause/resume", viewer, SLOT( toogle_animation() ) );
	anim_menu->addAction( "&Restart", viewer, SLOT( restart_animation() ) );
	anim_menu->addSeparator();
	anim_menu->addAction( "&Next frame", viewer, SLOT( goto_next_frame() ) );
	anim_menu->addAction( "Pre&vious frame", viewer, SLOT( goto_prev_frame() ) );
	//TODO: goto a specific frame
	
	//Actions related to the interface
	view_menu->addAction( "&Fullscreen", this, SLOT( toogle_fullscreen() ) );
	view_menu->addAction( "Fit window to &image", this, SLOT( resize_window() ), 400 );
	//TODO: hide and show menubar, statusbar, ...
}

void imageContainer::create_context(){
	if( context )
		return;
	
	context = new QMenu();
	context->addAction( "&Open", this, SLOT( open_file() ) );
	context->addAction( "&Delete", this, SLOT( delete_file() ) );
	context->addAction( "E&xit", qApp, SLOT( quit() ) );
}

void imageContainer::hide_menubar(){
	if( menubar && menubar_autohide )
		menubar->hide();
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


void imageContainer::open_file(){
	//Make filter
	QStringList ext = files->supported_extensions();
	QString filter = "Images (";
	for( int i=0; i<ext.count(); ++i )
		filter += ext[i] + " ";
	filter += ")";
	
	//Find folder to open in dialog
	QString folder = files->get_dir();
	if( folder.isEmpty() ){
		QStringList pic_folders = QStandardPaths::standardLocations( QStandardPaths::PicturesLocation );
		if( pic_folders.count() > 0 )
			folder = pic_folders[0];
	}
	
	//Show open file dialog and load the file on success
	QString file = QFileDialog::getOpenFileName( this
		,	tr( "Open image" )
		,	folder
		,	filter
		);
	
	if( !file.isEmpty() )
		load_image( file );
}
void imageContainer::next_file(){
	if( files->has_next() )
		files->next_file();
	else
		QApplication::beep();
}

void imageContainer::prev_file(){
	if( files->has_previous() )
		files->previous_file();
	else
		QApplication::beep();
}

void imageContainer::delete_file( bool ask ){
	if( !ask ){
		//Delete without warning
		files->delete_current_file();
		return;
	}
	
	QMessageBox::StandardButton result = QMessageBox::question(
			this, "Delete?"
		,	tr( "Do you want to permanently delete the following file?\n" ) + files->file_name()
		);
	
	if( result == QMessageBox::Yes )
		files->delete_current_file();
}

void imageContainer::update_controls(){
	//Show amount of frames in file
	ui->lbl_image_amount->setText(
			QString::number( viewer->get_current_frame()+1 )
			+ "/" +
			QString::number( viewer->get_frame_amount() )
		);
	
	//Disable button when animation is not playable
	update_toogle_btn(); //Make sure the icon is correct
	ui->btn_pause->setEnabled( viewer->can_animate() );
	//Disable animation menu as well
	if( menubar )
		anim_menu->setEnabled( viewer->can_animate() );
	
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
}


void imageContainer::update_toogle_btn(){
	ui->btn_pause->setIcon( QIcon( 
			viewer->is_animating() ? ":/main/pause.png" : ":/main/start.png"
		) );
}


void imageContainer::toogle_animation(){
	viewer->toogle_animation();
	
	update_toogle_btn();
}


void imageContainer::toogle_fullscreen(){
	if( is_fullscreen ){
		setStyleSheet( "" ); //Reset it
		showNormal();
		ui->control_sub->show();
		if( menubar && !menubar_autohide )
			menubar->show();
	}
	else{
		settings.beginGroup( "fullscreen" );
		
		setStyleSheet( settings.value( "style"
			,	"*, QMenuBar:item,QMenu:item{background: black;color:white;}"
				"QMenu, QMenuBar{border:1px solid lightgray}"
				"QMenuBar:item:disabled{color:gray}"
				"QMenuBar:item:selected,QMenu:item:selected,"
				"QMenuBar:item:pressed,QMenu:item:pressed{background:gray}"
				"QMenu:separator{background:lightgray;height:1px;margin:2px}"
			).toString() );
		
		if( settings.value( "hide_controls", true ).toBool() )
			ui->control_sub->hide();
		if( menubar && settings.value( "hide_menu", true ).toBool() )
			menubar->hide();
		
		showFullScreen();
		
		settings.endGroup();
	}
	is_fullscreen = !is_fullscreen;
}


void imageContainer::keyPressEvent( QKeyEvent *event ){
	Qt::KeyboardModifiers mods = event->modifiers();
	hide_menubar();
	
	switch( event->key() ){
		case Qt::Key_Escape: close(); break;
		case Qt::Key_Alt:
				if( menubar )
					menubar->show();
				else
					event->ignore();
			break;
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
				if( mods & Qt::ControlModifier )
					resize_window();
				else
					event->ignore();
			break;
		case Qt::Key_O:
				if( mods & Qt::ControlModifier )
					open_file();
			break;
		case Qt::Key_F11:
				toogle_fullscreen();
			break;
			
		case Qt::Key_Delete:
				delete_file( !(mods & Qt::ShiftModifier) );
			break;
		
		default: event->ignore();
	}
}

void imageContainer::contextMenuEvent( QContextMenuEvent* event ){
	//TODO: keyboard...
	if( context )
		context->exec( event->globalPos() );
}

void imageContainer::mousePressEvent( QMouseEvent* event ){
	hide_menubar();
	
	if( event->button() == viewer->get_context_button() )
		viewer->create_context_event( *event );
}

void imageContainer::resize_window(){
	if( !is_fullscreen ) //Buggy in fullscreen
		manager->resize_content( viewer->sizeHint(), viewer->size(), true );
}

