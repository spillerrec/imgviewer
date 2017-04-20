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

#include <stdexcept>
#include <QPainter>
#include <QDebug>

//True if img is paletted
bool isIndexed( const QImage& img )
	{ return img.format() == QImage::Format_Indexed8; }
	
static void copyIndexedImage( QImage& img_dest, int x, int y, QImage img_src ){
	//TODO: Limit ranges
	for( int iy=0; iy<img_src.height(); iy++ ){
		auto in  = img_src .scanLine( iy   );
		auto out = img_dest.scanLine( iy+y );
		for( int ix=0; ix<img_src.width(); ix++ )
			out[ix+x] = in[ix];
	}
}
	
static void overlayIndexedImage( QImage& img_dest, int x, int y, QImage img_src, int replace ){
	//Just copy if replace id is out of range
	if( replace < 0 || replace > 255 ){
		copyIndexedImage( img_dest, x, y, img_src );
		return;
	}
	
	//TODO: Limit ranges
	for( int iy=0; iy<img_src.height(); iy++ ){
		auto in  = img_src .scanLine( iy   );
		auto out = img_dest.scanLine( iy+y );
		for( int ix=0; ix<img_src.width(); ix++ )
			out[ix+x] = (in[ix] != replace) ? in[ix] : out[ix+x];
	}
}

QImage AnimCombiner::combineIndexed( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose, int indexed_background ){
	//Bail if they are not indexed
	if( !isIndexed( previous ) || !isIndexed( new_image ) )
		return {};
	
	//Bail on different colorTables
	//TODO: Try to merge them if possible
	if( previous.colorTable() != new_image.colorTable() ){
		qDebug( "Color tables do not match, reverting to RGB mode. Send a sample to the devs so this can be optimized." );
		return {};
	}
	
	//Merge the two images
	auto output = previous;
	switch( blend ){
		case BlendMode::REPLACE:    copyIndexedImage( output, x, y, new_image ); break;
		case BlendMode::OVERLAY: overlayIndexedImage( output, x, y, new_image, indexed_background ); break;
		default: throw std::runtime_error( "Missing blend mode" );
	};
	
	//Update 'previous' image
	switch( dispose ){
		case DisposeMode::NONE: previous = output; break;
		case DisposeMode::BACKGROUND:{
				//TODO: Fill rectangle with background color
				//TODO: We don't have the background color!!
				qDebug( "Indexed dispose background not yet done" );
			} break;
		case DisposeMode::REVERT: break;
		default: throw std::runtime_error( "Missing dispose mode" );
	}
	
	return output;
}

QImage AnimCombiner::combine( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose, int indexed_background ){
	if( previous.isNull() ){
		previous = QImage( new_image.size(), new_image.format() );
		previous.setColorTable( new_image.colorTable() );
		previous.fill( 0 );
	}
	
	//Try to see if we can merge it indexed
	auto tryIndexed = combineIndexed( new_image, x, y, blend, dispose, indexed_background );
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
