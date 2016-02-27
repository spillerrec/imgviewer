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

#ifndef ORIENTATION_HPP
#define ORIENTATION_HPP

struct Orientation{
	int8_t rotation{ 0 };
	bool flip_ver{ false };
	bool flip_hor{ false };
	
	Orientation difference( Orientation other ) const{
		return {
				int8_t(other.rotation - rotation)
			,	other.flip_ver != flip_ver
			,	other.flip_hor != flip_hor
			};
	}
};

#endif

