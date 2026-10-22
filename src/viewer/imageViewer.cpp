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
#include "AnimationHandler.h"
#include "QImagePainter.h"
#include "imageCache.h"
#include "settings/ViewerSettings.h"
#include <qobject.h>

#include <QPoint>
#include <QSize>
#include <QRect>

#include <QWheelEvent>

#include <QDrag>
#include <QMimeData>


using S = ViewerSettings;


imageViewer::imageViewer( QSettings& settings, QWidget* parent ): QWidget( parent ), settings( settings ){
	//User settings
	initial_resize = S(settings).initial_resize();
	
	animation_handler = std::make_unique<AnimationHandler>( this );
	painter = std::make_unique<QImagePainer>( this );
	connect( animation_handler.get(), &AnimationHandler::frame_changed, this, &imageViewer::on_frame_changed );
	
	viewbox_handler = std::make_unique<viewboxHandler>( settings, this );
	connect( viewbox_handler.get(), &viewboxHandler::double_clicked, animation_handler.get(), &AnimationHandler::goto_next_frame );
	connect( viewbox_handler.get(), &viewboxHandler::rocker_left,    animation_handler.get(), &AnimationHandler::goto_prev_frame );
	connect( viewbox_handler.get(), &viewboxHandler::rocker_right,   animation_handler.get(), &AnimationHandler::goto_next_frame );
	connect( viewbox_handler.get(), &viewboxHandler::toogle_animation, animation_handler.get(),  &AnimationHandler::toggle_animation );
	connect( viewbox_handler.get(), &viewboxHandler::create_drag_and_drop, this, &imageViewer::create_drag_and_drop );
	connect( viewbox_handler.get(), &viewboxHandler::goto_next_frame,  animation_handler.get(), &AnimationHandler::goto_next_frame );
	connect( viewbox_handler.get(), &viewboxHandler::goto_prev_frame,  animation_handler.get(), &AnimationHandler::goto_prev_frame );
	connect( viewbox_handler.get(), &viewboxHandler::viewbox_changed,  this, [this]{ update(); } );
	
	setContextMenuPolicy( Qt::PreventContextMenu );
}

void imageViewer::rotate( int8_t amount ){
	orientation = orientation.rotate( amount );
	update_viewbox_size();
	update();
}

void imageViewer::mirror( bool hor, bool ver ){
	orientation = orientation.mirror( hor, ver );
	update();
}

QImage imageViewer::get_frame(){
	int frame_idx = animation_handler->get_current_frame();
	return painter->get_frame( frame_idx, orientation, screen() );
}

void imageViewer::update_viewbox_size() {
	int index = animation_handler->get_current_frame();
	if( image_cache && index < image_cache->loaded() ){
		auto orient = orientation.add(image_cache->get_orientation());
		viewbox_handler->set_new_frame_size( orient.finalSize( image_cache->frame(index).size() ) );
	}
	else
		viewbox_handler->set_new_frame_size( {} );
}

void imageViewer::read_info(){
	if( !image_cache ) return;
	animation_handler->init_from_cache();
	emit image_info_read();
}

void imageViewer::on_frame_changed( int new_frame ){
	update();
	emit image_changed();
}

void imageViewer::init_size(){
	//TODO: customize
	if( initial_resize )
		emit resize_wanted();
	initial_resize = S(settings).keep_resize();
	
	animation_handler->goto_frame( 0 );
	animation_handler->restart_animation();
	update_viewbox_size();
}


void imageViewer::change_image( std::shared_ptr<imageCache> new_image ){
	if( new_image == image_cache )
		return;
	
	image_cache = std::move(new_image);
	animation_handler->set_image_cache( image_cache );
	painter->change_image( image_cache );
	
	if( image_cache ){
		switch( image_cache->get_status() ){
			case imageCache::INVALID:	break; //Loading failed
			
			//TODO: remove those connections again
			case imageCache::EMPTY:
					connect( image_cache.get(), &imageCache::info_loaded, this, &imageViewer::read_info );
					connect( image_cache.get(), &imageCache::frame_loaded, animation_handler.get(), &AnimationHandler::check_frame_loaded );
					connect( image_cache.get(), &imageCache::frame_loaded, this, [this](int idx){ if (idx==0) this->init_size(); } );
				break;
			
			case imageCache::INFO_READY:
			case imageCache::FRAMES_READY:
					connect( image_cache.get(), &imageCache::info_loaded, this, &imageViewer::read_info );
					connect( image_cache.get(), &imageCache::frame_loaded, animation_handler.get(), &AnimationHandler::check_frame_loaded );
					connect( image_cache.get(), &imageCache::frame_loaded, this, [this](int idx){ if (idx==0) this->init_size(); } );
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


void imageViewer::paintEvent( QPaintEvent* ){
	bool smooth_scaling = viewbox_handler->scale() <= 1.5 || S(settings).smooth_scaling();
	painter->paintEvent( this, animation_handler->get_current_frame(), viewbox_handler->area(), orientation, smooth_scaling);
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
		animation_handler->goto_frame( animation_handler->get_current_frame() + amount );
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


