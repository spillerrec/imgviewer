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
#include "viewer/qrect_extras.h"
#include <QApplication>
#include <QDesktopWidget>


windowManager::windowManager( QWidget *widget ){
	window = widget;
	desktop = QApplication::desktop();
}


windowManager::~windowManager(){
}


//Resizes the window so that "content" will have "wanted" size.
//Requires that only the only resizeable widget contains "content".
//Window will be moved so that it does not cross monitors boundaries.
//If "wanted" size can't fit on the monitor, it will be resized
//depending on "keep_aspect". If false, height and width will be clipped.
//If true, it will be scaled so "content" keeps it aspect ratio.
//Function returns "content"'s new size.
QSize windowManager::resize_content( QSize wanted, QSize content, bool keep_aspect ){
	//Get the difference in size between the content and the whole window
	QSize difference = window->frameGeometry().size() - content;
	
	//Take the full area and subtract the constant widths (which doesn't scale with aspect)
	QRect space = desktop->availableGeometry( window );
	space.setSize( space.size() - difference );
	
	//Prepare resize dimensions, but keep it within "space"
	QRect position = constrain( space, QRect( contrain_point( space, window->pos() ), wanted ), keep_aspect );
	
	if( position.size() != wanted ){
		int best = qsize_area( position.size() );
		
		for( int i=0; i<desktop->screenCount(); i++ ){
			QRect space_new = desktop->availableGeometry( i );
			space_new.setSize( space_new.size() - difference );
			
			QRect position_new = constrain( space_new, QRect( contrain_point( space_new, window->pos() ), wanted ), keep_aspect );
			
			int area = qsize_area( position_new.size() );
			if( area > best ){
				best = area;
				position = position_new;
			}
		}
	}
	
	//Move and resize window. We need to convert dimensions to window size though.
	window->move( position.topLeft() );
	window->resize( position.size() + window->size() - content );
	
	return position.size();
}

#include <algorithm>
void windowManager::restrain_window(){
	QRect space = desktop->availableGeometry( window );
	QRect frame = window->frameGeometry();
	frame.setTopLeft( window->pos() );
	
	if( frame.x() + frame.width() > space.x() + space.width() )
		frame.moveLeft( std::max( space.x(), space.x() + space.width() - frame.width() ) );
	if( frame.y() + frame.height() > space.y() + space.height() )
		frame.moveTop( std::max( space.y(), space.y() + space.height() - frame.height() ) );
	if( frame.x() < space.x() )
		frame.moveLeft( space.x() );
	if( frame.y() < space.y() )
		frame.moveTop( space.y() );
	window->move( frame.topLeft() );
//	window->resize( frame.size() );
}

