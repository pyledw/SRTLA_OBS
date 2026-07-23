#pragma once

#include <QDockWidget>
#include <obs.h>
#include <obs-frontend-api.h>

class VerticalUIDock : public QDockWidget {
	Q_OBJECT
public:
	VerticalUIDock(QWidget *parent = nullptr);
	~VerticalUIDock();

	void Initialize();

protected:
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
	obs_display_t *display = nullptr;
	QWidget *displayWidget = nullptr;

	static void drawPreview(void *data, uint32_t cx, uint32_t cy);

	obs_sceneitem_t *draggingItem = nullptr;
	float dragStartX, dragStartY;
	float itemStartX, itemStartY;
};
