#include "vertical-render.hpp"
#include "vertical-manager.hpp"
#include <media-io/video-io.h>
#include <media-io/video-frame.h>
#include <graphics/graphics.h>
#include <obs-avc.h>
#include <util/platform.h>

VerticalRender &VerticalRender::instance()
{
	static VerticalRender inst;
	return inst;
}

VerticalRender::VerticalRender() {}
VerticalRender::~VerticalRender() { stop(); }

void VerticalRender::start()
{
	if (active) return;

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	struct video_output_info vi = {0};
	vi.name = "PyleIRL Vertical Mix";
	vi.format = VIDEO_FORMAT_RGBA; // OBS handles conversion to NV12
	vi.fps_num = ovi.fps_num;
	vi.fps_den = ovi.fps_den;
	vi.width = 1080;
	vi.height = 1920;
	vi.cache_size = 6;
	vi.colorspace = ovi.colorspace;
	vi.range = ovi.range;

	if (video_output_open(&video, &vi) != VIDEO_OUTPUT_SUCCESS) {
		return;
	}

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "rate_control", "CBR");
	obs_data_set_int(settings, "bitrate", 6000);
	
	encoder = obs_video_encoder_create("obs_x264", "vertical_h264", settings, nullptr);
	obs_data_release(settings);

	if (encoder && video) {
		obs_encoder_set_video(encoder, video);
	}

	active = true;
	renderThread = std::thread(&VerticalRender::threadLoop, this);
}

void VerticalRender::stop()
{
	if (!active) return;
	active = false;
	if (renderThread.joinable()) {
		renderThread.join();
	}

	if (encoder) {
		obs_encoder_release(encoder);
		encoder = nullptr;
	}
	if (video) {
		video_output_close(video);
		video = nullptr;
	}
}

void VerticalRender::threadLoop()
{
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	uint64_t interval = 1000000000ULL * ovi.fps_den / ovi.fps_num;
	uint64_t next_time = os_gettime_ns();

	gs_texrender_t *texrender = nullptr;
	gs_stagesurf_t *stagesurf = nullptr;

	while (active) {
		uint64_t now = os_gettime_ns();
		if (now < next_time) {
			os_sleepto_ns(next_time);
		}
		next_time += interval;

		obs_enter_graphics();

		if (!texrender) texrender = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
		if (!stagesurf) stagesurf = gs_stagesurface_create(1080, 1920, GS_RGBA);

		gs_texrender_begin(texrender, 1080, 1920);
		gs_clear(GS_CLEAR_COLOR, nullptr, 0.0f, 0);

		obs_source_t *main_scene_source = obs_frontend_get_current_scene();
		if (main_scene_source) {
			obs_source_t *v_scene_source = VerticalManager::instance().getVerticalSceneSource(main_scene_source);
			if (v_scene_source) {
				obs_source_video_render(v_scene_source);
			}
			obs_source_release(main_scene_source);
		}

		gs_texrender_end(texrender);

		gs_stage_texture(stagesurf, gs_texrender_get_texture(texrender));

		uint8_t *data;
		uint32_t linesize;
		if (gs_stagesurface_map(stagesurf, &data, &linesize)) {
			struct video_frame frame;
			if (video_output_lock_frame(video, &frame, 1, os_gettime_ns())) {
				uint32_t height = 1920;
				for (uint32_t y = 0; y < height; y++) {
					memcpy(frame.data[0] + y * frame.linesize[0], data + y * linesize, linesize);
				}
				video_output_unlock_frame(video);
			}
			gs_stagesurface_unmap(stagesurf);
		}

		obs_leave_graphics();
	}

	obs_enter_graphics();
	if (texrender) gs_texrender_destroy(texrender);
	if (stagesurf) gs_stagesurface_destroy(stagesurf);
	obs_leave_graphics();
}
