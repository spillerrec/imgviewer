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
#include <QGuiApplication>
#include <QScreen>

#include <algorithm>
#include <utility>


//Resizes the window so that "content" will have "wanted" size.
//Requires that only the only resizeable widget contains "content".
//Window will be moved so that it does not cross monitors boundaries.
//If "wanted" size can't fit on the monitor, it will be resized
//depending on "keep_aspect". If false, height and width will be clipped.
//If true, it will be scaled so "content" keeps it aspect ratio.
//Function returns "content"'s new size.
QSize windowManager::resize_content( QSize wanted, QSize content, bool keep_aspect, bool only_upscale ){
	//Get the difference in size between the content and the whole window
	QSize difference = window.frameGeometry().size() - content;
	
	//Prepare resize dimensions, but keep it within "space"
	auto getPosition = [&]( QScreen* monitor ){
		//Take the full area and subtract the constant widths (which doesn't scale with aspect)
		QRect space = monitor->availableGeometry();
		space.setSize( space.size() - difference );
		
		QRect position = constrain( space, QRect( contrain_point( space, window.pos() ), wanted ), keep_aspect );
		return std::make_pair( position, qsize_area( position.size() ) );
	};
	
	//Find the best monitor, but stay on the first monitor if several can fit the entire window
	auto best = getPosition( window.screen() );
	for( auto& screen : QGuiApplication::screens() )
		best = std::max( best, getPosition( screen ), []( std::pair<QRect,int> a, std::pair<QRect,int> b ){ return a.second < b.second; } );
	
	//Move and resize window. We need to convert dimensions to window size though.
	window.move( best.first.topLeft() );
	window.resize( best.first.size() + window.size() - content );
	
	return best.first.size();
}

void windowManager::restrain_window(){
	QRect space = window.screen()->availableGeometry();
	window.move( contrain_point( {space.topLeft(), space.size()-window.frameGeometry().size()}, window.pos() ) );
}

