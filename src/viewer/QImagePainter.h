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

#ifndef QIMAGE_PAINTER_H
#define QIMAGE_PAINTER_H

#include <QWidget>
#include <QColor>
#include <QSettings>
#include <QContextMenuEvent>

#include <memory>
#include <qnamespace.h>
#include <qobject.h>

#include "Orientation.hpp"
#include "viewboxHandler.h"
#include "AnimationHandler.h"

class imageCache;

class QStaticText;

class QImagePainer: public QObject{
	Q_OBJECT
	
	//Color managed cache
	private:
		std::shared_ptr<imageCache> image_cache;
		QImage converted;
		Orientation converted_orientation;
		int converted_frame_idx = -1;
		QScreen* converted_monitor{ nullptr };
		void clear_converted(){
			converted = QImage();
			converted_monitor = nullptr;
		}
		
		void updateOrientation( Orientation wanted, Orientation current );
		void draw_message( QWidget* widget, QStaticText* text );
	
	public:
		explicit QImagePainer( QObject* parent ) : QObject( parent ) { }
		void paintEvent( QWidget* widget, int frame_idx, QRect area, Orientation orientation, bool smooth_scaling );
		
		void change_image( std::shared_ptr<imageCache> new_image );
		
		QImage get_frame( int frame_idx, Orientation orientation, QScreen* current_monitor );
};


#endif
