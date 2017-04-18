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
#include <QDebug>

//True if img is paletted
bool isIndexed( const QImage& img )
	{ return img.format() == QImage::Format_Indexed8; }

QImage AnimCombiner::combineIndexed( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose ){
	//Bail if they are not indexed
	if( !isIndexed( previous ) || !isIndexed( new_image ) )
		return {};
	
	//Bail on different colorTables
	if( previous.colorTable() != new_image.colorTable() )
		return {};
	//TODO: Try to merge them if possible
	
	//Merge the two images
	auto output = previous;
	//TODO: Limit range
	//TODO: Do alpha blending
	//TODO: Get alpha pixel index!
	for( int iy=0; iy<new_image.height(); iy++ ){
		auto in = new_image.scanLine( iy );
		auto out = output.scanLine( iy+y );
		for( int ix=0; ix<new_image.width(); ix++ )
			out[ix+x] = in[ix];
	}
	
	return output;
}

QImage AnimCombiner::combine( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose ){
	if( previous.isNull() ){
		previous = QImage( new_image.size(), new_image.format() );
		previous.setColorTable( new_image.colorTable() );
		previous.fill( 0 );
	}
	
	//Try to see if we can merge it indexed
	auto tryIndexed = combineIndexed( new_image, x, y, blend, dispose );
	if( !tryIndexed.isNull() )
		return tryIndexed;
	qDebug( "indexed combination failed" );
	
	//QPainter can't handle indexed images, convert to something it can handle
	auto fixFormat = []( QImage& img ){
			if( isIndexed( img ) )
				img = img.convertToFormat( QImage::Format_ARGB32 );
		};
	fixFormat( new_image );
	fixFormat( previous );
	
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
