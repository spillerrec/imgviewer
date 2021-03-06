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

#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QWidget>

class windowManager{
	private:
		QWidget& window;
		
	public:
		explicit windowManager( QWidget& widget ) : window(widget) { };
		
		QSize resize_content( QSize wanted, QSize content, bool keep_aspect = false, bool only_upscale = false );
		void restrain_window();
};


#endif