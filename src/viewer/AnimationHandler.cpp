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

#include "AnimationHandler.h"
#include "../viewer/imageCache.h"

using namespace std;

AnimationHandler::AnimationHandler( QObject* parent ) : QObject( parent ), time( this ){
	time.setSingleShot( true );
	connect( &time, &QTimer::timeout, this, &AnimationHandler::next_frame );
}

int AnimationHandler::get_frame_amount() const{ 
	return image_cache ? image_cache->frame_count() : 0; 
}

bool AnimationHandler::can_animate() const {
	return image_cache ? image_cache->is_animated() : false;
}

void AnimationHandler::set_image_cache( std::shared_ptr<imageCache> cache ) {
	time.stop();
	image_cache = std::move(cache);
	current_frame = 0;
	loop_counter = 0;
	continue_animating = false;
	waiting_on_frame = -1;
}

void AnimationHandler::init_from_cache() {
	if( !image_cache ) return;
	loop_counter = image_cache->loop_count();
	continue_animating = image_cache->is_animated();
	emit animation_state_changed( continue_animating );
}

void AnimationHandler::next_frame() {
	change_frame( current_frame + 1 );
}

void AnimationHandler::change_frame( int wanted ) {
	if( !image_cache ) return;

	auto frame_amount = image_cache->frame_count();

	// Cycle backwards
	if( wanted < 0 )
		wanted = frame_amount - 1;

	if( image_cache->loaded() < frame_amount && wanted >= image_cache->loaded() ){
		// Wait for frame to be available
		waiting_on_frame = wanted;
		return;
	}

	if( frame_amount < 1 ) return; // Not loaded

	// Go to next frame
	current_frame = wanted;
	if( current_frame >= frame_amount ){
		// Last frame reached
		if( loop_counter > 0 ){
			// Reduce counter by one
			current_frame = 0;
			loop_counter--;
		}
		else if( loop_counter == -1 )
			current_frame = 0; // Repeat forever
		else{
			continue_animating = false; // Stop looping
			current_frame--;
			emit animation_state_changed( continue_animating );
		}
	}

	if( continue_animating ){
		int delay = image_cache->frame_delay( current_frame );
		if( delay > 0 ) time.start( delay );
	}

	emit frame_changed( current_frame );
}

void AnimationHandler::goto_frame( int index ) {
	time.stop();
	continue_animating = false;
	change_frame( index );
	emit animation_state_changed( continue_animating );
}

void AnimationHandler::restart_animation() {
	continue_animating = can_animate();
	emit animation_state_changed( continue_animating );
	current_frame = -1; // Force wrap to 0 in change_frame
	change_frame( current_frame + 1 );
}

bool AnimationHandler::toggle_animation() {
	if( image_cache && image_cache->is_animated() ){
		if( continue_animating ){
			time.stop();
			continue_animating = false;
		}
		else{
			continue_animating = true;
			next_frame();
		}
	}
	emit animation_state_changed( continue_animating );
	return continue_animating;
}

void AnimationHandler::check_frame_loaded( unsigned int idx ) {
	if( waiting_on_frame <= -1 ) return;

	if( idx == (unsigned int)waiting_on_frame ){
		waiting_on_frame = -1;
		change_frame( idx );
	}
}
