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

#include "AnimCombiner.hpp"

#include <QPainter>

QImage AnimCombiner::combine( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose ){
	QImage output = previous;
	QPainter painter( &output );
	
	if( blend == BlendMode::OVERLAY )
		painter.setCompositionMode( QPainter::CompositionMode_Source );
	painter.drawImage( x, y, new_image );
	
	switch( dispose ){
		case DisposeMode::NONE: previous = output; break;
		case DisposeMode::BACKGROUND:{
				previous = output;
				QPainter canvas_painter( &previous );
				canvas_painter.setCompositionMode( QPainter::CompositionMode_Source );
				canvas_painter.fillRect( x, y, new_image.width(), new_image.height(), QColor(0,0,0,0) );
			} break;
		case DisposeMode::REVERT: break;
	}
	
	return output;
}
