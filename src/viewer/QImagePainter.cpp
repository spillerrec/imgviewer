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

#include "QImagePainter.h"
#include "imageCache.h"
#include "colorManager.h"
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



void QImagePainer::updateOrientation( Orientation wanted, Orientation current ){
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
	converted_orientation = wanted;
}

QImage QImagePainer::get_frame( int frame_idx, Orientation orientation, QScreen* current_monitor ){
	if( !image_cache || frame_idx >= image_cache->loaded() )
		return {};

	bool frameIsCached = converted_frame_idx == frame_idx;
	
	if( converted_monitor != current_monitor || !frameIsCached ){
		//Cache invalid, refresh
		converted = image_cache->frame( frame_idx );
		
		//Transform colors to current monitor profile
		image_cache->get_manager()->doTransform( converted, image_cache->get_profile(), current_monitor );
		converted_monitor = current_monitor;

		updateOrientation( orientation.add(image_cache->get_orientation()), {} ); // Would make more sense to reset orientation to correct value, then call updateOrientation afterwards, but usure about the logic here
	}
	else
		updateOrientation( orientation, converted_orientation );
	
	return converted;
}

void QImagePainer::change_image( std::shared_ptr<imageCache> new_image ){
	if( new_image == image_cache )
		return;
	
	image_cache = std::move(new_image);
	clear_converted();
}


void QImagePainer::draw_message( QWidget* widget, QStaticText *text ){
	text->prepare();	//Make sure it has calculated the size
	QSizeF txt_size = text->size();
	int x = ( widget->size().width() - txt_size.width() ) * 0.5;
	int y = ( widget->size().height() - txt_size.height() ) * 0.5;
	
	QPainter painter( widget );
	//Prepare drawing
	painter.setRenderHints( QPainter::Antialiasing, true );
	painter.setBrush( QBrush( QColor( Qt::gray ) ) );
	painter.setPen( QPen( Qt::NoPen ) );
	
	//Draw the background, box and text
	painter.drawRoundedRect( x-10,y-10, txt_size.width()+20, txt_size.height()+20, 10, 10 );
	painter.setPen( QPen( Qt::black ) );
	painter.drawStaticText( x,y, *text );
}


void QImagePainer::paintEvent( QWidget* widget, int frame_idx, QRect area, Orientation orientation, bool smooth_scaling ){
	static QStaticText txt_loading( tr( "Loading" ) );
	static QStaticText txt_no_image( tr( "No image selected" ) );
	static QStaticText txt_invalid( tr( "Image invalid or broken!" ) );
	
	//Start checking for errors
	
	if( !image_cache || image_cache->get_status() == imageCache::EMPTY ){
		//We have nothing to display
		draw_message( widget, &txt_no_image );
		return;
	}
	
	if( image_cache->get_status() == imageCache::INVALID ){
		//Image could not be loaded
		draw_message( widget, &txt_invalid );
		return;
	}
	if( frame_idx >= image_cache->loaded() ){
		//Image is currently loading
		draw_message( widget, &txt_loading );
		return;
	}
	
	
	//Everything went fine, start drawing the image
	QPainter painter( widget );
	if( smooth_scaling )
		painter.setRenderHints( QPainter::SmoothPixmapTransform, true );
	
	painter.drawImage( area, get_frame( frame_idx, orientation, widget->screen() ) );
}
