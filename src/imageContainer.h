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

#ifndef IMAGECONTAINER_H
#define IMAGECONTAINER_H

#include <QWidget>
#include <QString>
#include <QList>

class imageViewer;
class imageCache;
class windowManager;
class fileManager;
class Ui_controls;

class imageContainer: public QWidget{
	Q_OBJECT
	
	private:
		imageViewer *viewer;
		windowManager *manager;
		Ui_controls *ui;
		
		fileManager *files;
		
		bool resize_window; //resize the window to fit image
		
	private:
		bool is_fullscreen;
		
	protected:
		void keyPressEvent( QKeyEvent *event );
		void dragEnterEvent( QDragEnterEvent *event );
		void dropEvent( QDropEvent *event );
		
	private slots:
		void update_controls();
		void update_toogle_btn();
		
		void update_file();
	
	public slots:
		void toogle_animation();
		void toogle_fullscreen();
		
		void next_file();
		void prev_file();
	
	public:
		explicit imageContainer( QWidget* parent = 0 );
		~imageContainer();
		
		void load_image( QString filepath );
};


#endif