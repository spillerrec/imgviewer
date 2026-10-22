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

#ifndef ANIMATIONHANDLER_H
#define ANIMATIONHANDLER_H

#include <QObject>
#include <QTimer>
#include <memory>

class imageCache;

class AnimationHandler : public QObject {
	Q_OBJECT

private:
	std::shared_ptr<imageCache> image_cache;
	int current_frame{ 0 };
	int loop_counter{ 0 };
	bool continue_animating{ false };
	int waiting_on_frame{ -1 };
	QTimer time;

public:
	explicit AnimationHandler( QObject* parent = nullptr );

	// State accessors
	int get_frame_amount() const;
	int get_current_frame() const { return current_frame; }
	bool can_animate() const;
	bool is_animating() const { return continue_animating; }

	// Control
	void set_image_cache( std::shared_ptr<imageCache> cache );
	void init_from_cache(); // called when info is loaded: sets loop_counter, continue_animating

private slots:
	void next_frame();

public slots:
	void change_frame( int frame );
	void goto_frame( int index );
	void goto_next_frame() { goto_frame( current_frame + 1); }
	void goto_prev_frame() { goto_frame( current_frame - 1); }
	void restart_animation();
	bool toggle_animation();
	void check_frame_loaded( unsigned int idx );

signals:
	void frame_changed( int new_frame );
	void animation_state_changed( bool animating );
};

#endif // ANIMATIONHANDLER_H
