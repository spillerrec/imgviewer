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

#include <QPoint>
#include <QSize>
#include <QRect>

#include <QPainter>
#include <QImage>
#include <QStaticText>
#include <QBrush>
#include <QPen>
#include <QColor>

#include <QTimer>

#include <QMouseEvent>
#include <QWheelEvent>

#include <cmath>
#include <algorithm>


imageViewer::imageViewer( const QSettings& settings, QWidget* parent ): QWidget( parent ), settings( settings ){
	image_cache = NULL;
	frame_amount = 0;
	current_frame = -1;
	loop_counter = 0;
	continue_animating = false;
	waiting_on_frame = -1;
	
	//Auto scale default settings
	auto_scale_on = true;
	auto_aspect_ratio = settings.value( "viewer/aspect_ration", true ).toBool();
	auto_downscale_only = settings.value( "viewer/downscale", true ).toBool();
	auto_upscale_only = settings.value( "viewer/upscale", false ).toBool();
	current_scale = 1;
	
	button_rleft = translate_button( "mouse/rocker-left", 'L' );
	button_rright = translate_button( "mouse/rocker-right", 'R' );
	button_drag = translate_button( "mouse/dragging", 'L' );
	button_double = translate_button( "mouse/double-click", 'L' );
	button_scaling = translate_button( "mouse/cycle-scales", 'M' );
	button_context = translate_button( "mouse/context-menu", 'R' );
	
	shown_pos = QPoint( 0,0 );
	shown_zoom_level = 0;
	
	time = new QTimer( this );
	time->setSingleShot( true );
	connect( time, SIGNAL( timeout() ), this, SLOT( next_frame() ) );
	
	mouse_active = Qt::NoButton;
	multi_button = false;
	is_zooming = false;
	
	setContextMenuPolicy( Qt::PreventContextMenu );
}


bool imageViewer::can_animate(){
	return image_cache ? image_cache->is_animated() : false;
}

bool imageViewer::moveable() const{
	QImage *frame = image_cache ? image_cache->frame( current_frame ) : NULL;
	if( !frame )
		return false;
	
	QSize img = frame->size() * current_scale;
	return ( img.width() > size().width() ) || ( img.height() > size().height() );
}

QSize imageViewer::frame_size(){
	if( !image_cache || image_cache->loaded() < 1 )
		return QSize( 0,0 );
	
	if( image_cache->is_animated() )
		return image_cache->frame( 0 )->size(); //Just return the first frame
	else{
		//Iterate over all frames and find the largest
		QSize combined;
		for( int i=0; i<image_cache->loaded(); i++ )
			combined = combined.expandedTo( image_cache->frame( i )->size() );
		
		return combined;
	}
}

void imageViewer::next_frame(){
	if( !image_cache )
		return;
	
	if( image_cache->loaded() < frame_amount && current_frame+1 >= image_cache->loaded() ){
		//Wait for frame to be available
		waiting_on_frame = current_frame + 1;
		return;
	}
	
	if( frame_amount < 1 )
		return;	//Not loaded
	
	//Go to next frame
	current_frame++;
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


void imageViewer::goto_next_frame(){
	time->stop();
	continue_animating = false;
	next_frame();
}


void imageViewer::prev_frame(){
	if( current_frame < 0 || frame_amount < 2 )
		return;
	
	if( current_frame > 0 )
		current_frame--;
	else	//current_frame == 0
		current_frame = frame_amount - 1;
	
	update();
	emit image_changed();
}


void imageViewer::goto_prev_frame(){
	time->stop();
	continue_animating = false;
	prev_frame();
}

void imageViewer::restart_animation(){
	continue_animating = can_animate();
	current_frame = -1;
	next_frame();
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
	
	return continue_animating;
}


void imageViewer::auto_scale( QSize img ){
	using namespace std;
	
	QSize widget = size();
	
	if( auto_scale_on ){
		double scaling_x = (double) widget.width() / img.width();
		double scaling_y = (double) widget.height() / img.height();
		double img_aspect = (double) img.width() / img.height();
		
		//Prevent certain types of scaling
		if( auto_downscale_only ){
			scaling_x = min( scaling_x, 1.0 );
			scaling_y = min( scaling_y, 1.0 );
		}
		if( auto_upscale_only ){
			scaling_x = max( scaling_x, 1.0 );
			scaling_y = max( scaling_y, 1.0 );
		}
		
		//Keep aspect ratio
		if( auto_aspect_ratio ){
			int res_width = scaling_x * img.width();
			int res_height = scaling_y * img.height();
			double new_aspect = (double)res_width / res_height;
			
			if( img_aspect < new_aspect )
				scaling_x = scaling_y;
			else
				scaling_y = scaling_x;
		}
		
		//Apply scaling
		shown_size.setWidth( scaling_x * img.width() + 0.5 );
		shown_size.setHeight( scaling_y * img.height() + 0.5 );
		current_scale = scaling_x; //prevents it to scale in both dimensions!
		
		//Position image
		QSize temp = widget - shown_size;
		shown_pos = QPoint( temp.width() / 2.0 + 0.5, temp.height() / 2.0 + 0.5 );
		
		if( current_scale == 1.0 )
			shown_zoom_level = 0; //Division by zero
		else
			shown_zoom_level = ( log2( current_scale ) / log2( 2 ) ) * 2.0;
		
		setCursor( Qt::ArrowCursor );
	}
	else{
		//Calculate zoom to 2^(x/2)
		current_scale = pow( 2.0, shown_zoom_level * 0.5 );
		
		QPoint before = image_pos( img, keep_on );
		shown_size = img * current_scale;
		
		if( keep_on != QPoint( 0,0 ) ){
			QPoint after = image_pos( img, keep_on );
			shown_pos -= (before - after) * current_scale;
		}
		
		//Make sure it doesn't leave the screen
		QSize diff = widget - shown_size;
		if( diff.width() >= 0 )
			shown_pos.setX( diff.width() / 2.0 + 0.5 );
		else{
			shown_pos.setX( min( 0, shown_pos.x() ) );
			if( shown_pos.x() + shown_size.width() < widget.width() )
				shown_pos.setX( widget.width() - shown_size.width() );
		}
		
		if( diff.height() >= 0 )
			shown_pos.setY( diff.height() / 2.0 + 0.5 );
		else{
			shown_pos.setY( min( 0, shown_pos.y() ) );
			if( shown_pos.y() + shown_size.height() < widget.height() )
				shown_pos.setY( widget.height() - shown_size.height() );
		}
		
		//Show hand-cursor if image can be moved/is being moved
		if( shown_size.width() <= widget.width() && shown_size.height() <= widget.height() )
			setCursor( Qt::ArrowCursor );
		else{
			if( mouse_active & Qt::LeftButton )
				setCursor( Qt::ClosedHandCursor );
			else
				setCursor( Qt::OpenHandCursor );
		}
	}
	
	keep_on = QPoint( 0,0 );
}



void imageViewer::read_info(){
	frame_amount = image_cache->frame_count();
	loop_counter = image_cache->loop_count();
	continue_animating = image_cache->is_animated();
	
	current_frame = -1;
	emit image_info_read();
	next_frame();
}
void imageViewer::check_frame( unsigned int idx ){
	//TODO: add code to check if needed to update
	if( waiting_on_frame <= -1 )
		return;
	
	if( idx == (unsigned int)waiting_on_frame ){
		waiting_on_frame = -1;
		next_frame();
	}
}


void imageViewer::change_image( imageCache *new_image, bool delete_old ){
	if( image_cache ){
		if( delete_old )
			delete image_cache;
		else
			disconnect( image_cache, 0, this, 0 );
	}
	
	image_cache = new_image;
	waiting_on_frame = -1;
	current_frame = -1;
	frame_amount = 0;
	
	shown_pos = QPoint( 0,0 );
	shown_zoom_level = 0;
	current_scale = 1;
	
	if( image_cache ){
		switch( image_cache->get_status() ){
			case imageCache::INVALID:	break; //Loading failed
			
			//TODO: remove those connections again
			case imageCache::EMPTY:
					connect( image_cache, SIGNAL( info_loaded() ), this, SLOT( read_info() ) );
					connect( image_cache, SIGNAL( frame_loaded(unsigned int) ), this, SLOT( check_frame(unsigned int) ) );
				break;
			
			case imageCache::INFO_READY:
			case imageCache::FRAMES_READY:
					connect( image_cache, SIGNAL( frame_loaded(unsigned int) ), this, SLOT( check_frame(unsigned int) ) );
					read_info();
				break;
			
			case imageCache::LOADED:
					read_info();
				break;
		}
		
		update();
		emit image_changed();
	}
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
	static QStaticText txt_error( tr( "Unspecified error" ) );
	QSize current_size = size();
	
	//Start checking for errors
	
	if( !image_cache || image_cache->get_status() == imageCache::EMPTY ){
		//We have nothing to display
		draw_message( &txt_no_image );
		return;
	}
	
	if( image_cache->get_status() == imageCache::INVALID ){
		//Image is currently loading
		draw_message( &txt_invalid );
		return;
	}
	if( current_frame < 0 || current_frame >= image_cache->loaded() ){
		//Image is currently loading
		draw_message( &txt_loading );
		return;
	}
	
	if( !current_size.isValid() ){
		draw_message( &txt_error );
		return;
	}
	
	
	//Everything when fine, start drawing the image
	QImage *frame = image_cache->frame( current_frame );
	QSize img_size = frame->size();
	auto_scale( img_size );
	
	QPainter painter( this );
	if( img_size.width()*1.5 >= shown_size.width() )
		painter.setRenderHints( QPainter::SmoothPixmapTransform, true );
	
	painter.drawImage( QRect( shown_pos, shown_size ), *frame );
}

QSize imageViewer::sizeHint() const{
	if( image_cache && frame_amount > 0 )
		return image_cache->frame( 0 )->size();
	
	return QSize();
}


QPoint imageViewer::image_pos( QSize img_size, QPoint pos ){
	QPoint aligned = pos - shown_pos;
	aligned.setX( aligned.x() * img_size.width() / shown_size.width() );
	aligned.setY( aligned.y() * img_size.height() / shown_size.height() );
	return aligned;
}

Qt::MouseButton imageViewer::translate_button( const char* name, char fallback ) const{
	QString s = settings.value( name ).toString();
	
	QChar c( fallback );
	if( s.count() > 0 )
		c = s[0];
	
	switch( c.unicode() ){
		case 'l':
		case 'L': return Qt::LeftButton;
		case 'm':
		case 'M': return Qt::MidButton;
		case 'r':
		case 'R': return Qt::RightButton;
		case '1': return Qt::ExtraButton1;
		case '2': return Qt::ExtraButton2;
		case '3': return Qt::ExtraButton3;
		case '4': return Qt::ExtraButton4;
		case '5': return Qt::ExtraButton5;
		case '6': return Qt::ExtraButton6;
		case '7': return Qt::ExtraButton7;
		case '8': return Qt::ExtraButton8;
		case '9': return Qt::ExtraButton9;
		case 'a':
		case 'A': return Qt::AllButtons;
		default: return Qt::NoButton;
	}
}

void imageViewer::mousePressEvent( QMouseEvent *event ){
	emit clicked();
	
	//Rocker gestures
	if( mouse_active & ( button_rleft | button_rright ) ){
		mouse_active |= event->button();
		multi_button = true;
		unsigned current = event->button() & event->buttons();
		
		if( current == button_rleft ){
			if( event->modifiers() & Qt::ControlModifier )
				goto_prev_frame();
			else
				emit rocker_left();
		}
		else if( current == button_rright ){
			if( event->modifiers() & Qt::ControlModifier )
				goto_next_frame();
			else
				emit rocker_right();
		}
		return;
	}
	
	mouse_active |= event->button();
	mouse_last_pos = event->pos();
	
	//Change cursor when dragging
	if( event->button() == button_drag )
		if( moveable() )
			setCursor( Qt::ClosedHandCursor );
}


void imageViewer::mouseDoubleClickEvent( QMouseEvent *event ){
	//Only emit if only LeftButton is pressed, and no other buttons
	if( !( event->buttons() & ~button_double ) ){
		if( event->modifiers() & Qt::ControlModifier )
			toogle_animation();
		else
			emit double_clicked();
	}
}


void imageViewer::mouseMoveEvent( QMouseEvent *event ){
	if( !(mouse_active & button_drag) )
		return;
	
	if( event->modifiers() & Qt::ControlModifier ){
		if( !is_zooming ){
			is_zooming = true;
			start_zoom = shown_zoom_level;
		}
		
		auto_scale_on = false;
		QPoint diff = mouse_last_pos - event->pos();
		shown_zoom_level = start_zoom + diff.y() * 0.05;
		keep_on = mouse_last_pos;
	}
	else{
		if( is_zooming ){
			is_zooming = false;
			mouse_last_pos = event->pos();
		}
		
		shown_pos -= mouse_last_pos - event->pos();
		mouse_last_pos = event->pos();
	}
	
	update();
}


void imageViewer::mouseReleaseEvent( QMouseEvent *event ){
	setCursor( ( (mouse_active & button_drag) && moveable() ) ? Qt::OpenHandCursor : Qt::ArrowCursor );
	
	//If only one button was pressed
	if( !multi_button ){
		if( event->button() == button_scaling ){
			//Cycle through scalings
			if( auto_scale_on ){
				auto_scale_on = false;
				shown_zoom_level = 0;
			}
			else{
				if( !moveable() )
					shown_zoom_level = 0;
				else
					auto_scale_on = true;
			}
			
			keep_on = event->pos();
			update();
		}
		else if( event->button() == button_context ){
			//Open context menu
			create_context_event( *event );
		}
	}
	
	//Revert properties
	mouse_active &= ~event->button();
	is_zooming = false;
	if( !mouse_active ) //Last button released
		multi_button = false;
}

void imageViewer::create_context_event( const QMouseEvent& event ){
	emit context_menu( QContextMenuEvent(
			QContextMenuEvent::Mouse
		,	event.pos()
		,	event.globalPos()
		,	event.modifiers()
		) );
}

void imageViewer::wheelEvent( QWheelEvent *event ){
	int amount = event->delta() / 8;
	if( amount == 0 )
		return;
	
	if( event->modifiers() & Qt::ControlModifier ){
		//Change current frame
		if( amount > 0 )
			goto_next_frame();
		else
			goto_prev_frame();
	}
	else{
		//Change zoom-level
		shown_zoom_level += (amount > 0) ? 1.0 : -1.0;
		auto_scale_on = false;
		
		keep_on = event->pos();
		update();
	}
}




