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

#ifndef ANIM_COMBINER_HPP
#define ANIM_COMBINER_HPP

#include <QImage>

enum class BlendMode{
	REPLACE,
	OVERLAY
};

enum class DisposeMode{
	NONE, //Do nothing
	BACKGROUND, //Clear the background to transparent
	REVERT //Revert to previous frame
};

class AnimCombiner{
	private:
		QImage previous;
		
	public:
		AnimCombiner( QImage previous ) : previous(previous) { }
		QImage combine( QImage new_image, int x, int y, BlendMode blend, DisposeMode dispose );
	
};


#endif