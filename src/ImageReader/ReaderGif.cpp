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

//NOTE: The gif_lib documentation is completely rubbish, so some parts is likely wrong

#include "ReaderGif.hpp"
#include "AnimCombiner.hpp"

#include <QImage>
#include <QPainter>
#include <gif_lib.h>
#include <cstring>
#include <cmath>
#include <vector>


bool ReaderGif::can_read( const uint8_t* data, unsigned length, QString ) const{
	return true;
	//TODO: Find out if there is anything like this, or if we have to look for some MAGIC bytes
}

inline QRgb convertColorType( GifColorType color )
	{ return qRgb( color.Red, color.Green, color.Blue ); }

static QVector<QRgb> convertColorMap( ColorMapObject* map ){
	if( !map )
		return {};
	
	//TODO: int BitsPerPixel ??
	//TODO: bool SortFlag ??
	QVector<QRgb> table;
	table.reserve( map->ColorCount );
	
	for( int i=0; i<map->ColorCount; i++ )
		table.push_back( convertColorType( map->Colors[i] ) );
	
	return table;
}

QImage convertImage( GifImageDesc image, GifByteType* raster, ColorMapObject* parent ){
	QImage img( image.Width, image.Height, QImage::Format_Indexed8 );
	img.setColorTable( convertColorMap( image.ColorMap ? image.ColorMap : parent ) );
	
	for( int iy=0; iy<image.Height; iy++ )
		std::memcpy( img.scanLine( iy ), raster+image.Width*iy, image.Width );
	
	return img;
}


struct Reader{
	const uint8_t* data;
	unsigned remaining;
	
	Reader( const uint8_t* data, unsigned remaining ) : data(data), remaining(remaining) { }
};

int ReadFromReader( GifFileType* gif, GifByteType* out, int amount ){
	auto reader = static_cast<Reader*>( gif->UserData );
	amount = std::min( amount, int(reader->remaining) );
	std::memcpy( out, reader->data, amount );
	reader->remaining -= amount;
	reader->data += amount;
	return amount;
}


DisposeMode gifDispose( GraphicsControlBlock* gcb ){
	if( !gcb )
		return DisposeMode::NONE;
	
	switch( gcb->DisposalMode ){
		case 0: return DisposeMode::NONE;
		case 1: return DisposeMode::NONE;
		case 2: return DisposeMode::BACKGROUND;
		case 3: return DisposeMode::REVERT;
		
		default:
			qWarning( "Unspecified GIF disposal mode: %d", gcb->DisposalMode );
			return DisposeMode::NONE;
	}
}


AReader::Error ReaderGif::read( imageCache &cache, const uint8_t* data, unsigned length, QString format ) const{
	if( !can_read( data, length, format ) )
		return ERROR_TYPE_UNKNOWN;
	
	//Set up reader
	Reader reader( data, length );
	int error;
	auto gif = DGifOpen( &reader, &ReadFromReader, &error );
	if( !gif )
		return ERROR_INITIALIZATION; //??
	
	//Read entire image
	if( DGifSlurp( gif ) != GIF_OK )
		return ERROR_FILE_BROKEN;
	
	cache.set_info( gif->ImageCount, true, -1 );
	qDebug( "Size: %dx%d", gif->SWidth, gif->SHeight );
	
	//TODO:
	AnimCombiner combiner( {} );
	for( int i=0; i<gif->ImageCount; i++ ){
		//qDebug( "Local color map: %p", gif->SavedImages[i].ImageDesc.ColorMap );
		auto saved = gif->SavedImages[i];
		auto img = convertImage( saved.ImageDesc, saved.RasterBits, gif->SColorMap );
		
		GraphicsControlBlock gcb;
		if( DGifSavedExtensionToGCB( gif, i, &gcb ) != GIF_OK )
			qDebug( "Did not contain stuff" );
		auto dispose = gifDispose( &gcb ); //TODO: If none
		
		auto delay = gcb.DelayTime * 10;
		delay = (delay == 0) ? 100 : delay; //TODO: replace with constant
		//TODO: Be 0 if no gcb
		
		auto transparent = gcb.TransparentColor;
		//TODO: check for -1
		
		cache.add_frame( combiner.combine( img, saved.ImageDesc.Left, saved.ImageDesc.Top, BlendMode::OVERLAY, dispose, transparent ), delay );
	}
	
	//Clean up
	return (DGifCloseFile( gif, &error ) != GIF_OK) ? ERROR_UNKNOWN : ERROR_NONE;
}

