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

#include "color.h"

#include <lcms2.h>
#include <QImage.h>


color::color( QString filepath ){
	//Init default profiles
	p_srgb = cmsCreate_sRGBProfile();
	p_monitor = cmsOpenProfileFromFile("1.icm", "r");
	
	//If there is a profile, make a default transform.
	if( p_monitor ){
		t_default = cmsCreateTransform(
				p_srgb
			,	TYPE_BGRA_8
			,	p_monitor
			,	TYPE_BGRA_8
			,	INTENT_PERCEPTUAL
			,	0
			);
		qDebug( "Profile loaded" );
	}
	else{
		t_default = NULL;
		qDebug( "Profile not loaded" );
	}
}
color::~color(){
	//Delete profiles and transforms
	cmsCloseProfile( p_srgb );
	if( p_monitor )
		cmsCloseProfile( p_monitor );
	if( t_default )
		cmsDeleteTransform( t_default );
}

//Load profile from memory and transform image
void color::transform( QImage *img, char *data, unsigned len ) const{
	cmsHPROFILE in = cmsOpenProfileFromMem( (const void*)data, len );
	if( in ){
		//Create transform
		cmsHPROFILE transform = cmsCreateTransform(
				in
			,	TYPE_BGRA_8
			,	( p_monitor ) ? p_monitor : p_srgb //Fall back to sRGB
			,	TYPE_BGRA_8
			,	INTENT_PERCEPTUAL
			,	0
			);
		cmsCloseProfile( in );
		
		do_transform( img, transform );
		cmsDeleteTransform( transform );
	}
}


void color::do_transform( QImage *img, cmsHTRANSFORM transform ) const{
	
		qDebug( "Should transform" );
		if( transform ){
		//Make sure the image is in the correct pixel format
		QImage::Format format = img->format();
		qDebug( "Format: %d", (int)format );
		if(	format != QImage::Format_RGB32
			&&	format != QImage::Format_ARGB32
			&&	format != QImage::Format_ARGB32_Premultiplied
			)
			return;
		qDebug( "did transform %x", qRgba( 0xAA, 0xBB, 0xCC, 0xDD ) );
		
		
		
		//Convert
		for( int i=0; i < img->height(); i++ ){
			char* line = (char*)img->scanLine( i );
			cmsDoTransform( transform, line, line, img->width() );
		}
	}
}

