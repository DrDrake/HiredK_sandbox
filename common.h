--[[
- @file common.h
- @brief
]]

#define EDITOR_WINDOW_TITLE "Planet Editor"

MessageID = {
	ID_PLAYER_INPUT = root.Network.ID_USER_PACKET_ENUM + 1
}

#include "shared/atmosphere.h"
#include "shared/camera_first_person.h"
#include "shared/camera_third_person.h"
#include "shared/camera_orbital.h"
#include "shared/camera_ortho.h"
#include "shared/object_types.h"
#include "shared/object_base.h"
#include "shared/object_shapes.h"
#include "shared/object_lights.h"
#include "shared/object_vehicles.h"
#include "shared/object_misc.h"
#include "shared/player.h"
#include "shared/ocean.h"
#include "shared/planet.h"
#include "shared/planet_producer.h"
#include "shared/planet_sector.h"
#include "shared/road_graph_layers.h"
#include "shared/render_pipeline.h"
#include "shared/volumetric_clouds.h"