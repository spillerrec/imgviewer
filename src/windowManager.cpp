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

#include "windowManager.h"
#include "qrect_extras.h"
#include <QApplication>
#include <QDesktopWidget>


windowManager::windowManager( QWidget *widget ){
	window = widget;
	desktop = QApplication::desktop();
}


windowManager::~windowManager(){
}


//Resizes the window so that "content" will have "wanted" size.
//Requires that only the only resizable widget contains "content".
//Window will be moved so that it does not cross monitors boundaries.
//If "wanted" size can't fit on the monitor, it will be resized
//depending on "keep_aspect". If false, height and width will be clipped.
//If true, it will be scaled so "content" keeps it aspect ratio.
//Function returns "content"'s new size.
QSize windowManager::resize_content( QSize wanted, QSize content, bool keep_aspect ){
	//Get the difference in size between the content and the whole window
	QSize difference = window->frameGeometry().size() - content;
	
	//Take the full area and substract the constant widths (which doesn't scale with aspect)
	QRect space = desktop->availableGeometry( window );
	space.setSize( space.size() - difference );
	
	//Push window top-left edge into the screen
	QPoint topleft = window->pos();
	if( !space.contains( topleft ) ){
		if( space.x() > topleft.x() )
			topleft.setX( space.x() );
		if( space.y() > topleft.y() )
			topleft.setY( space.y() );
	}
	
	//Prepare resize dimentions, but keep it within "space"
	QRect position = constrain( space, QRect( topleft, wanted ), keep_aspect );
	
	//Move and resize window. We need to convert dimentions to window size though.
	window->move( position.topLeft() );
	window->resize( position.size() + window->size() - content );
	
	return position.size();
}

