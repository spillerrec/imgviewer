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

#ifndef ANIM_FRAME_COMBINER_WORKER_HPP
#define ANIM_FRAME_COMBINER_WORKER_HPP

#include "AnimCombinerRgba.hpp"
#include "../viewer/imageCache.h"

#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

// A decoded APNG frame waiting to be combined on the worker thread.
struct PendingFrame {
	std::unique_ptr<uint8_t[]> pixels; // RGBA8 raw data (owned)
	int width, height;                 // Frame dimensions
	int x_offset, y_offset;            // Placement on canvas
	BlendMode blend_mode;
	DisposeMode dispose_mode;
	unsigned delay_ms;
};

// Background worker that combines APNG frames on a separate thread.
// The decoder pushes raw decoded frames via submit(); the worker runs
// AnimCombinerRgba and calls cache.add_frame() without blocking decoding.
class AnimFrameCombinerWorker {
private:
	imageCache& cache;
	AnimCombinerRgba combiner;

	std::queue<PendingFrame> queue;
	std::mutex mtx;
	std::condition_variable cv;
	std::atomic<bool> shutdown{ false };
	std::thread worker_thread;

public:
	// Start the background thread. Canvas size is needed for AnimCombinerRgba.
	AnimFrameCombinerWorker( imageCache& cache, int canvas_w, int canvas_h );
	
	~AnimFrameCombinerWorker();

	// Non-blocking: push a decoded frame to be combined later.
	void submit( PendingFrame&& frame );

	// Block until all submitted frames have been processed and added to cache.
	void wait_for_completion();

private:
	void worker_loop();
};

#endif // ANIM_FRAME_COMBINER_WORKER_HPP
