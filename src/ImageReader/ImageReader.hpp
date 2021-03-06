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

#ifndef IMAGE_READER_HPP
#define IMAGE_READER_HPP

#include <QString>
#include <vector>
#include <map>
#include <memory>
#include "AReader.hpp"

class ImageReader{
	protected:
		std::vector<std::unique_ptr<AReader>> readers;
		std::map<QString,AReader*> formats;
		
	public:
		ImageReader();
		
		AReader::Error read( imageCache &cache, QString filepath ) const;
		
		QList<QString> supportedExtensions() const;
};


#endif