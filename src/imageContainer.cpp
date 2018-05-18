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
#include "ImageReader/ImageReader.hpp"
#include "viewer/settings/ViewerSettings.h"
#include "viewer/imageViewer.h"
#include "viewer/imageCache.h"
#include "fileManager.h"
#include "windowManager.h"

#include "ui_controls_ui.h"

#include <QMimeData>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QUrl>

#include <QClipboard>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QCursor>
#include <QKeyEvent>
#include <QMenu>
#include <QStandardPaths>


imageContainer::AnimButton::AnimButton( imageContainer* parent )
	:	playing( ":/main/pause.png" )
	,	paused(  ":/main/start.png" )
	,	parent( parent ) { }


void imageContainer::AnimButton::setState( bool is_playing ){
	auto icon = is_playing ? playing : paused;
	parent->ui->btn_pause->setIcon( icon );
	
#ifdef WIN_TOOLBAR
	if( parent->btn_pause )
		parent->btn_pause->setIcon( icon );
#endif
}


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
	,	animation( this )
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
	files = std::make_unique<fileManager>( settings );
	manager = std::make_unique<windowManager>( *this );
	ui->setupUi( this );

	//Add and refresh widgets
	create_menubar();
	ui->viewer_layout->insertWidget( 1, viewer );
	updateImageInfo();
	updatePosition();
	create_context();
	
	//Connect signals
	connect( viewer, SIGNAL( clicked() ),         this, SLOT( hide_menubar() ) );
	connect( viewer, SIGNAL( image_info_read() ), this, SLOT( updateImageInfo() ) );
	connect( viewer, SIGNAL( image_changed() ),   this, SLOT( updateImageInfo() ) );
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
	connect( files.get(), SIGNAL( file_changed() ),     this, SLOT( update_file() ) );
	connect( files.get(), SIGNAL( position_changed() ), this, SLOT( updatePosition() ) );
}

//We just need this here to avoid including fileManager and windowManager in the header
imageContainer::~imageContainer(){}

#ifdef WIN_TOOLBAR
void imageContainer::init_win_toolbar(){
	auto *thumbnail_bar = new QWinThumbnailToolBar( this );
	thumbnail_bar->setWindow( windowHandle() );
	
	auto make_btn = [&]( const char* name, const char* icon, void (imageContainer::*slot)() ){
			auto btn = new QWinThumbnailToolButton( thumbnail_bar );
			btn->setEnabled( false );
			btn->setToolTip( tr( name ) );
			btn->setIcon( QIcon( icon ) );
			connect( btn, &QWinThumbnailToolButton::clicked, this, slot );
			thumbnail_bar->addButton( btn );
			return btn;
		};
	
	btn_prev  = make_btn( "Previous", ":/main/prev.png" , prev_file );
	btn_pause = make_btn( "Pause"   , ":/main/start.png", toogle_animation );
	btn_next  = make_btn( "Next"    , ":/main/next.png" , next_file );
}
#endif


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
	create_context();
	menubar->addMenu( context );
	anim_menu = menubar->addMenu( tr( "&Animation" ) );
	auto view_menu = menubar->addMenu( tr( "&View" ) );
	auto about_menu = menubar->addMenu( tr( "A&bout" ) );
	
	//Animation actions
	anim_menu->addAction( "&Pause/resume",   viewer, SLOT( toogle_animation()  ) );
	anim_menu->addAction( "&Restart",        viewer, SLOT( restart_animation() ) );
	anim_menu->addSeparator();
	anim_menu->addAction( "&Next frame",     viewer, SLOT( goto_next_frame()   ) );
	anim_menu->addAction( "Pre&vious frame", viewer, SLOT( goto_prev_frame()   ) );
	//TODO: goto a specific frame
	
	//Actions related to the interface
	view_menu->addAction( "&Fullscreen", this, SLOT( toogle_fullscreen() ) );
	view_menu->addAction( "Fit window to &image", this, SLOT( resize_window() ), 400 );
	view_menu->addAction( "Rotate Left",         viewer, SLOT( rotateLeft()  ) );
	view_menu->addAction( "Rotate Right",        viewer, SLOT( rotateRight() ) );
	view_menu->addAction( "Mirror horizontally", viewer, SLOT( mirrorHor()   ) );
	view_menu->addAction( "Mirror vertically",   viewer, SLOT( mirrorVer()   ) );
	
	//TODO: hide and show menubar, statusbar, ...
	about_menu->addAction( "&Help", this, SLOT( open_help() ) );
}

void imageContainer::create_context(){
	if( context )
		return;
	context = new QMenu( tr( "&File" ) );
	
	auto scaling = new QMenu( tr("&Scaling"), context );
	
	//Smooth scaling option
	//Downscale viewer option
	auto smooth = scaling->addAction( "&Smooth scaling"
		, [&](bool checked){
				ViewerSettings(settings).smooth_scaling().set( checked );
				viewer->updateView();
			} );
	smooth->setCheckable( true );
	smooth->setChecked( ViewerSettings(settings).smooth_scaling() );
	
	scaling->addSeparator();
	
	//Downscale viewer option
	auto downscale = scaling->addAction( "Only &downscale"
		, [&](bool checked){
				ViewerSettings(settings).auto_downscale_only().set( checked );
				viewer->updateView();
			} );
	downscale->setCheckable( true );
	downscale->setChecked( ViewerSettings(settings).auto_downscale_only() );
	
	//Upscale viewer option
	auto upscale = scaling->addAction( "Only &upscale"
		, [&](bool checked){
				ViewerSettings(settings).auto_upscale_only().set( checked );
				viewer->updateView();
			} );
	upscale->setCheckable( true );
	upscale->setChecked( ViewerSettings(settings).auto_upscale_only() );
	
	scaling->addSeparator();
	
	//Keep-resize viewer option
	auto keep_resize = scaling->addAction( "&Keep resizing window"
		, [&](bool checked){
				ViewerSettings(settings).keep_resize().set( checked );
			} );
	keep_resize->setCheckable( true );
	keep_resize->setChecked( ViewerSettings(settings).keep_resize() );
	
	context->addAction( "&Open",      this, SLOT( open_file()      ) );
	context->addSeparator();
	context->addAction( "&Delete",    this, SLOT( delete_file()    ) );
	context->addAction( "&Copy",      this, SLOT( copy_file()      ) );
	context->addAction( "Copy &Path", this, SLOT( copy_file_path() ) );
	context->addSeparator();
	context->addMenu( scaling );
	context->addSeparator();
	context->addAction( "E&xit",      qApp, SLOT( quit()           ) );
}

void imageContainer::hide_menubar(){
	if( menubar && menubar_autohide )
		menubar->hide();
}

void imageContainer::load_image( QFileInfo file ){
	files->set_files( file );
	updatePosition();
}


void imageContainer::update_file(){
	qDebug( "updating file: %s", files->file_name().toLocal8Bit().constData() );
	viewer->change_image( files->file() );
	updateImageInfo();
}


void imageContainer::open_file(){
	//Make filter
	QStringList ext = ImageReader().supportedExtensions();
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
	QString file = QFileDialog::getOpenFileName( this, tr( "Open image" ), folder, filter );
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
	//Ask user before deleting
	if( ask ){
		auto message = tr( "Do you want to permanently delete the following file?\n" ) + files->file_name();
		if( QMessageBox::question( this, "Delete?", message ) != QMessageBox::Yes )
			return;
	}
	
	files->delete_current_file();
}

void imageContainer::open_help(){
	auto url = "https://github.com/spillerrec/imgviewer/wiki";
	auto message = tr( "Open the online help?\n" ) + url;
	if( QMessageBox::question( this, "Open online help", message ) == QMessageBox::Yes )
		QDesktopServices::openUrl( QUrl( url ) );
}

void imageContainer::copy_file(){
	QApplication::clipboard()->setImage( viewer->get_frame() );
}
void imageContainer::copy_file_path(){
	QApplication::clipboard()->setText( files->file_path() );
}

void imageContainer::updatePosition(){
	setWindowTitle( files->file_name() );
	
	//Disable left or right buttons
	ui->btn_next->setEnabled( files->has_next() );
	ui->btn_prev->setEnabled( files->has_previous() );
#ifdef WIN_TOOLBAR
	if( btn_prev && btn_next ){
		btn_next->setEnabled( files->has_next() );
		btn_prev->setEnabled( files->has_previous() );
	}
#endif
}
void imageContainer::updateImageInfo(){
	setWindowTitle( files->file_name() );
	
	//Show amount of frames in file
	int current_frame = viewer->get_current_frame() + 1;
	if( viewer->get_frame_amount() == 0 )
		current_frame = 0;
	ui->lbl_image_amount->setText(
			QString::number( current_frame )
			+ "/" +
			QString::number( viewer->get_frame_amount() )
		);
	
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
	
	//Disable button when animation is not playable
	update_toogle_btn(); //Make sure the icon is correct
	ui->btn_pause->setEnabled( viewer->can_animate() );
	//Disable animation menu as well
	if( menubar )
		anim_menu->setEnabled( viewer->can_animate() );
	
#ifdef WIN_TOOLBAR
	if( btn_pause )
		btn_pause->setEnabled( viewer->can_animate() );
#endif
}


void imageContainer::update_toogle_btn(){
	animation.setState( viewer->is_animating() );
}


void imageContainer::toogle_animation(){
	viewer->toogle_animation();
	update_toogle_btn();
}


void imageContainer::toogle_fullscreen(){
	if( is_fullscreen ){
		setStyleSheet( "" ); //Reset it
		if( was_maximized )
			showMaximized();
		else
			showNormal();
		ui->control_sub->show();
		if( menubar && !menubar_autohide )
			menubar->show();
	}
	else{
		was_maximized = isMaximized();
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
				else if( mods & Qt::ShiftModifier )
					viewer->rotateLeft();
				else
					prev_file();
			break;
		case Qt::Key_Right:
				if( mods & Qt::ControlModifier )
					viewer->goto_next_frame();
				else if( mods & Qt::ShiftModifier )
					viewer->rotateRight();
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
					resize_window( mods & Qt::ShiftModifier );
				else
					event->ignore();
			break;
		case Qt::Key_H: viewer->mirrorHor(); break;
		case Qt::Key_V: viewer->mirrorVer(); break;
		case Qt::Key_O:
				if( mods & Qt::ControlModifier )
					open_file();
			break;
			
		case Qt::Key_F1: open_help(); break;
		case Qt::Key_F11: toogle_fullscreen(); break;
			
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

void imageContainer::resize_window( bool only_upscale ){
	if( !is_fullscreen ) //Buggy in fullscreen
		manager->resize_content( viewer->sizeHint(), viewer->size(), viewer->auto_zoom_active(), only_upscale );
}

void imageContainer::center_window(){
	QSize half_size( frameGeometry().size() / 2 );
	move( QCursor::pos() - QPoint( half_size.width(), half_size.height() ) );
	manager->restrain_window();
}

