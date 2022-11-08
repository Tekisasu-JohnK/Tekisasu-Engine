/*************************************************************************/
/*  navigation_agent_2d.cpp                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "navigation_agent_2d.h"

#include "core/math/geometry_2d.h"
#include "scene/resources/world_2d.h"
#include "servers/navigation_server_2d.h"

void NavigationAgent2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_rid"), &NavigationAgent2D::get_rid);

	ClassDB::bind_method(D_METHOD("set_avoidance_enabled", "enabled"), &NavigationAgent2D::set_avoidance_enabled);
	ClassDB::bind_method(D_METHOD("get_avoidance_enabled"), &NavigationAgent2D::get_avoidance_enabled);

	ClassDB::bind_method(D_METHOD("set_path_desired_distance", "desired_distance"), &NavigationAgent2D::set_path_desired_distance);
	ClassDB::bind_method(D_METHOD("get_path_desired_distance"), &NavigationAgent2D::get_path_desired_distance);

	ClassDB::bind_method(D_METHOD("set_target_desired_distance", "desired_distance"), &NavigationAgent2D::set_target_desired_distance);
	ClassDB::bind_method(D_METHOD("get_target_desired_distance"), &NavigationAgent2D::get_target_desired_distance);

	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &NavigationAgent2D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &NavigationAgent2D::get_radius);

	ClassDB::bind_method(D_METHOD("set_neighbor_distance", "neighbor_distance"), &NavigationAgent2D::set_neighbor_distance);
	ClassDB::bind_method(D_METHOD("get_neighbor_distance"), &NavigationAgent2D::get_neighbor_distance);

	ClassDB::bind_method(D_METHOD("set_max_neighbors", "max_neighbors"), &NavigationAgent2D::set_max_neighbors);
	ClassDB::bind_method(D_METHOD("get_max_neighbors"), &NavigationAgent2D::get_max_neighbors);

	ClassDB::bind_method(D_METHOD("set_time_horizon", "time_horizon"), &NavigationAgent2D::set_time_horizon);
	ClassDB::bind_method(D_METHOD("get_time_horizon"), &NavigationAgent2D::get_time_horizon);

	ClassDB::bind_method(D_METHOD("set_max_speed", "max_speed"), &NavigationAgent2D::set_max_speed);
	ClassDB::bind_method(D_METHOD("get_max_speed"), &NavigationAgent2D::get_max_speed);

	ClassDB::bind_method(D_METHOD("set_path_max_distance", "max_speed"), &NavigationAgent2D::set_path_max_distance);
	ClassDB::bind_method(D_METHOD("get_path_max_distance"), &NavigationAgent2D::get_path_max_distance);

	ClassDB::bind_method(D_METHOD("set_navigation_layers", "navigation_layers"), &NavigationAgent2D::set_navigation_layers);
	ClassDB::bind_method(D_METHOD("get_navigation_layers"), &NavigationAgent2D::get_navigation_layers);

	ClassDB::bind_method(D_METHOD("set_navigation_layer_value", "layer_number", "value"), &NavigationAgent2D::set_navigation_layer_value);
	ClassDB::bind_method(D_METHOD("get_navigation_layer_value", "layer_number"), &NavigationAgent2D::get_navigation_layer_value);

	ClassDB::bind_method(D_METHOD("set_navigation_map", "navigation_map"), &NavigationAgent2D::set_navigation_map);
	ClassDB::bind_method(D_METHOD("get_navigation_map"), &NavigationAgent2D::get_navigation_map);

	ClassDB::bind_method(D_METHOD("set_target_location", "location"), &NavigationAgent2D::set_target_location);
	ClassDB::bind_method(D_METHOD("get_target_location"), &NavigationAgent2D::get_target_location);

	ClassDB::bind_method(D_METHOD("get_next_location"), &NavigationAgent2D::get_next_location);
	ClassDB::bind_method(D_METHOD("distance_to_target"), &NavigationAgent2D::distance_to_target);
	ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &NavigationAgent2D::set_velocity);
	ClassDB::bind_method(D_METHOD("get_nav_path"), &NavigationAgent2D::get_nav_path);
	ClassDB::bind_method(D_METHOD("get_nav_path_index"), &NavigationAgent2D::get_nav_path_index);
	ClassDB::bind_method(D_METHOD("is_target_reached"), &NavigationAgent2D::is_target_reached);
	ClassDB::bind_method(D_METHOD("is_target_reachable"), &NavigationAgent2D::is_target_reachable);
	ClassDB::bind_method(D_METHOD("is_navigation_finished"), &NavigationAgent2D::is_navigation_finished);
	ClassDB::bind_method(D_METHOD("get_final_location"), &NavigationAgent2D::get_final_location);

	ClassDB::bind_method(D_METHOD("_avoidance_done", "new_velocity"), &NavigationAgent2D::_avoidance_done);

	ADD_GROUP("Pathfinding", "");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "target_location", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR), "set_target_location", "get_target_location");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "path_desired_distance", PROPERTY_HINT_RANGE, "0.1,100,0.01,suffix:px"), "set_path_desired_distance", "get_path_desired_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "target_desired_distance", PROPERTY_HINT_RANGE, "0.1,100,0.01,suffix:px"), "set_target_desired_distance", "get_target_desired_distance");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "path_max_distance", PROPERTY_HINT_RANGE, "10,100,1,suffix:px"), "set_path_max_distance", "get_path_max_distance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "navigation_layers", PROPERTY_HINT_LAYERS_2D_NAVIGATION), "set_navigation_layers", "get_navigation_layers");

	ADD_GROUP("Avoidance", "");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "avoidance_enabled"), "set_avoidance_enabled", "get_avoidance_enabled");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.1,500,0.01,suffix:px"), "set_radius", "get_radius");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "neighbor_distance", PROPERTY_HINT_RANGE, "0.1,100000,0.01,suffix:px"), "set_neighbor_distance", "get_neighbor_distance");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "max_neighbors", PROPERTY_HINT_RANGE, "1,10000,1"), "set_max_neighbors", "get_max_neighbors");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "time_horizon", PROPERTY_HINT_RANGE, "0.1,10000,0.01,suffix:s"), "set_time_horizon", "get_time_horizon");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_speed", PROPERTY_HINT_RANGE, "0.1,100000,0.01,suffix:px/s"), "set_max_speed", "get_max_speed");

	ADD_SIGNAL(MethodInfo("path_changed"));
	ADD_SIGNAL(MethodInfo("target_reached"));
	ADD_SIGNAL(MethodInfo("navigation_finished"));
	ADD_SIGNAL(MethodInfo("velocity_computed", PropertyInfo(Variant::VECTOR2, "safe_velocity")));
}

void NavigationAgent2D::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_POST_ENTER_TREE: {
			// need to use POST_ENTER_TREE cause with normal ENTER_TREE not all required Nodes are ready.
			// cannot use READY as ready does not get called if Node is readded to SceneTree
			set_agent_parent(get_parent());
			set_physics_process_internal(true);
		} break;

		case NOTIFICATION_PARENTED: {
			if (is_inside_tree() && (get_parent() != agent_parent)) {
				// only react to PARENTED notifications when already inside_tree and parent changed, e.g. users switch nodes around
				// PARENTED notification fires also when Node is added in scripts to a parent
				// this would spam transforms fails and world fails while Node is outside SceneTree
				// when node gets reparented when joining the tree POST_ENTER_TREE takes care of this
				set_agent_parent(get_parent());
				set_physics_process_internal(true);
			}
		} break;

		case NOTIFICATION_UNPARENTED: {
			// if agent has no parent no point in processing it until reparented
			set_agent_parent(nullptr);
			set_physics_process_internal(false);
		} break;

		case NOTIFICATION_PAUSED: {
			if (agent_parent && !agent_parent->can_process()) {
				map_before_pause = NavigationServer2D::get_singleton()->agent_get_map(get_rid());
				NavigationServer2D::get_singleton()->agent_set_map(get_rid(), RID());
			} else if (agent_parent && agent_parent->can_process() && !(map_before_pause == RID())) {
				NavigationServer2D::get_singleton()->agent_set_map(get_rid(), map_before_pause);
				map_before_pause = RID();
			}
		} break;

		case NOTIFICATION_UNPAUSED: {
			if (agent_parent && !agent_parent->can_process()) {
				map_before_pause = NavigationServer2D::get_singleton()->agent_get_map(get_rid());
				NavigationServer2D::get_singleton()->agent_set_map(get_rid(), RID());
			} else if (agent_parent && agent_parent->can_process() && !(map_before_pause == RID())) {
				NavigationServer2D::get_singleton()->agent_set_map(get_rid(), map_before_pause);
				map_before_pause = RID();
			}
		} break;

		case NOTIFICATION_EXIT_TREE: {
			agent_parent = nullptr;
			set_physics_process_internal(false);
		} break;

		case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
			if (agent_parent) {
				if (avoidance_enabled) {
					// agent_position on NavigationServer is avoidance only and has nothing to do with pathfinding
					// no point in flooding NavigationServer queue with agent position updates that get send to the void if avoidance is not used
					NavigationServer2D::get_singleton()->agent_set_position(agent, agent_parent->get_global_position());
				}
				_check_distance_to_target();
			}
		} break;
	}
}

NavigationAgent2D::NavigationAgent2D() {
	agent = NavigationServer2D::get_singleton()->agent_create();
	set_neighbor_distance(500.0);
	set_max_neighbors(10);
	set_time_horizon(20.0);
	set_radius(10.0);
	set_max_speed(200.0);

	// Preallocate query and result objects to improve performance.
	navigation_query = Ref<NavigationPathQueryParameters2D>();
	navigation_query.instantiate();

	navigation_result = Ref<NavigationPathQueryResult2D>();
	navigation_result.instantiate();
}

NavigationAgent2D::~NavigationAgent2D() {
	NavigationServer2D::get_singleton()->free(agent);
	agent = RID(); // Pointless
}

void NavigationAgent2D::set_avoidance_enabled(bool p_enabled) {
	avoidance_enabled = p_enabled;
	if (avoidance_enabled) {
		NavigationServer2D::get_singleton()->agent_set_callback(agent, this, "_avoidance_done");
	} else {
		NavigationServer2D::get_singleton()->agent_set_callback(agent, nullptr, "_avoidance_done");
	}
}

bool NavigationAgent2D::get_avoidance_enabled() const {
	return avoidance_enabled;
}

void NavigationAgent2D::set_agent_parent(Node *p_agent_parent) {
	// remove agent from any avoidance map before changing parent or there will be leftovers on the RVO map
	NavigationServer2D::get_singleton()->agent_set_callback(agent, nullptr, "_avoidance_done");
	if (Object::cast_to<Node2D>(p_agent_parent) != nullptr) {
		// place agent on navigation map first or else the RVO agent callback creation fails silently later
		agent_parent = Object::cast_to<Node2D>(p_agent_parent);
		if (map_override.is_valid()) {
			NavigationServer2D::get_singleton()->agent_set_map(get_rid(), map_override);
		} else {
			NavigationServer2D::get_singleton()->agent_set_map(get_rid(), agent_parent->get_world_2d()->get_navigation_map());
		}
		// create new avoidance callback if enabled
		set_avoidance_enabled(avoidance_enabled);
	} else {
		agent_parent = nullptr;
		NavigationServer2D::get_singleton()->agent_set_map(get_rid(), RID());
	}
}

void NavigationAgent2D::set_navigation_layers(uint32_t p_navigation_layers) {
	bool navigation_layers_changed = navigation_layers != p_navigation_layers;
	navigation_layers = p_navigation_layers;
	if (navigation_layers_changed) {
		_request_repath();
	}
}

uint32_t NavigationAgent2D::get_navigation_layers() const {
	return navigation_layers;
}

void NavigationAgent2D::set_navigation_layer_value(int p_layer_number, bool p_value) {
	ERR_FAIL_COND_MSG(p_layer_number < 1, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_MSG(p_layer_number > 32, "Navigation layer number must be between 1 and 32 inclusive.");
	uint32_t _navigation_layers = get_navigation_layers();
	if (p_value) {
		_navigation_layers |= 1 << (p_layer_number - 1);
	} else {
		_navigation_layers &= ~(1 << (p_layer_number - 1));
	}
	set_navigation_layers(_navigation_layers);
}

bool NavigationAgent2D::get_navigation_layer_value(int p_layer_number) const {
	ERR_FAIL_COND_V_MSG(p_layer_number < 1, false, "Navigation layer number must be between 1 and 32 inclusive.");
	ERR_FAIL_COND_V_MSG(p_layer_number > 32, false, "Navigation layer number must be between 1 and 32 inclusive.");
	return get_navigation_layers() & (1 << (p_layer_number - 1));
}

void NavigationAgent2D::set_navigation_map(RID p_navigation_map) {
	map_override = p_navigation_map;
	NavigationServer2D::get_singleton()->agent_set_map(agent, map_override);
	_request_repath();
}

RID NavigationAgent2D::get_navigation_map() const {
	if (map_override.is_valid()) {
		return map_override;
	} else if (agent_parent != nullptr) {
		return agent_parent->get_world_2d()->get_navigation_map();
	}
	return RID();
}

void NavigationAgent2D::set_path_desired_distance(real_t p_dd) {
	path_desired_distance = p_dd;
}

void NavigationAgent2D::set_target_desired_distance(real_t p_dd) {
	target_desired_distance = p_dd;
}

void NavigationAgent2D::set_radius(real_t p_radius) {
	radius = p_radius;
	NavigationServer2D::get_singleton()->agent_set_radius(agent, radius);
}

void NavigationAgent2D::set_neighbor_distance(real_t p_distance) {
	neighbor_distance = p_distance;
	NavigationServer2D::get_singleton()->agent_set_neighbor_distance(agent, neighbor_distance);
}

void NavigationAgent2D::set_max_neighbors(int p_count) {
	max_neighbors = p_count;
	NavigationServer2D::get_singleton()->agent_set_max_neighbors(agent, max_neighbors);
}

void NavigationAgent2D::set_time_horizon(real_t p_time) {
	time_horizon = p_time;
	NavigationServer2D::get_singleton()->agent_set_time_horizon(agent, time_horizon);
}

void NavigationAgent2D::set_max_speed(real_t p_max_speed) {
	max_speed = p_max_speed;
	NavigationServer2D::get_singleton()->agent_set_max_speed(agent, max_speed);
}

void NavigationAgent2D::set_path_max_distance(real_t p_pmd) {
	path_max_distance = p_pmd;
}

real_t NavigationAgent2D::get_path_max_distance() {
	return path_max_distance;
}

void NavigationAgent2D::set_target_location(Vector2 p_location) {
	target_location = p_location;
	_request_repath();
}

Vector2 NavigationAgent2D::get_target_location() const {
	return target_location;
}

Vector2 NavigationAgent2D::get_next_location() {
	update_navigation();

	const Vector<Vector2> &navigation_path = navigation_result->get_path();
	if (navigation_path.size() == 0) {
		ERR_FAIL_COND_V_MSG(agent_parent == nullptr, Vector2(), "The agent has no parent.");
		return agent_parent->get_global_position();
	} else {
		return navigation_path[nav_path_index];
	}
}

const Vector<Vector2> &NavigationAgent2D::get_nav_path() const {
	return navigation_result->get_path();
}

real_t NavigationAgent2D::distance_to_target() const {
	ERR_FAIL_COND_V_MSG(agent_parent == nullptr, 0.0, "The agent has no parent.");
	return agent_parent->get_global_position().distance_to(target_location);
}

bool NavigationAgent2D::is_target_reached() const {
	return target_reached;
}

bool NavigationAgent2D::is_target_reachable() {
	return target_desired_distance >= get_final_location().distance_to(target_location);
}

bool NavigationAgent2D::is_navigation_finished() {
	update_navigation();
	return navigation_finished;
}

Vector2 NavigationAgent2D::get_final_location() {
	update_navigation();

	const Vector<Vector2> &navigation_path = navigation_result->get_path();
	if (navigation_path.size() == 0) {
		return Vector2();
	}
	return navigation_path[navigation_path.size() - 1];
}

void NavigationAgent2D::set_velocity(Vector2 p_velocity) {
	target_velocity = p_velocity;
	NavigationServer2D::get_singleton()->agent_set_target_velocity(agent, target_velocity);
	NavigationServer2D::get_singleton()->agent_set_velocity(agent, prev_safe_velocity);
	velocity_submitted = true;
}

void NavigationAgent2D::_avoidance_done(Vector3 p_new_velocity) {
	const Vector2 velocity = Vector2(p_new_velocity.x, p_new_velocity.z);
	prev_safe_velocity = velocity;

	if (!velocity_submitted) {
		target_velocity = Vector2();
		return;
	}
	velocity_submitted = false;

	emit_signal(SNAME("velocity_computed"), velocity);
}

PackedStringArray NavigationAgent2D::get_configuration_warnings() const {
	PackedStringArray warnings = Node::get_configuration_warnings();

	if (!Object::cast_to<Node2D>(get_parent())) {
		warnings.push_back(RTR("The NavigationAgent2D can be used only under a Node2D inheriting parent node."));
	}

	return warnings;
}

void NavigationAgent2D::update_navigation() {
	if (agent_parent == nullptr) {
		return;
	}
	if (!agent_parent->is_inside_tree()) {
		return;
	}
	if (update_frame_id == Engine::get_singleton()->get_physics_frames()) {
		return;
	}

	update_frame_id = Engine::get_singleton()->get_physics_frames();

	Vector2 origin = agent_parent->get_global_position();

	bool reload_path = false;

	if (NavigationServer2D::get_singleton()->agent_is_map_changed(agent)) {
		reload_path = true;
	} else if (navigation_result->get_path().size() == 0) {
		reload_path = true;
	} else {
		// Check if too far from the navigation path
		if (nav_path_index > 0) {
			const Vector<Vector2> &navigation_path = navigation_result->get_path();

			Vector2 segment[2];
			segment[0] = navigation_path[nav_path_index - 1];
			segment[1] = navigation_path[nav_path_index];
			Vector2 p = Geometry2D::get_closest_point_to_segment(origin, segment);
			if (origin.distance_to(p) >= path_max_distance) {
				// To faraway, reload path
				reload_path = true;
			}
		}
	}

	if (reload_path) {
		navigation_query->set_start_position(origin);
		navigation_query->set_target_position(target_location);
		navigation_query->set_navigation_layers(navigation_layers);

		if (map_override.is_valid()) {
			navigation_query->set_map(map_override);
		} else {
			navigation_query->set_map(agent_parent->get_world_2d()->get_navigation_map());
		}

		NavigationServer2D::get_singleton()->query_path(navigation_query, navigation_result);
		navigation_finished = false;
		nav_path_index = 0;
		emit_signal(SNAME("path_changed"));
	}

	if (navigation_result->get_path().size() == 0) {
		return;
	}

	// Check if we can advance the navigation path
	if (navigation_finished == false) {
		// Advances to the next far away location.
		const Vector<Vector2> &navigation_path = navigation_result->get_path();
		while (origin.distance_to(navigation_path[nav_path_index]) < path_desired_distance) {
			nav_path_index += 1;
			if (nav_path_index == navigation_path.size()) {
				_check_distance_to_target();
				nav_path_index -= 1;
				navigation_finished = true;
				emit_signal(SNAME("navigation_finished"));
				break;
			}
		}
	}
}

void NavigationAgent2D::_request_repath() {
	navigation_result->reset();
	target_reached = false;
	navigation_finished = false;
	update_frame_id = 0;
}

void NavigationAgent2D::_check_distance_to_target() {
	if (!target_reached) {
		if (distance_to_target() < target_desired_distance) {
			target_reached = true;
			emit_signal(SNAME("target_reached"));
		}
	}
}
