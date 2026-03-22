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
#include <QPushButton>


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
	connect( viewer, &imageViewer::clicked,         this, &imageContainer::hide_menubar );
	connect( viewer, &imageViewer::image_info_read, this, &imageContainer::updateImageInfo );
	connect( viewer, &imageViewer::image_changed,   this, &imageContainer::updateImageInfo );
	connect( viewer, &imageViewer::double_clicked,  this, &imageContainer::toogle_fullscreen );
	connect( viewer, &imageViewer::resize_wanted,   this, &imageContainer::resize_window_normal );
	connect( viewer, &imageViewer::rocker_left,     this, &imageContainer::prev_file );
	connect( viewer, &imageViewer::rocker_right,    this, &imageContainer::next_file );
	connect( viewer, &imageViewer::context_menu,    this, &imageContainer::context_menu );
	connect( ui->btn_sub_next, &QPushButton::pressed,  viewer, &imageViewer::goto_next_frame );
	connect( ui->btn_sub_prev, &QPushButton::pressed,  viewer, &imageViewer::goto_prev_frame );
	connect( ui->btn_pause,    &QPushButton::pressed,  this, &imageContainer::toogle_animation );
	connect( ui->btn_next,     &QPushButton::pressed,  this, &imageContainer::next_file );
	connect( ui->btn_prev,     &QPushButton::pressed,  this, &imageContainer::prev_file );
	connect( files.get(), &fileManager::file_changed,     this, &imageContainer::update_file );
	connect( files.get(), &fileManager::position_changed, this, &imageContainer::updatePosition );
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
	connect( menubar, &QMenuBar::triggered, this, &imageContainer::hide_menubar );
	if( menubar_autohide )
		menubar->hide();
	
	//Top-level menus
	create_context();
	menubar->addMenu( context );
	anim_menu = menubar->addMenu( tr( "&Animation" ) );
	auto view_menu = menubar->addMenu( tr( "&View" ) );
	auto about_menu = menubar->addMenu( tr( "A&bout" ) );
	
	//Animation actions
	addAction(anim_menu->addAction( "&Pause/resume",   viewer, &imageViewer::toogle_animation ));
	anim_menu->addAction( "&Restart",        viewer, &imageViewer::restart_animation );
	anim_menu->addSeparator();
	anim_menu->addAction( "&Next frame",     viewer, &imageViewer::goto_next_frame );
	anim_menu->addAction( "Pre&vious frame", viewer, &imageViewer::goto_prev_frame );
	//TODO: goto a specific frame
	
	//Actions related to the interface
	addAction(view_menu->addAction( "&Fullscreen",          this, &imageContainer::toogle_fullscreen, QKeySequence(Qt::Key_F11) ));
	addAction(view_menu->addAction( "Fit window to &image", this, &imageContainer::resize_window, QKeySequence(Qt::CTRL | Qt::Key_A) ));
	addAction(view_menu->addAction( "Rotate Left",          viewer, &imageViewer::rotateLeft ));
	addAction(view_menu->addAction( "Rotate Right",         viewer, &imageViewer::rotateRight ));
	addAction(view_menu->addAction( "Mirror horizontally",  viewer, &imageViewer::mirrorHor, QKeySequence(Qt::Key_H) ));
	addAction(view_menu->addAction( "Mirror vertically",    viewer, &imageViewer::mirrorVer, QKeySequence(Qt::Key_V) ));
	
	//TODO: hide and show menubar, statusbar, ...
	addAction(about_menu->addAction( "&Help", this, &imageContainer::open_help, QKeySequence(Qt::Key_F1) ));
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
	
	addAction(context->addAction( "&Open", this, &imageContainer::open_file, QKeySequence(Qt::CTRL | Qt::Key_O) ));
	context->addSeparator();
	context->addAction( "&Delete",    this, &imageContainer::delete_file    );
	context->addAction( "&Copy",      this, &imageContainer::copy_file      );
	context->addAction( "Copy &Path", this, &imageContainer::copy_file_path );
	context->addSeparator();
	context->addMenu( scaling );
	context->addSeparator();
	addAction(context->addAction( "E&xit", qApp, &QCoreApplication::quit, QKeySequence(Qt::Key_Escape) ));
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

