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

IndexColor::IndexColor( int indexed, const QVector<QRgb>& table ) : hasIndexed(true), indexed(indexed){
	if( indexed >= 0 && indexed < table.size() )
		rgb = table[indexed];
	else
		rgb = qRgba(0,0,0,0);
}

int IndexColor::getIndexed() const{
	if( !hasIndexed )
		throw std::runtime_error( "No indexed color provided!" );
	return indexed;
}

//True if img is paletted
inline bool isIndexed( const QImage& img )
	{ return img.format() == QImage::Format_Indexed8; }


static QSize restrictSize( QSize limit, int x, int y, QSize wanted ){
	//TODO: x/y being negative?
	return {
			std::min( wanted.width(),  limit.width()  - x )
		,	std::min( wanted.height(), limit.height() - y )
		};
}
	
static void copyIndexedImage( QImage& img_dest, int x, int y, QImage img_src ){
	auto size = restrictSize( img_dest.size(), x, y, img_src.size() );
	for( int iy=0; iy<size.height(); iy++ ){
		auto in  = img_src .scanLine( iy   );
		auto out = img_dest.scanLine( iy+y );
		for( int ix=0; ix<size.width(); ix++ )
			out[ix+x] = in[ix];
	}
}
	
static void overlayIndexedImage( QImage& img_dest, int x, int y, QImage img_src, int replace ){
	//Just copy if replace id is out of range
	if( replace < 0 || replace > 255 ){
		copyIndexedImage( img_dest, x, y, img_src );
		return;
	}
	
	auto size = restrictSize( img_dest.size(), x, y, img_src.size() );
	for( int iy=0; iy<size.height(); iy++ ){
		auto in  = img_src .scanLine( iy   );
		auto out = img_dest.scanLine( iy+y );
		for( int ix=0; ix<size.width(); ix++ )
			out[ix+x] = (in[ix] != replace) ? in[ix] : out[ix+x];
	}
}

static void fillIndexedRect( QImage& img_dest, int x, int y, QSize area, int color ){
	auto size = restrictSize( img_dest.size(), x, y, area );
	for( int iy=0; iy<size.height(); iy++ ){
		auto out = img_dest.scanLine( iy+y );
		for( int ix=0; ix<size.width(); ix++ )
			out[ix+x] = color;
	}
}

QImage AnimCombiner::combineIndexed( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose, IndexColor transparent ){
	//Bail if the new image is not indexed
	if( !isIndexed( new_image ) )
		return {};
	
//	qDebug( "Color table size: %d", new_image.colorTable().size() );
//	qDebug( "Blend: %s, transparent id: %d", (blend == BlendMode::REPLACE ? "replace" : "overlay"), transparent.getIndexed() );
	
	//Figure out if we can merge them trivially
	//Previous image must be indexed and color tables must be the same
	bool mergeable = isIndexed( previous ) && previous.colorTable() == new_image.colorTable();
	
	//Do we need to keep contents around the new_image?
	bool completelyOverlaps = QRect( {0, 0}, previous.size() ) == QRect( {x,y}, new_image.size() );
	
	//Do we need to overlay transparent areas with the previous image?
	bool noOverlayNeeded = (blend == BlendMode::OVERLAY) && transparent.getIndexed() < 0;
	
	
	auto output = previous;
	//Avoid any merging if the entire image gets replaced completely
	if( completelyOverlaps && noOverlayNeeded ){
		qDebug( "Skipping merging" );
		output = new_image;
	}
	else if( mergeable ){ //Merge the two images normally
		switch( blend ){
			case BlendMode::REPLACE:    copyIndexedImage( output, x, y, new_image ); break;
			case BlendMode::OVERLAY: overlayIndexedImage( output, x, y, new_image, transparent.getIndexed() ); break;
			default: throw std::runtime_error( "Missing blend mode" );
		};
	}
	else //We will have to merge these in RGB mode
		return {};
	
	//Update 'previous' image
	switch( dispose ){
		case DisposeMode::NONE: previous = output; break;
		case DisposeMode::BACKGROUND:
				fillIndexedRect( previous, x, y, new_image.size(), background_color.getIndexed() );
				//TODO: is buggy if color table changes?
			break;
		case DisposeMode::REVERT: break;
		default: throw std::runtime_error( "Missing dispose mode" );
	}
	
	return output;
}

QImage AnimCombiner::combine( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose, IndexColor transparent ){
	if( previous.isNull() ){
		previous = QImage( new_image.size(), new_image.format() );
		previous.setColorTable( new_image.colorTable() );
		previous.fill( isIndexed(previous) ? background_color.getIndexed() : background_color.getRgb() );
	}
	
	//Try to see if we can merge it indexed
	auto tryIndexed = combineIndexed( new_image, x, y, blend, dispose, transparent );
	if( !tryIndexed.isNull() )
		return tryIndexed;
	
	//QPainter can't handle indexed images, convert to something it can handle
	auto fixFormat = []( QImage& img, IndexColor transparent={} ){
			if( isIndexed( img ) ){
				if( transparent.hasIndex() && transparent.getIndexed() >= 0 ){
					auto palette = img.colorTable();
					palette[transparent.getIndexed()] = qRgba(0,0,0,0);
					img.setColorTable( palette );
				}
				img = img.convertToFormat( QImage::Format_ARGB32 );
			}
		};
	fixFormat( new_image, transparent );
	fixFormat( previous );
	
	QImage output = previous;
	QPainter painter( &output );
	
	if( blend == BlendMode::REPLACE )
		painter.setCompositionMode( QPainter::CompositionMode_Source );
	painter.drawImage( x, y, new_image );
	
	switch( dispose ){
		case DisposeMode::NONE: previous = output; break;
		case DisposeMode::BACKGROUND:{
				previous = output;
				QPainter canvas_painter( &previous );
				canvas_painter.setCompositionMode( QPainter::CompositionMode_Source );
				canvas_painter.fillRect( x, y, new_image.width(), new_image.height(), background_color.getRgb() );
			} break;
		case DisposeMode::REVERT: break;
	}
	
	return output;
}
