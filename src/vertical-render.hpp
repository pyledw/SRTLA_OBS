#pragma once

#include <obs.h>
#include <thread>
#include <atomic>

class VerticalRender {
public:
	static VerticalRender &instance();

	void start();
	void stop();

	obs_encoder_t *getEncoder() const { return encoder; }

private:
	VerticalRender();
	~VerticalRender();

	static void renderCallback(void *param, uint32_t cx, uint32_t cy);
	
	obs_encoder_t *encoder = nullptr;
	video_t *video = nullptr;
	
	std::thread renderThread;
	std::atomic<bool> active{false};
	
	void threadLoop();
};
