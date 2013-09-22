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
#include <QCoreApplication>
#include <QImage>

#include <qglobal.h>
#ifdef Q_OS_WIN
	#include <qt_windows.h>
#endif

color::color( QString filepath ){
	//Init default profiles
	p_srgb = cmsCreate_sRGBProfile();
	
#ifdef Q_OS_WIN
	//Try to grab it from the Windows APIs
	DISPLAY_DEVICE disp;
	DWORD index = 0;
	disp.cb = sizeof(DISPLAY_DEVICE);
	while( EnumDisplayDevices( NULL, index++, &disp, 0 ) != 0 ){
		//Temporaries for converting
		DWORD size = 250;
		wchar_t icc_path[size];
		char path_ancii[size*2];
		
		//Get profile
		HDC hdc = CreateDC( NULL, disp.DeviceName, icc_path, NULL ); //TODO: what is icc_path used here for?
		GetICMProfile( hdc, &size, icc_path );
		DeleteDC( hdc );
		
		//Read and add
		wcstombs( path_ancii, icc_path, size*2 );
		monitors.push_back( MonitorIcc( cmsOpenProfileFromFile( path_ancii, "r") ) );
	}
#else
	QString app_path = QCoreApplication::applicationDirPath();
	monitors.push_back( MonitorIcc( cmsOpenProfileFromFile( (app_path + "/1.icm").toLocal8Bit().data(), "r") ) );
#endif
	
	//Create default transforms
	for( unsigned i=0; i<monitors.size(); ++i ){
		MonitorIcc &icc( monitors[i] );
		//If there is a profile, make a default transform.
		if( icc.profile ){
			icc.transform_srgb = cmsCreateTransform(
					p_srgb
				,	TYPE_BGRA_8	//This might be incorrect on some systems???
				,	icc.profile
				,	TYPE_BGRA_8
				,	INTENT_PERCEPTUAL
				,	0
				);
		}
		else
			icc.transform_srgb = NULL;
	}
}
color::~color(){
	//Delete profiles and transforms
	cmsCloseProfile( p_srgb );
	for( unsigned i=0; i<monitors.size(); ++i ){
		if( monitors[i].profile )
			cmsCloseProfile( monitors[i].profile );
		if( monitors[i].transform_srgb )
			cmsDeleteTransform( monitors[i].transform_srgb );
	}
}


//Load a profile from memory and get its transform
cmsHTRANSFORM color::get_transform( cmsHPROFILE in, unsigned monitor ) const{
	if( in && monitors.size() > monitor ){
		//Get monitor profile, or fall-back to sRGB
		cmsHPROFILE use = monitors[monitor].profile;
		if( !use )
			use = p_srgb;
			
		//Create transform
		return cmsCreateTransform(
				in
			,	TYPE_BGRA_8
			,	use
			,	TYPE_BGRA_8
			,	INTENT_PERCEPTUAL
			,	0
			);
	}
	else
		return NULL;
}


void color::do_transform( QImage *img, unsigned monitor, cmsHTRANSFORM transform ) const{
	if( !transform && monitors.size() > monitor )
		transform = monitors[monitor].transform_srgb;
	
	if( transform ){
		//Make sure the image is in the correct pixel format
		QImage::Format format = img->format();
		//qDebug( "Format: %d", (int)format );
		if(	format != QImage::Format_RGB32
			&&	format != QImage::Format_ARGB32
			&&	format != QImage::Format_ARGB32_Premultiplied
			)
			return;
		//qDebug( "did transform %x", qRgba( 0xAA, 0xBB, 0xCC, 0xDD ) );
		
		//Convert
		//TODO: use multiple threads?
		for( int i=0; i < img->height(); i++ ){
			char* line = (char*)img->scanLine( i );
			cmsDoTransform( transform, line, line, img->width() );
		}
	}
}

