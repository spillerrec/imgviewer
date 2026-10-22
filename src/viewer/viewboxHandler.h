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

#ifndef VIEWBOX_HANDLER_HPP
#define VIEWBOX_HANDLER_HPP

#include <QWidget>
#include <QColor>
#include <QSettings>
#include <QContextMenuEvent>

#include <qnamespace.h>

#include "ZoomBox.hpp"

class imageCache;

class viewboxHandler : public QObject{
	Q_OBJECT
	
	private:
		ZoomBox zoom;
		QSettings& settings;
		QSize view_size; ///< Widget size
		Qt::CursorShape current_cursor_shape = Qt::ArrowCursor;
		
		Qt::MouseButtons mouse_active{ Qt::NoButton };
		bool multi_button{ false };
		bool is_zooming{ false };
		double start_zoom{ 1.0 };
		QPoint mouse_last_pos;
	
		Qt::MouseButton button_rleft;
		Qt::MouseButton button_rright;
		Qt::MouseButton button_drag;
		Qt::MouseButton button_double;
		Qt::MouseButton button_scaling;
		Qt::MouseButton button_context;

		
		bool auto_scale_on{ true };
		void restrict_view( bool force=false );
		void auto_zoom( QSize size );

		void setCursor( Qt::CursorShape shape );
	
	public:
	
		viewboxHandler( QSettings& settings, QObject* parent );
		
		
		Qt::MouseButton translate_button( const QSettings& settings, const char* name, char fallback ) const;
		
		void update_cursor();
		
		void mousePressEvent( QMouseEvent *event );
		void mouseMoveEvent( QMouseEvent *event );
		void mouseDoubleClickEvent( QMouseEvent *event );
		void mouseReleaseEvent( QMouseEvent *event );

		Qt::MouseButton get_context_button() const{ return button_context; }
		void create_context_event( const QMouseEvent& event );
		
		void change_zoom( double new_level, QPoint keep_on );
		
		bool auto_zoom_active() const{ return auto_scale_on; }

		void set_new_frame_size(QSize frameSize);

		auto scale() const { return zoom.scale(); }
		auto area() const { return zoom.area(); }
		auto size() const { return zoom.size(); }
		auto zoom_level() const { return zoom.level(); }

	signals:
		void double_clicked();
		void rocker_left();
		void rocker_right();
		void clicked();
		void context_menu( QContextMenuEvent& event );
		void goto_next_frame();
		void goto_prev_frame();
		void viewbox_changed();
		void toogle_animation();

		void create_drag_and_drop();
		
	public slots:
		void updateView();
		void setWidgetSize( QSize );
};

#endif
