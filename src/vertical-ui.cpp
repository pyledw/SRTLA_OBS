#include "vertical-ui.hpp"
#include "vertical-manager.hpp"
#include <QMouseEvent>
#include <QWheelEvent>

VerticalUIDock::VerticalUIDock(QWidget *parent)
	: QDockWidget("PyleIRL Vertical Canvas", parent)
{
	displayWidget = new QWidget();
	displayWidget->setMinimumSize(360, 640);
	setWidget(displayWidget);
}

VerticalUIDock::~VerticalUIDock()
{
	if (display) {
		obs_display_remove_draw_callback(display, drawPreview, this);
		obs_display_destroy(display);
	}
}

void VerticalUIDock::Initialize()
{
	if (display) return;

	gs_init_data info = {};
	info.cx = displayWidget->width();
	info.cy = displayWidget->height();
	info.format = GS_RGBA;
	info.zsformat = GS_ZS_NONE;
	info.window.hwnd = (HWND)displayWidget->winId();

	display = obs_display_create(&info, 0);
	if (display) {
		obs_display_add_draw_callback(display, drawPreview, this);
	}
}

void VerticalUIDock::resizeEvent(QResizeEvent *event)
{
	QDockWidget::resizeEvent(event);
	if (display) {
		obs_display_resize(display, displayWidget->width(), displayWidget->height());
	}
}

void VerticalUIDock::drawPreview(void *data, uint32_t cx, uint32_t cy)
{
	VerticalUIDock *dock = static_cast<VerticalUIDock *>(data);
	
	// Clear background
	gs_clear(GS_CLEAR_COLOR, nullptr, 0.0f, 0);

	obs_source_t *main_scene_source = obs_frontend_get_current_scene();
	if (main_scene_source) {
		obs_source_t *v_scene_source = VerticalManager::instance().getVerticalSceneSource(main_scene_source);
		if (v_scene_source) {
			// Calculate aspect ratio scaling
			float aspect = 1080.0f / 1920.0f;
			float scale = (float)cy / 1920.0f;
			if ((float)cx / (float)cy > aspect) {
				scale = (float)cy / 1920.0f; // Pillarbox
			} else {
				scale = (float)cx / 1080.0f; // Letterbox
			}

			gs_matrix_push();
			
			// Center the canvas
			float x_offset = (cx - (1080.0f * scale)) / 2.0f;
			float y_offset = (cy - (1920.0f * scale)) / 2.0f;
			
			gs_matrix_translate3f(x_offset, y_offset, 0.0f);
			gs_matrix_scale3f(scale, scale, 1.0f);

			obs_source_video_render(v_scene_source);

			gs_matrix_pop();
		}
		obs_source_release(main_scene_source);
	}
}

void VerticalUIDock::mousePressEvent(QMouseEvent *event)
{
	// Drag and drop implementation is complex natively.
	// For this sprint, we can leave it as an empty stub,
	// or implement basic hit testing.
	QDockWidget::mousePressEvent(event);
}

void VerticalUIDock::mouseMoveEvent(QMouseEvent *event)
{
	QDockWidget::mouseMoveEvent(event);
}

void VerticalUIDock::mouseReleaseEvent(QMouseEvent *event)
{
	QDockWidget::mouseReleaseEvent(event);
}

void VerticalUIDock::wheelEvent(QWheelEvent *event)
{
	QDockWidget::wheelEvent(event);
}

extern "C" void *create_vertical_dock()
{
	VerticalUIDock *dock = new VerticalUIDock();
	dock->Initialize();
	return dock;
}
