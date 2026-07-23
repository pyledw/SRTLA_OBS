#pragma once

#include <obs.h>
#include <obs-frontend-api.h>
#include <QObject>
#include <QMap>
#include <QString>

class VerticalManager : public QObject {
	Q_OBJECT
public:
	static VerticalManager &instance();

	void initialize();
	void shutdown();

	obs_source_t *getVerticalSceneSource(obs_source_t *mainSceneSource);
	void createVerticalScene(obs_source_t *mainSceneSource);

private:
	VerticalManager(QObject *parent = nullptr);
	~VerticalManager();

	static void obsFrontendEvent(enum obs_frontend_event event, void *private_data);

	void scanExistingScenes();

	static void sourceCreated(void *data, calldata_t *cd);
	static void sourceRemoved(void *data, calldata_t *cd);

	static void onItemAdd(void *data, calldata_t *cd);
	static void onItemRemove(void *data, calldata_t *cd);
	static void onItemVisible(void *data, calldata_t *cd);

	QMap<obs_source_t *, obs_source_t *> horizontalToVertical;
	bool initialized = false;
};
