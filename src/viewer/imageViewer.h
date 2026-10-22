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

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QWidget>
#include <QColor>
#include <QSettings>
#include <QContextMenuEvent>

#include <memory>
#include <qnamespace.h>

#include "Orientation.hpp"
#include "viewboxHandler.h"
#include "AnimationHandler.h"
#include "QImagePainter.h"

class imageCache;

class QStaticText;

class imageViewer: public QWidget{
	Q_OBJECT
	
	private:
		std::shared_ptr<imageCache> image_cache;
		std::unique_ptr<AnimationHandler> animation_handler;
		std::unique_ptr<QImagePainer> painter;
	public:
		AnimationHandler* animation() { return animation_handler.get(); }
	
	//How the image is to be viewed
	private:
		Orientation orientation;
		std::unique_ptr<viewboxHandler> viewbox_handler;
		
	//Settings to autoscale
	private:
		bool initial_resize;
		
	private:
		QSettings& settings;
		void init_size();
		
	private slots:
		void read_info();
		void on_frame_changed( int new_frame );
	public slots:
		void rotate( int8_t amount );
		void rotateLeft (){ rotate( -1 ); };
		void rotateRight(){ rotate( +1 ); };
		void mirror( bool hor, bool ver );
		void mirrorHor(){ mirror( true, false ); }
		void mirrorVer(){ mirror( false, true ); }
	
	protected:
		void paintEvent( QPaintEvent* ) override;
		void resizeEvent( QResizeEvent* event ) override {
			viewbox_handler->setWidgetSize( size() );
			viewbox_handler->updateView();
		}
		void update_viewbox_size();
	
	//Controlling mouse actions
	protected:
		void mousePressEvent( QMouseEvent *event ) override;
		void mouseMoveEvent( QMouseEvent *event ) override;
		void mouseDoubleClickEvent( QMouseEvent *event ) override;
		void mouseReleaseEvent( QMouseEvent *event ) override;
		void wheelEvent( QWheelEvent *event ) override;
		
		void create_drag_and_drop();
	
	public:
		explicit imageViewer( QSettings& settings, QWidget* parent = 0 );
		
		void change_image( std::shared_ptr<imageCache> new_image );
		
		Qt::MouseButton get_context_button() const{ return viewbox_handler->get_context_button(); }
		
		bool auto_zoom_active() const{ return viewbox_handler->auto_zoom_active(); }
		QSize sizeHint() const override;
		QImage get_frame();
		
		viewboxHandler* viewbox() { return viewbox_handler.get(); }
	
	signals:
		void image_info_read();
		void resize_wanted();
		void image_changed();
};


#endif
