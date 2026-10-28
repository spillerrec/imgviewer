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

#include "AnimFrameCombinerWorker.hpp"

#include <tracy/Tracy.hpp>

AnimFrameCombinerWorker::AnimFrameCombinerWorker( imageCache& cache, int canvas_w, int canvas_h )
	: cache(cache), canvas_width(canvas_w), canvas_height(canvas_h) {
	worker_thread = std::thread( &AnimFrameCombinerWorker::worker_loop, this );
}

AnimFrameCombinerWorker::~AnimFrameCombinerWorker() {
	{
		std::lock_guard<std::mutex> lock(mtx);
		shutdown.store(true);
	}
	cv.notify_one();
	if ( worker_thread.joinable() )
		worker_thread.join();
}

void AnimFrameCombinerWorker::submit( PendingFrame&& frame ) {
	{
		std::lock_guard<std::mutex> lock(mtx);
		queue.push( std::move(frame) );
	}
	cv.notify_one();
}

void AnimFrameCombinerWorker::wait_for_completion() {
	ZoneScoped;
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait( lock, [this]{ return queue.empty(); } );
}

void AnimFrameCombinerWorker::worker_loop() {
	ZoneScoped;
	AnimCombinerRgba combiner( canvas_width, canvas_height );

	while ( true ) {
		PendingFrame frame;
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait( lock, [this]{ return !queue.empty() || shutdown.load(); } );

			if ( shutdown.load() && queue.empty() )
				return;

			frame = std::move(queue.front());
			queue.pop();
		}

		imageData output = combiner.combine( frame.pixels.get(),
	                                      frame.width, frame.height,
	                                      frame.x_offset, frame.y_offset,
	                                      frame.blend_mode, frame.dispose_mode );
		cache.add_frame( std::move(output), frame.delay_ms );

		cv.notify_one();
	}
}
