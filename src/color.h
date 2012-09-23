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

#ifndef COLOR_H
#define COLOR_H

#include <QString>
#include <lcms2.h>

class QImage;

class color{
	private:
		cmsHPROFILE p_srgb;
		cmsHPROFILE p_monitor;
		cmsHTRANSFORM t_default;
	
	private:
		void do_transform( QImage *img, cmsHTRANSFORM transform ) const;
	
	public:
		explicit color( QString filepath );
		~color();
		
		void transform( QImage *img ) const{
			do_transform( img, t_default );
		}
		void transform( QImage *img, char *data, unsigned len ) const;
};


#endif