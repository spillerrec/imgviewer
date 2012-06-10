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
#include <QApplication>
#include <QDesktopWidget>


windowManager::windowManager( QWidget *widget ){
	window = widget;
	desktop = QApplication::desktop();
}


windowManager::~windowManager(){
}


QSize windowManager::resize( QSize size ){
	QRect space = desktop->availableGeometry( window );
	QSize diff = window->frameGeometry().size() - window->size();
	qDebug( "diff: %d, %d", diff.width(), diff.height() );
	
	//Reduce to fit screen
	if( size.width() > space.width() )
		size.setWidth( space.width() );
	if( size.height() > space.height() )
		size.setHeight( space.height() );
	
	QRect position( window->pos(), size );
	
	//Check if there is room for it at current window position
	if( !space.contains( position ) ){
		//If window is to the upper left of the screern, move to corner
		if( position.x() < space.x() )
			position.setX( space.x() );
		if( position.y() < space.y() )
			position.setY( space.y() );
		
		//If window sticks out of the screen, move towards center
	qDebug( "position before: %d, %d, %d, %d", position.x(), position.y(), position.width(), position.height() );
		int space_right = space.x() + space.width();
		int space_bottom = space.y() + space.height();
		if( position.x() + position.width() > space_right )
			position.moveLeft( space_right - position.width() );
		if( position.y() + position.height() > space_bottom )
			position.moveTop( space_bottom - position.height() );
		
		window->move( position.topLeft() );
	}
	
	window->resize( position.size() - diff );
	
	qDebug( "position after: %d, %d, %d, %d", position.x(), position.y(), position.width(), position.height() );
	return position.size();
}


QSize windowManager::resize_content( QSize wanted, QSize content, bool keep_aspect ){
	QSize difference = window->frameGeometry().size() - content;
	return resize( wanted + difference ) - difference;
}

