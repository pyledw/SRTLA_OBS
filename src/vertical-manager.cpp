#include "vertical-manager.hpp"
#include <util/bmem.h>
#include <QDebug>

VerticalManager &VerticalManager::instance()
{
	static VerticalManager inst;
	return inst;
}

VerticalManager::VerticalManager(QObject *parent) : QObject(parent) {}

VerticalManager::~VerticalManager()
{
	shutdown();
}

void VerticalManager::initialize()
{
	if (initialized)
		return;

	// Hook into OBS frontend events
	obs_frontend_add_event_callback(obsFrontendEvent, this);
	
	// Hook into source creation/destruction globally
	signal_handler_t *core_sh = obs_get_signal_handler();
	signal_handler_connect(core_sh, "source_create", sourceCreated, this);
	signal_handler_connect(core_sh, "source_remove", sourceRemoved, this);

	scanExistingScenes();

	initialized = true;
}

void VerticalManager::shutdown()
{
	if (!initialized)
		return;

	obs_frontend_remove_event_callback(obsFrontendEvent, this);
	
	signal_handler_t *core_sh = obs_get_signal_handler();
	signal_handler_disconnect(core_sh, "source_create", sourceCreated, this);
	signal_handler_disconnect(core_sh, "source_remove", sourceRemoved, this);

	horizontalToVertical.clear();
	initialized = false;
}

obs_source_t *VerticalManager::getVerticalSceneSource(obs_source_t *mainSceneSource)
{
	if (horizontalToVertical.contains(mainSceneSource)) {
		return horizontalToVertical[mainSceneSource];
	}
	return nullptr;
}

void VerticalManager::obsFrontendEvent(enum obs_frontend_event event, void *private_data)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(private_data);
	if (event == OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED) {
		mgr->scanExistingScenes();
	}
}

static bool scan_scene_callback(void *param, obs_source_t *source)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(param);
	if (obs_source_get_id(source) == QString("scene")) {
		mgr->createVerticalScene(source);
	}
	return true;
}

void VerticalManager::scanExistingScenes()
{
	// Enum all scenes in frontend
	char **scene_names = obs_frontend_get_scene_names();
	if (scene_names) {
		for (int i = 0; scene_names[i] != NULL; i++) {
			obs_source_t *source = obs_get_source_by_name(scene_names[i]);
			if (source) {
				createVerticalScene(source);
				obs_source_release(source);
			}
		}
		bfree(scene_names);
	}
}

void VerticalManager::createVerticalScene(obs_source_t *mainSceneSource)
{
	// Check if this is already a vertical scene
	QString mainName = obs_source_get_name(mainSceneSource);
	if (mainName.startsWith("[V] "))
		return;

	if (horizontalToVertical.contains(mainSceneSource))
		return;

	QString vName = "[V] " + mainName;
	
	// Check if it already exists in OBS
	obs_source_t *vSource = obs_get_source_by_name(vName.toUtf8().constData());
	if (!vSource) {
		obs_scene_t *vScene = obs_scene_create(vName.toUtf8().constData());
		if (vScene) {
			vSource = obs_scene_get_source(vScene);
			// We do not add it to frontend so it doesn't clutter the main list,
			// or we can add it to frontend so they can edit it!
			// We MUST add it to the frontend so they can select and edit it natively.
			// Wait, the plan says we add it. OBS automatically adds it?
			// obs_scene_create doesn't add to frontend.
			// Currently we will not add to frontend, we will make a custom Qt Dock!
			// Actually, the new plan says: "We will build a custom Qt Dock into the OBS UI... These scenes are truly hidden"
		}
	} else {
		// Existing vertical scene found
	}

	if (vSource) {
		horizontalToVertical.insert(mainSceneSource, vSource);

		// Hook into main scene's item events to sync
		signal_handler_t *sh = obs_source_get_signal_handler(mainSceneSource);
		signal_handler_connect(sh, "item_add", onItemAdd, this);
		signal_handler_connect(sh, "item_remove", onItemRemove, this);
		signal_handler_connect(sh, "item_visible", onItemVisible, this);
		
		// Optional: Initial sync of items
		obs_scene_t *mainScene = obs_scene_from_source(mainSceneSource);
		obs_scene_t *vertScene = obs_scene_from_source(vSource);
		
		if (mainScene && vertScene) {
			obs_scene_enum_items(mainScene, [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
				obs_scene_t *v_scene = static_cast<obs_scene_t *>(param);
				obs_source_t *item_source = obs_sceneitem_get_source(item);
				
				// Check if it already exists in vertical scene
				obs_sceneitem_t *existing_v_item = obs_scene_sceneitem_from_source(v_scene, item_source);
				if (!existing_v_item) {
					obs_sceneitem_t *new_v_item = obs_scene_add(v_scene, item_source);
					obs_sceneitem_set_visible(new_v_item, obs_sceneitem_visible(item));
					
					// Apply default centering
					struct vec2 pos;
					vec2_set(&pos, 420.0f, 0.0f); // 1920/2 - 1080/2 = 420
					obs_sceneitem_set_pos(new_v_item, &pos);
				}
				return true;
			}, vertScene);
		}
		
		obs_source_release(vSource); // Since obs_get_source_by_name or obs_scene_get_source adds a ref
	}
}

void VerticalManager::sourceCreated(void *data, calldata_t *cd)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(data);
	obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
	if (obs_source_get_id(source) == QString("scene")) {
		mgr->createVerticalScene(source);
	}
}

void VerticalManager::sourceRemoved(void *data, calldata_t *cd)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(data);
	obs_source_t *source = (obs_source_t *)calldata_ptr(cd, "source");
	
	if (mgr->horizontalToVertical.contains(source)) {
		obs_source_t *vSource = mgr->horizontalToVertical.take(source);
		signal_handler_t *sh = obs_source_get_signal_handler(source);
		signal_handler_disconnect(sh, "item_add", onItemAdd, mgr);
		signal_handler_disconnect(sh, "item_remove", onItemRemove, mgr);
		signal_handler_disconnect(sh, "item_visible", onItemVisible, mgr);
		
		// If we want to clean up the hidden vertical scene
		// obs_source_remove(vSource); 
	}
}

void VerticalManager::onItemAdd(void *data, calldata_t *cd)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(data);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(cd, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(cd, "item");
	obs_source_t *source = obs_sceneitem_get_source(item);
	obs_source_t *sceneSource = obs_scene_get_source(scene);

	obs_source_t *vSceneSource = mgr->getVerticalSceneSource(sceneSource);
	if (vSceneSource) {
		obs_scene_t *vScene = obs_scene_from_source(vSceneSource);
		
		// Check if already there
		obs_sceneitem_t *existing = obs_scene_sceneitem_from_source(vScene, source);
		if (!existing) {
			obs_sceneitem_t *new_v_item = obs_scene_add(vScene, source);
			obs_sceneitem_set_visible(new_v_item, obs_sceneitem_visible(item));
			
			// Center it for 1080x1920 (420px offset)
			struct vec2 pos;
			vec2_set(&pos, 420.0f, 0.0f);
			obs_sceneitem_set_pos(new_v_item, &pos);
		}
	}
}

void VerticalManager::onItemRemove(void *data, calldata_t *cd)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(data);
	obs_scene_t *scene = (obs_scene_t *)calldata_ptr(cd, "scene");
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(cd, "item");
	obs_source_t *source = obs_sceneitem_get_source(item);
	obs_source_t *sceneSource = obs_scene_get_source(scene);

	obs_source_t *vSceneSource = mgr->getVerticalSceneSource(sceneSource);
	if (vSceneSource) {
		obs_scene_t *vScene = obs_scene_from_source(vSceneSource);
		obs_sceneitem_t *v_item = obs_scene_sceneitem_from_source(vScene, source);
		if (v_item) {
			obs_sceneitem_remove(v_item);
		}
	}
}

void VerticalManager::onItemVisible(void *data, calldata_t *cd)
{
	VerticalManager *mgr = static_cast<VerticalManager *>(data);
	obs_sceneitem_t *item = (obs_sceneitem_t *)calldata_ptr(cd, "item");
	bool visible = calldata_bool(cd, "visible");
	
	obs_scene_t *scene = obs_sceneitem_get_scene(item);
	obs_source_t *source = obs_sceneitem_get_source(item);
	obs_source_t *sceneSource = obs_scene_get_source(scene);

	obs_source_t *vSceneSource = mgr->getVerticalSceneSource(sceneSource);
	if (vSceneSource) {
		obs_scene_t *vScene = obs_scene_from_source(vSceneSource);
		obs_sceneitem_t *v_item = obs_scene_sceneitem_from_source(vScene, source);
		if (v_item) {
			obs_sceneitem_set_visible(v_item, visible);
		}
	}
}

#include "vertical-render.hpp"
extern "C" void start_vertical_services()
{
	VerticalManager::instance().initialize();
	VerticalRender::instance().start();
}
