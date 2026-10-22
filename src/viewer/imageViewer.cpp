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

#include "imageViewer.h"
#include "imageCache.h"
#include "colorManager.h"
#include "settings/ViewerSettings.h"
#include <qobject.h>

using namespace std;

#include <QPoint>
#include <QSize>
#include <QRect>

#include <QPainter>
#include <QImage>
#include <QTransform>
#include <QStaticText>
#include <QBrush>
#include <QPen>
#include <QColor>

#include <QTimer>

#include <QMouseEvent>
#include <QWheelEvent>

#include <QApplication>
#include <QScreen>

#include <QDrag>
#include <QMimeData>


using S = ViewerSettings;


imageViewer::imageViewer( QSettings& settings, QWidget* parent ): QWidget( parent ), settings( settings ){
	//User settings
	initial_resize = S(settings).initial_resize();
	
	time = new QTimer( this );
	time->setSingleShot( true );
	connect( time, &QTimer::timeout, this, &imageViewer::next_frame );
	
	viewbox_handler = std::make_unique<viewboxHandler>( settings, this );
	connect( viewbox_handler.get(), &viewboxHandler::double_clicked, this, [this]{ goto_next_frame(); } );
	connect( viewbox_handler.get(), &viewboxHandler::rocker_left,    this, &imageViewer::goto_prev_frame );
	connect( viewbox_handler.get(), &viewboxHandler::rocker_right,   this, &imageViewer::goto_next_frame );
	connect( viewbox_handler.get(), &viewboxHandler::toogle_animation, this, [this]{ toogle_animation(); } );
	connect( viewbox_handler.get(), &viewboxHandler::create_drag_and_drop, this, &imageViewer::create_drag_and_drop );
	connect( viewbox_handler.get(), &viewboxHandler::goto_next_frame,  this, &imageViewer::goto_next_frame );
	connect( viewbox_handler.get(), &viewboxHandler::goto_prev_frame,  this, &imageViewer::goto_prev_frame );
	connect( viewbox_handler.get(), &viewboxHandler::viewbox_changed,  this, [this]{ update(); } );
	
	setContextMenuPolicy( Qt::PreventContextMenu );
}

void imageViewer::updateOrientation( Orientation wanted, Orientation current ){
	auto orientation = current.difference( wanted ).normalized();
	if( orientation.rotation != 0 ){
		QTransform transform;
		transform.rotate( orientation.rotation * 90 );
		converted = converted.transformed( transform );
	}
	if (orientation.flip_hor)
		converted = converted.flipped(Qt::Horizontal);
	if (orientation.flip_ver)
		converted = converted.flipped(Qt::Vertical);
}

void imageViewer::rotate( int8_t amount ){
	auto transform = orientation.rotate( amount );
	updateOrientation( transform, orientation );
	orientation = transform;
	viewbox_handler->set_new_frame_size(frameSize());
	update();
}

void imageViewer::mirror( bool hor, bool ver ){
	auto transform = orientation.mirror( hor, ver );
	updateOrientation( transform, orientation );
	orientation = transform;
	update();
}

QImage imageViewer::get_frame(){
	if( !image_cache || current_frame >= image_cache->loaded() )
		return {};
	
	QScreen* current_monitor = screen();
	if( converted_monitor != current_monitor ){
		//Cache invalid, refresh
		converted = image_cache->frame( current_frame );
		
		//Transform colors to current monitor profile
		image_cache->get_manager()->doTransform( converted, image_cache->get_profile(), current_monitor );
		converted_monitor = current_monitor;
		
		updateOrientation( orientation.add(image_cache->get_orientation()), {} );
	}
	
	return converted;
}

QSize imageViewer::frameSize( unsigned index ) const{
	if( image_cache ){
		auto orient = orientation.add(image_cache->get_orientation());
		return orient.finalSize( image_cache->frame(index).size() );
	}
	else
		return {};
}

bool imageViewer::can_animate() const{
	return image_cache ? image_cache->is_animated() : false;
}

int imageViewer::get_frame_amount() const{ 
	return image_cache ? image_cache->frame_count() : 0; 
}

void imageViewer::change_frame( int wanted ){
	if( !image_cache )
		return;
	clear_converted();
	
	auto frame_amount = image_cache->frame_count();
	//Cycle backwards
	if( wanted < 0 )
		wanted = frame_amount - 1;
	
	if( image_cache->loaded() < frame_amount && wanted >= image_cache->loaded() ){
		//Wait for frame to be available
		waiting_on_frame = wanted;
		return;
	}
	
	if( frame_amount < 1 )
		return;	//Not loaded
	
	//Go to next frame
	current_frame = wanted;
	if( current_frame >= frame_amount ){
		//Last frame reached
		if( loop_counter > 0 ){
			//Reduce counter by one
			current_frame = 0;
			loop_counter--;
		}
		else if( loop_counter == -1 )
			current_frame = 0; //Repeat forever
		else{
			continue_animating = false;	//Stop looping
			current_frame--;
		}
	}
	
	
	if( continue_animating ){
		int delay = image_cache->frame_delay( current_frame );
		if( delay > 0 )
			time->start( delay );
	}
	
	update();
	emit image_changed();
}

void imageViewer::goto_frame( int index ){
	time->stop();
	continue_animating = false;
	change_frame( index );
	
	//Make sure it works if the new frame has different dimensions
	viewbox_handler->set_new_frame_size(frameSize());
}

void imageViewer::restart_animation(){
	continue_animating = can_animate();
	change_frame( 0 );
}

bool imageViewer::toogle_animation(){
	if( image_cache && image_cache->is_animated() ){
		if( continue_animating ){
			//TODO: check for stop on last frame
			time->stop();
			continue_animating = false;
		}
		else{
			continue_animating = true;
			next_frame();
		}
	}
	emit image_changed();
	
	return continue_animating;
}


void imageViewer::read_info(){
	if( !image_cache )
		return;
	
	loop_counter = image_cache->loop_count();
	continue_animating = image_cache->is_animated();
	
	emit image_info_read();
}
void imageViewer::check_frame( unsigned int idx ){
	if( idx == 0 )
		init_size();
	
	if( waiting_on_frame <= -1 )
		return;
	
	if( idx == (unsigned int)waiting_on_frame ){
		waiting_on_frame = -1;
		change_frame( idx );
	}
}

void imageViewer::init_size(){
	//TODO: customize
	if( initial_resize )
		emit resize_wanted();
	initial_resize = S(settings).keep_resize();
	
	viewbox_handler->set_new_frame_size( frameSize() );
	
	change_frame( 0 );
}


void imageViewer::change_image( std::shared_ptr<imageCache> new_image ){
	if( new_image == image_cache )
		return;
	
	time->stop(); //Prevent previous animation to interfere
	
	image_cache = std::move(new_image);
	waiting_on_frame = -1;
	current_frame = 0;
	clear_converted();
	
	if( image_cache ){
		switch( image_cache->get_status() ){
			case imageCache::INVALID:	break; //Loading failed
			
			//TODO: remove those connections again
			case imageCache::EMPTY:
					connect( image_cache.get(), &imageCache::info_loaded, this, &imageViewer::read_info );
					connect( image_cache.get(), &imageCache::frame_loaded, this, &imageViewer::check_frame );
				break;
			
			case imageCache::INFO_READY:
			case imageCache::FRAMES_READY:
					connect( image_cache.get(), &imageCache::info_loaded, this, &imageViewer::read_info );
					connect( image_cache.get(), &imageCache::frame_loaded, this, &imageViewer::check_frame );
					read_info();
				break;
			
			case imageCache::LOADED:
					read_info();
				break;
		}
		
		if( image_cache->loaded() >= 1 )
			init_size();
		else
			update();
		
		emit image_changed();
	}
	else
		update();
}


void imageViewer::draw_message( QStaticText *text ){
	text->prepare();	//Make sure it has calculated the size
	QSizeF txt_size = text->size();
	int x = ( size().width() - txt_size.width() ) * 0.5;
	int y = ( size().height() - txt_size.height() ) * 0.5;
	
	QPainter painter( this );
	//Prepare drawing
	painter.setRenderHints( QPainter::Antialiasing, true );
	painter.setBrush( QBrush( QColor( Qt::gray ) ) );
	painter.setPen( QPen( Qt::NoPen ) );
	
	//Draw the background, box and text
	painter.drawRoundedRect( x-10,y-10, txt_size.width()+20, txt_size.height()+20, 10, 10 );
	painter.setPen( QPen( Qt::black ) );
	painter.drawStaticText( x,y, *text );
}


void imageViewer::paintEvent( QPaintEvent* ){
	static QStaticText txt_loading( tr( "Loading" ) );
	static QStaticText txt_no_image( tr( "No image selected" ) );
	static QStaticText txt_invalid( tr( "Image invalid or broken!" ) );
	
	//Start checking for errors
	
	if( !image_cache || image_cache->get_status() == imageCache::EMPTY ){
		//We have nothing to display
		draw_message( &txt_no_image );
		return;
	}
	
	if( image_cache->get_status() == imageCache::INVALID ){
		//Image could not be loaded
		draw_message( &txt_invalid );
		return;
	}
	if( current_frame >= image_cache->loaded() ){
		//Image is currently loading
		draw_message( &txt_loading );
		return;
	}
	
	
	//Everything went fine, start drawing the image
	QPainter painter( this );
	if( viewbox_handler->scale() <= 1.5 || S(settings).smooth_scaling() )
		painter.setRenderHints( QPainter::SmoothPixmapTransform, true );
	
	painter.drawImage( viewbox_handler->area(), get_frame() );
}

QSize imageViewer::sizeHint() const{
	if( !image_cache )
		return QSize();
	
	auto orient = orientation.add( image_cache->get_orientation() );
	QSize size = orient.finalSize( image_cache->size_hint() );
	if (size.isEmpty())
		return QSize();
	
	return size.expandedTo( viewbox_handler->size() );
}


void imageViewer::mousePressEvent( QMouseEvent *event ){
	viewbox_handler->mousePressEvent( event );
}


void imageViewer::mouseDoubleClickEvent( QMouseEvent *event ){
	viewbox_handler->mouseDoubleClickEvent( event );
}

void imageViewer::mouseMoveEvent( QMouseEvent *event ){
	viewbox_handler->mouseMoveEvent( event );
}


void imageViewer::mouseReleaseEvent( QMouseEvent *event ){
	viewbox_handler->mouseReleaseEvent( event );
}

void imageViewer::wheelEvent( QWheelEvent *event ){
	int amount = event->angleDelta().y() / 8;
	if( amount == 0 )
		return;
	amount = ( amount > 0 ) ? 1 : -1;
	
	if( event->modifiers() & Qt::ControlModifier ){
		//Change current frame
		goto_frame( current_frame + amount );
	}
	else //Change zoom-level
		viewbox_handler->change_zoom( viewbox_handler->zoom_level() + amount, event->position().toPoint() );
}

void imageViewer::create_drag_and_drop(){
	if( image_cache ){
		QDrag *drag = new QDrag( this );
		QMimeData *mimeData = new QMimeData;
		
		mimeData->setText( image_cache->url.toString() );
		mimeData->setUrls( QList<QUrl>() << image_cache->url );
		mimeData->setImageData( get_frame() );

		drag->setMimeData( mimeData );
		drag->exec( Qt::CopyAction | Qt::MoveAction );
	}
}


