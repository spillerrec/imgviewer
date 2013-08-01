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

class imageCache;

class QStaticText;
class QPaintEvent;
class QResizeEvent;

class imageViewer: public QWidget{
	Q_OBJECT
	
	private:
		imageCache *image_cache;
		int frame_amount;
		int current_frame;
		int loop_counter;
		bool continue_animating;
		int waiting_on_frame;
	public:
		int get_frame_amount(){ return frame_amount; }
		int get_current_frame(){ return current_frame; }
		bool can_animate();
		bool is_animating(){ return continue_animating; }
		QSize frame_size();
	
	//How the image is to be viewed
	private:
		QPoint shown_pos;
		QSize shown_size;
		double shown_zoom_level;
		QColor background;
	public:
		void set_background_color( QColor new_color ){ background = new_color; update(); }
		bool moveable() const;
		
	//Settings to autoscale
	private:
		bool auto_scale_on;
		bool auto_aspect_ratio;
		bool auto_downscale_only;
		bool auto_upscale_only;
		double current_scale;
	private slots:
		void auto_scale( QSize img );
	//Access methods
	public:
		void set_auto_scale( bool is_on ){ auto_scale_on = is_on; }
		void set_auto_aspect( bool is_on ){ auto_aspect_ratio = is_on; }
		void set_auto_scaling( bool upscales, bool downscales ){
			auto_downscale_only = !upscales;
			auto_upscale_only = !downscales;
		}
		
	private:
		QTimer *time;
		
	private slots:
		void read_info();
		void check_frame( unsigned int idx );
	private slots:
		void next_frame();
		void prev_frame();
	public slots:
		void goto_next_frame();
		void goto_prev_frame();
		bool toogle_animation();
	
	protected:
		void draw_message( QStaticText *text );
		void paintEvent( QPaintEvent *event );
	//	void resizeEvent( QResizeEvent *event );
	//Controlling mouse actions
	
	protected:
		Qt::MouseButtons mouse_active;
		bool multi_button;
		bool is_zooming;
		double start_zoom;
		QPoint mouse_last_pos;
		QPoint keep_on;
		QPoint image_pos( QSize img, QPoint pos );
		
		void mousePressEvent( QMouseEvent *event );
		void mouseMoveEvent( QMouseEvent *event );
		void mouseDoubleClickEvent( QMouseEvent *event );
		void mouseReleaseEvent( QMouseEvent *event );
		void wheelEvent( QWheelEvent *event );
	
	public:
		explicit imageViewer( QWidget* parent = 0 );
		
		void change_image( imageCache *new_image, bool delete_old = true );
		
		
		QSize sizeHint() const;
	
	signals:
		void image_info_read();
		void image_changed();
		void double_clicked();
		void rocker_left();
		void rocker_right();
};


#endif