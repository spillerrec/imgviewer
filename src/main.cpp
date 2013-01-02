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

#include <QCoreApplication>
#include <QApplication>
#include <QDir>

#include "imageContainer.h"


int main(int argc, char *argv[]){
	QApplication a(argc, argv);
	
	QStringList args = QCoreApplication::arguments();
	
	imageContainer test(NULL);
	test.show();
	
	if( args.size() == 2 ){
		QDir file( args.at(1) );
		test.load_image( file.path() );
	}
	
	return a.exec();
}
