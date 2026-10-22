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

#include "viewboxHandler.h"
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


viewboxHandler::viewboxHandler( QSettings& settings, QObject* parent ) : QObject(parent), settings(settings){
	
	button_rleft   = translate_button( settings, "mouse/rocker-left",  'L' );
	button_rright  = translate_button( settings, "mouse/rocker-right", 'R' );
	button_drag    = translate_button( settings, "mouse/dragging",     'L' );
	button_double  = translate_button( settings, "mouse/double-click", 'L' );
	button_scaling = translate_button( settings, "mouse/cycle-scales", 'M' );
	button_context = translate_button( settings, "mouse/context-menu", 'R' );
	
}

Qt::MouseButton viewboxHandler::translate_button( const QSettings& settings, const char* name, char fallback ) const{
	QString s = settings.value( name ).toString();
	
	QChar c( fallback );
	if( s.size() > 0 )
		c = s[0];
	
	switch( c.unicode() ){
		case 'l':
		case 'L': return Qt::LeftButton;
		case 'm':
		case 'M': return Qt::MiddleButton;
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


void viewboxHandler::setCursor( Qt::CursorShape shape )
{
	if (shape != current_cursor_shape){
		current_cursor_shape = shape;
		// TODO: Signal change
	}
}


void viewboxHandler::update_cursor(){
	//Show hand-cursor if image can be moved/is being moved
	if( zoom.moveable( view_size ) )
		setCursor( Qt::ArrowCursor );
	else{
		if( mouse_active & Qt::LeftButton )
			setCursor( Qt::ClosedHandCursor );
		else
			setCursor( Qt::OpenHandCursor );
	}
}


void viewboxHandler::mousePressEvent( QMouseEvent *event ){
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
		if( zoom.moveable( view_size ) )
			setCursor( Qt::ClosedHandCursor );
}


void viewboxHandler::mouseDoubleClickEvent( QMouseEvent *event ){
	//Only emit if only LeftButton is pressed, and no other buttons
	if( !( event->buttons() & ~button_double ) ){
		if( event->modifiers() & Qt::ControlModifier )
			emit toogle_animation();
		else
			emit double_clicked();
	}
	else
		mousePressEvent( event ); //TODO: Test on windows
}

void viewboxHandler::mouseMoveEvent( QMouseEvent *event ){
	if( mouse_active & button_context ){
		emit create_drag_and_drop();
		mouse_active = Qt::NoButton;
		return;
	}
	else if( !(mouse_active & button_drag) )
		return;
	
	if( event->modifiers() & Qt::ControlModifier ){
		if( !is_zooming ){
			is_zooming = true;
			start_zoom = zoom.level();
		}
		
		QPoint diff = mouse_last_pos - event->pos();
		change_zoom( start_zoom + diff.y() * 0.05, mouse_last_pos );
	}
	else{
		if( is_zooming ){
			is_zooming = false;
			mouse_last_pos = event->pos();
		}
		
		if( zoom.move( event->pos() - mouse_last_pos ) ){
			restrict_view();
			emit viewbox_changed();
		}
		mouse_last_pos = event->pos();
	}
}


void viewboxHandler::mouseReleaseEvent( QMouseEvent *event ){
	setCursor( ( (mouse_active & button_drag) && zoom.moveable( view_size) ) ? Qt::OpenHandCursor : Qt::ArrowCursor );
	
	//If only one button was pressed
	if( !multi_button ){
		if( event->button() == button_scaling ){
			//Cycle through scalings
			if( auto_scale_on || !zoom.moveable(view_size) )
				change_zoom( 0, event->pos() );
			else{
				auto_zoom( view_size );
				auto_scale_on = true;
			}
		}
		else if( event->button() == button_context ){
			//Open context menu
			mouse_active &= ~event->button(); //Prevents rocker-gestures from activating
			create_context_event( *event );
		}
	}
	
	//Revert properties
	mouse_active &= ~event->button();
	is_zooming = false;
	if( !mouse_active ) //Last button released
		multi_button = false;
}

void viewboxHandler::create_context_event( const QMouseEvent& event ){
	QContextMenuEvent new_event(
			QContextMenuEvent::Mouse
		,	event.pos()
		,	event.globalPosition().toPoint()
		,	event.modifiers()
		);
	emit context_menu( new_event );
}


void viewboxHandler::change_zoom( double new_level, QPoint keep_on ){
	auto_scale_on = false;
	
	if( zoom.change_scale( new_level, keep_on ) ){
		restrict_view();
		emit viewbox_changed();
	}
	update_cursor();
}

void viewboxHandler::set_new_frame_size(QSize frameSize){
	if( zoom.change_content( frameSize, true ) )
		updateView();
}

using S = ViewerSettings;

void viewboxHandler::restrict_view( bool force ){
	if( S(settings).restrict_viewpoint() || force )
		zoom.restrict( view_size );
}

void viewboxHandler::auto_zoom( QSize size ){
	zoom.resize(
			size
		,	S( settings ).auto_downscale_only()
		,	S( settings ).auto_upscale_only()
		,	S( settings ).auto_aspect_ratio()
		);
	emit viewbox_changed();
	update_cursor();
}

void viewboxHandler::updateView(){
	if( auto_scale_on )
		auto_zoom( view_size );
	else
		restrict_view();
}

void viewboxHandler::setWidgetSize( QSize widget_size ){
	view_size = widget_size;
}


