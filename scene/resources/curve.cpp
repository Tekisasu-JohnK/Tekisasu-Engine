/*************************************************************************/
/*  curve.cpp                                                            */
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

#include "curve.h"

#include "core/core_string_names.h"

const char *Curve::SIGNAL_RANGE_CHANGED = "range_changed";

Curve::Curve() {
}

void Curve::set_point_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	if (_points.size() >= p_count) {
		_points.resize(p_count);
		mark_dirty();
	} else {
		for (int i = p_count - _points.size(); i > 0; i--) {
			_add_point(Vector2());
		}
	}
	notify_property_list_changed();
}

int Curve::_add_point(Vector2 p_position, real_t p_left_tangent, real_t p_right_tangent, TangentMode p_left_mode, TangentMode p_right_mode) {
	// Add a point and preserve order

	// Curve bounds is in 0..1
	if (p_position.x > MAX_X) {
		p_position.x = MAX_X;
	} else if (p_position.x < MIN_X) {
		p_position.x = MIN_X;
	}

	int ret = -1;

	if (_points.size() == 0) {
		_points.push_back(Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
		ret = 0;

	} else if (_points.size() == 1) {
		// TODO Is the `else` able to handle this block already?

		real_t diff = p_position.x - _points[0].position.x;

		if (diff > 0) {
			_points.push_back(Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 1;
		} else {
			_points.insert(0, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 0;
		}

	} else {
		int i = get_index(p_position.x);

		if (i == 0 && p_position.x < _points[0].position.x) {
			// Insert before anything else
			_points.insert(0, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = 0;
		} else {
			// Insert between i and i+1
			++i;
			_points.insert(i, Point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode));
			ret = i;
		}
	}

	update_auto_tangents(ret);

	mark_dirty();

	return ret;
}

int Curve::add_point(Vector2 p_position, real_t p_left_tangent, real_t p_right_tangent, TangentMode p_left_mode, TangentMode p_right_mode) {
	int ret = _add_point(p_position, p_left_tangent, p_right_tangent, p_left_mode, p_right_mode);
	notify_property_list_changed();

	return ret;
}

int Curve::get_index(real_t p_offset) const {
	// Lower-bound float binary search

	int imin = 0;
	int imax = _points.size() - 1;

	while (imax - imin > 1) {
		int m = (imin + imax) / 2;

		real_t a = _points[m].position.x;
		real_t b = _points[m + 1].position.x;

		if (a < p_offset && b < p_offset) {
			imin = m;

		} else if (a > p_offset) {
			imax = m;

		} else {
			return m;
		}
	}

	// Will happen if the offset is out of bounds
	if (p_offset > _points[imax].position.x) {
		return imax;
	}
	return imin;
}

void Curve::clean_dupes() {
	bool dirty = false;

	for (int i = 1; i < _points.size(); ++i) {
		real_t diff = _points[i - 1].position.x - _points[i].position.x;
		if (diff <= CMP_EPSILON) {
			_points.remove_at(i);
			--i;
			dirty = true;
		}
	}

	if (dirty) {
		mark_dirty();
	}
}

void Curve::set_point_left_tangent(int p_index, real_t p_tangent) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.write[p_index].left_tangent = p_tangent;
	_points.write[p_index].left_mode = TANGENT_FREE;
	mark_dirty();
}

void Curve::set_point_right_tangent(int p_index, real_t p_tangent) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.write[p_index].right_tangent = p_tangent;
	_points.write[p_index].right_mode = TANGENT_FREE;
	mark_dirty();
}

void Curve::set_point_left_mode(int p_index, TangentMode p_mode) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.write[p_index].left_mode = p_mode;
	if (p_index > 0) {
		if (p_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - _points[p_index].position).normalized();
			_points.write[p_index].left_tangent = v.y / v.x;
		}
	}
	mark_dirty();
}

void Curve::set_point_right_mode(int p_index, TangentMode p_mode) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.write[p_index].right_mode = p_mode;
	if (p_index + 1 < _points.size()) {
		if (p_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - _points[p_index].position).normalized();
			_points.write[p_index].right_tangent = v.y / v.x;
		}
	}
	mark_dirty();
}

real_t Curve::get_point_left_tangent(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), 0);
	return _points[p_index].left_tangent;
}

real_t Curve::get_point_right_tangent(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), 0);
	return _points[p_index].right_tangent;
}

Curve::TangentMode Curve::get_point_left_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), TANGENT_FREE);
	return _points[p_index].left_mode;
}

Curve::TangentMode Curve::get_point_right_mode(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), TANGENT_FREE);
	return _points[p_index].right_mode;
}

void Curve::_remove_point(int p_index) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.remove_at(p_index);
	mark_dirty();
}

void Curve::remove_point(int p_index) {
	_remove_point(p_index);
	notify_property_list_changed();
}

void Curve::clear_points() {
	_points.clear();
	mark_dirty();
	notify_property_list_changed();
}

void Curve::set_point_value(int p_index, real_t p_position) {
	ERR_FAIL_INDEX(p_index, _points.size());
	_points.write[p_index].position.y = p_position;
	update_auto_tangents(p_index);
	mark_dirty();
}

int Curve::set_point_offset(int p_index, real_t p_offset) {
	ERR_FAIL_INDEX_V(p_index, _points.size(), -1);
	Point p = _points[p_index];
	_remove_point(p_index);
	int i = _add_point(Vector2(p_offset, p.position.y));
	_points.write[i].left_tangent = p.left_tangent;
	_points.write[i].right_tangent = p.right_tangent;
	_points.write[i].left_mode = p.left_mode;
	_points.write[i].right_mode = p.right_mode;
	if (p_index != i) {
		update_auto_tangents(p_index);
	}
	update_auto_tangents(i);
	return i;
}

Vector2 Curve::get_point_position(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), Vector2(0, 0));
	return _points[p_index].position;
}

Curve::Point Curve::get_point(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, _points.size(), Point());
	return _points[p_index];
}

void Curve::update_auto_tangents(int p_index) {
	Point &p = _points.write[p_index];

	if (p_index > 0) {
		if (p.left_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - p.position).normalized();
			p.left_tangent = v.y / v.x;
		}
		if (_points[p_index - 1].right_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index - 1].position - p.position).normalized();
			_points.write[p_index - 1].right_tangent = v.y / v.x;
		}
	}

	if (p_index + 1 < _points.size()) {
		if (p.right_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - p.position).normalized();
			p.right_tangent = v.y / v.x;
		}
		if (_points[p_index + 1].left_mode == TANGENT_LINEAR) {
			Vector2 v = (_points[p_index + 1].position - p.position).normalized();
			_points.write[p_index + 1].left_tangent = v.y / v.x;
		}
	}
}

#define MIN_Y_RANGE 0.01

void Curve::set_min_value(real_t p_min) {
	if (_minmax_set_once & 0b11 && p_min > _max_value - MIN_Y_RANGE) {
		_min_value = _max_value - MIN_Y_RANGE;
	} else {
		_minmax_set_once |= 0b10; // first bit is "min set"
		_min_value = p_min;
	}
	// Note: min and max are indicative values,
	// it's still possible that existing points are out of range at this point.
	emit_signal(SNAME(SIGNAL_RANGE_CHANGED));
}

void Curve::set_max_value(real_t p_max) {
	if (_minmax_set_once & 0b11 && p_max < _min_value + MIN_Y_RANGE) {
		_max_value = _min_value + MIN_Y_RANGE;
	} else {
		_minmax_set_once |= 0b01; // second bit is "max set"
		_max_value = p_max;
	}
	emit_signal(SNAME(SIGNAL_RANGE_CHANGED));
}

real_t Curve::sample(real_t p_offset) const {
	if (_points.size() == 0) {
		return 0;
	}
	if (_points.size() == 1) {
		return _points[0].position.y;
	}

	int i = get_index(p_offset);

	if (i == _points.size() - 1) {
		return _points[i].position.y;
	}

	real_t local = p_offset - _points[i].position.x;

	if (i == 0 && local <= 0) {
		return _points[0].position.y;
	}

	return sample_local_nocheck(i, local);
}

real_t Curve::sample_local_nocheck(int p_index, real_t p_local_offset) const {
	const Point a = _points[p_index];
	const Point b = _points[p_index + 1];

	/* Cubic bezier
	 *
	 *       ac-----bc
	 *      /         \
	 *     /           \     Here with a.right_tangent > 0
	 *    /             \    and b.left_tangent < 0
	 *   /               \
	 *  a                 b
	 *
	 *  |-d1--|-d2--|-d3--|
	 *
	 * d1 == d2 == d3 == d / 3
	 */

	// Control points are chosen at equal distances
	real_t d = b.position.x - a.position.x;
	if (Math::is_zero_approx(d)) {
		return b.position.y;
	}
	p_local_offset /= d;
	d /= 3.0;
	real_t yac = a.position.y + d * a.right_tangent;
	real_t ybc = b.position.y - d * b.left_tangent;

	real_t y = Math::bezier_interpolate(a.position.y, yac, ybc, b.position.y, p_local_offset);

	return y;
}

void Curve::mark_dirty() {
	_baked_cache_dirty = true;
	emit_signal(CoreStringNames::get_singleton()->changed);
}

Array Curve::get_data() const {
	Array output;
	const unsigned int ELEMS = 5;
	output.resize(_points.size() * ELEMS);

	for (int j = 0; j < _points.size(); ++j) {
		const Point p = _points[j];
		int i = j * ELEMS;

		output[i] = p.position;
		output[i + 1] = p.left_tangent;
		output[i + 2] = p.right_tangent;
		output[i + 3] = p.left_mode;
		output[i + 4] = p.right_mode;
	}

	return output;
}

void Curve::set_data(const Array p_input) {
	const unsigned int ELEMS = 5;
	ERR_FAIL_COND(p_input.size() % ELEMS != 0);

	_points.clear();

	// Validate input
	for (int i = 0; i < p_input.size(); i += ELEMS) {
		ERR_FAIL_COND(p_input[i].get_type() != Variant::VECTOR2);
		ERR_FAIL_COND(!p_input[i + 1].is_num());
		ERR_FAIL_COND(p_input[i + 2].get_type() != Variant::FLOAT);

		ERR_FAIL_COND(p_input[i + 3].get_type() != Variant::INT);
		int left_mode = p_input[i + 3];
		ERR_FAIL_COND(left_mode < 0 || left_mode >= TANGENT_MODE_COUNT);

		ERR_FAIL_COND(p_input[i + 4].get_type() != Variant::INT);
		int right_mode = p_input[i + 4];
		ERR_FAIL_COND(right_mode < 0 || right_mode >= TANGENT_MODE_COUNT);
	}

	_points.resize(p_input.size() / ELEMS);

	for (int j = 0; j < _points.size(); ++j) {
		Point &p = _points.write[j];
		int i = j * ELEMS;

		p.position = p_input[i];
		p.left_tangent = p_input[i + 1];
		p.right_tangent = p_input[i + 2];
		int left_mode = p_input[i + 3];
		int right_mode = p_input[i + 4];
		p.left_mode = (TangentMode)left_mode;
		p.right_mode = (TangentMode)right_mode;
	}

	mark_dirty();
	notify_property_list_changed();
}

void Curve::bake() {
	_baked_cache.clear();

	_baked_cache.resize(_bake_resolution);

	for (int i = 1; i < _bake_resolution - 1; ++i) {
		real_t x = i / static_cast<real_t>(_bake_resolution);
		real_t y = sample(x);
		_baked_cache.write[i] = y;
	}

	if (_points.size() != 0) {
		_baked_cache.write[0] = _points[0].position.y;
		_baked_cache.write[_baked_cache.size() - 1] = _points[_points.size() - 1].position.y;
	}

	_baked_cache_dirty = false;
}

void Curve::set_bake_resolution(int p_resolution) {
	ERR_FAIL_COND(p_resolution < 1);
	ERR_FAIL_COND(p_resolution > 1000);
	_bake_resolution = p_resolution;
	_baked_cache_dirty = true;
}

real_t Curve::sample_baked(real_t p_offset) const {
	if (_baked_cache_dirty) {
		// Last-second bake if not done already
		const_cast<Curve *>(this)->bake();
	}

	// Special cases if the cache is too small
	if (_baked_cache.size() == 0) {
		if (_points.size() == 0) {
			return 0;
		}
		return _points[0].position.y;
	} else if (_baked_cache.size() == 1) {
		return _baked_cache[0];
	}

	// Get interpolation index
	real_t fi = p_offset * _baked_cache.size();
	int i = Math::floor(fi);
	if (i < 0) {
		i = 0;
		fi = 0;
	} else if (i >= _baked_cache.size()) {
		i = _baked_cache.size() - 1;
		fi = 0;
	}

	// Sample
	if (i + 1 < _baked_cache.size()) {
		real_t t = fi - i;
		return Math::lerp(_baked_cache[i], _baked_cache[i + 1], t);
	} else {
		return _baked_cache[_baked_cache.size() - 1];
	}
}

void Curve::ensure_default_setup(real_t p_min, real_t p_max) {
	if (_points.size() == 0 && _min_value == 0 && _max_value == 1) {
		add_point(Vector2(0, 1));
		add_point(Vector2(1, 1));
		set_min_value(p_min);
		set_max_value(p_max);
	}
}

bool Curve::_set(const StringName &p_name, const Variant &p_value) {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			Vector2 position = p_value.operator Vector2();
			set_point_offset(point_index, position.x);
			set_point_value(point_index, position.y);
			return true;
		} else if (property == "left_tangent") {
			set_point_left_tangent(point_index, p_value);
			return true;
		} else if (property == "left_mode") {
			int mode = p_value;
			set_point_left_mode(point_index, (TangentMode)mode);
			return true;
		} else if (property == "right_tangent") {
			set_point_right_tangent(point_index, p_value);
			return true;
		} else if (property == "right_mode") {
			int mode = p_value;
			set_point_right_mode(point_index, (TangentMode)mode);
			return true;
		}
	}
	return false;
}

bool Curve::_get(const StringName &p_name, Variant &r_ret) const {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			r_ret = get_point_position(point_index);
			return true;
		} else if (property == "left_tangent") {
			r_ret = get_point_left_tangent(point_index);
			return true;
		} else if (property == "left_mode") {
			r_ret = get_point_left_mode(point_index);
			return true;
		} else if (property == "right_tangent") {
			r_ret = get_point_right_tangent(point_index);
			return true;
		} else if (property == "right_mode") {
			r_ret = get_point_right_mode(point_index);
			return true;
		}
	}
	return false;
}

void Curve::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < _points.size(); i++) {
		PropertyInfo pi = PropertyInfo(Variant::VECTOR2, vformat("point_%d/position", i));
		pi.usage &= ~PROPERTY_USAGE_STORAGE;
		p_list->push_back(pi);

		if (i != 0) {
			pi = PropertyInfo(Variant::FLOAT, vformat("point_%d/left_tangent", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);

			pi = PropertyInfo(Variant::INT, vformat("point_%d/left_mode", i), PROPERTY_HINT_ENUM, "Free,Linear");
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}

		if (i != _points.size() - 1) {
			pi = PropertyInfo(Variant::FLOAT, vformat("point_%d/right_tangent", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);

			pi = PropertyInfo(Variant::INT, vformat("point_%d/right_mode", i), PROPERTY_HINT_ENUM, "Free,Linear");
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}
	}
}

void Curve::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_point_count"), &Curve::get_point_count);
	ClassDB::bind_method(D_METHOD("set_point_count", "count"), &Curve::set_point_count);
	ClassDB::bind_method(D_METHOD("add_point", "position", "left_tangent", "right_tangent", "left_mode", "right_mode"), &Curve::add_point, DEFVAL(0), DEFVAL(0), DEFVAL(TANGENT_FREE), DEFVAL(TANGENT_FREE));
	ClassDB::bind_method(D_METHOD("remove_point", "index"), &Curve::remove_point);
	ClassDB::bind_method(D_METHOD("clear_points"), &Curve::clear_points);
	ClassDB::bind_method(D_METHOD("get_point_position", "index"), &Curve::get_point_position);
	ClassDB::bind_method(D_METHOD("set_point_value", "index", "y"), &Curve::set_point_value);
	ClassDB::bind_method(D_METHOD("set_point_offset", "index", "offset"), &Curve::set_point_offset);
	ClassDB::bind_method(D_METHOD("sample", "offset"), &Curve::sample);
	ClassDB::bind_method(D_METHOD("sample_baked", "offset"), &Curve::sample_baked);
	ClassDB::bind_method(D_METHOD("get_point_left_tangent", "index"), &Curve::get_point_left_tangent);
	ClassDB::bind_method(D_METHOD("get_point_right_tangent", "index"), &Curve::get_point_right_tangent);
	ClassDB::bind_method(D_METHOD("get_point_left_mode", "index"), &Curve::get_point_left_mode);
	ClassDB::bind_method(D_METHOD("get_point_right_mode", "index"), &Curve::get_point_right_mode);
	ClassDB::bind_method(D_METHOD("set_point_left_tangent", "index", "tangent"), &Curve::set_point_left_tangent);
	ClassDB::bind_method(D_METHOD("set_point_right_tangent", "index", "tangent"), &Curve::set_point_right_tangent);
	ClassDB::bind_method(D_METHOD("set_point_left_mode", "index", "mode"), &Curve::set_point_left_mode);
	ClassDB::bind_method(D_METHOD("set_point_right_mode", "index", "mode"), &Curve::set_point_right_mode);
	ClassDB::bind_method(D_METHOD("get_min_value"), &Curve::get_min_value);
	ClassDB::bind_method(D_METHOD("set_min_value", "min"), &Curve::set_min_value);
	ClassDB::bind_method(D_METHOD("get_max_value"), &Curve::get_max_value);
	ClassDB::bind_method(D_METHOD("set_max_value", "max"), &Curve::set_max_value);
	ClassDB::bind_method(D_METHOD("clean_dupes"), &Curve::clean_dupes);
	ClassDB::bind_method(D_METHOD("bake"), &Curve::bake);
	ClassDB::bind_method(D_METHOD("get_bake_resolution"), &Curve::get_bake_resolution);
	ClassDB::bind_method(D_METHOD("set_bake_resolution", "resolution"), &Curve::set_bake_resolution);
	ClassDB::bind_method(D_METHOD("_get_data"), &Curve::get_data);
	ClassDB::bind_method(D_METHOD("_set_data", "data"), &Curve::set_data);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "min_value", PROPERTY_HINT_RANGE, "-1024,1024,0.01"), "set_min_value", "get_min_value");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "max_value", PROPERTY_HINT_RANGE, "-1024,1024,0.01"), "set_max_value", "get_max_value");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "bake_resolution", PROPERTY_HINT_RANGE, "1,1000,1"), "set_bake_resolution", "get_bake_resolution");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "_set_data", "_get_data");
	ADD_ARRAY_COUNT("Points", "point_count", "set_point_count", "get_point_count", "point_");

	ADD_SIGNAL(MethodInfo(SIGNAL_RANGE_CHANGED));

	BIND_ENUM_CONSTANT(TANGENT_FREE);
	BIND_ENUM_CONSTANT(TANGENT_LINEAR);
	BIND_ENUM_CONSTANT(TANGENT_MODE_COUNT);
}

int Curve2D::get_point_count() const {
	return points.size();
}

void Curve2D::set_point_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	if (points.size() >= p_count) {
		points.resize(p_count);
		mark_dirty();
	} else {
		for (int i = p_count - points.size(); i > 0; i--) {
			_add_point(Vector2());
		}
	}
	notify_property_list_changed();
}

void Curve2D::_add_point(const Vector2 &p_position, const Vector2 &p_in, const Vector2 &p_out, int p_atpos) {
	Point n;
	n.position = p_position;
	n.in = p_in;
	n.out = p_out;
	if (p_atpos >= 0 && p_atpos < points.size()) {
		points.insert(p_atpos, n);
	} else {
		points.push_back(n);
	}

	mark_dirty();
}

void Curve2D::add_point(const Vector2 &p_position, const Vector2 &p_in, const Vector2 &p_out, int p_atpos) {
	_add_point(p_position, p_in, p_out, p_atpos);
	notify_property_list_changed();
}

void Curve2D::set_point_position(int p_index, const Vector2 &p_position) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].position = p_position;
	mark_dirty();
}

Vector2 Curve2D::get_point_position(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector2());
	return points[p_index].position;
}

void Curve2D::set_point_in(int p_index, const Vector2 &p_in) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].in = p_in;
	mark_dirty();
}

Vector2 Curve2D::get_point_in(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector2());
	return points[p_index].in;
}

void Curve2D::set_point_out(int p_index, const Vector2 &p_out) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].out = p_out;
	mark_dirty();
}

Vector2 Curve2D::get_point_out(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector2());
	return points[p_index].out;
}

void Curve2D::_remove_point(int p_index) {
	ERR_FAIL_INDEX(p_index, points.size());
	points.remove_at(p_index);
	mark_dirty();
}

void Curve2D::remove_point(int p_index) {
	_remove_point(p_index);
	notify_property_list_changed();
}

void Curve2D::clear_points() {
	if (!points.is_empty()) {
		points.clear();
		mark_dirty();
		notify_property_list_changed();
	}
}

Vector2 Curve2D::sample(int p_index, const real_t p_offset) const {
	int pc = points.size();
	ERR_FAIL_COND_V(pc == 0, Vector2());

	if (p_index >= pc - 1) {
		return points[pc - 1].position;
	} else if (p_index < 0) {
		return points[0].position;
	}

	Vector2 p0 = points[p_index].position;
	Vector2 p1 = p0 + points[p_index].out;
	Vector2 p3 = points[p_index + 1].position;
	Vector2 p2 = p3 + points[p_index + 1].in;

	return p0.bezier_interpolate(p1, p2, p3, p_offset);
}

Vector2 Curve2D::samplef(real_t p_findex) const {
	if (p_findex < 0) {
		p_findex = 0;
	} else if (p_findex >= points.size()) {
		p_findex = points.size();
	}

	return sample((int)p_findex, Math::fmod(p_findex, (real_t)1.0));
}

void Curve2D::mark_dirty() {
	baked_cache_dirty = true;
	emit_signal(CoreStringNames::get_singleton()->changed);
}

void Curve2D::_bake_segment2d(RBMap<real_t, Vector2> &r_bake, real_t p_begin, real_t p_end, const Vector2 &p_a, const Vector2 &p_out, const Vector2 &p_b, const Vector2 &p_in, int p_depth, int p_max_depth, real_t p_tol) const {
	real_t mp = p_begin + (p_end - p_begin) * 0.5;
	Vector2 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector2 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
	Vector2 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	Vector2 na = (mid - beg).normalized();
	Vector2 nb = (end - mid).normalized();
	real_t dp = na.dot(nb);

	if (dp < Math::cos(Math::deg_to_rad(p_tol))) {
		r_bake[mp] = mid;
	}

	if (p_depth < p_max_depth) {
		_bake_segment2d(r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
		_bake_segment2d(r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
	}
}

void Curve2D::_bake() const {
	if (!baked_cache_dirty) {
		return;
	}

	baked_max_ofs = 0;
	baked_cache_dirty = false;

	if (points.size() == 0) {
		baked_point_cache.clear();
		baked_dist_cache.clear();
		return;
	}

	if (points.size() == 1) {
		baked_point_cache.resize(1);
		baked_point_cache.set(0, points[0].position);

		baked_dist_cache.resize(1);
		baked_dist_cache.set(0, 0.0);
		return;
	}

	Vector2 position = points[0].position;
	real_t dist = 0.0;

	List<Vector2> pointlist;
	List<real_t> distlist;

	// Start always from origin.
	pointlist.push_back(position);
	distlist.push_back(0.0);

	for (int i = 0; i < points.size() - 1; i++) {
		real_t step = 0.1; // at least 10 substeps ought to be enough?
		real_t p = 0.0;

		while (p < 1.0) {
			real_t np = p + step;
			if (np > 1.0) {
				np = 1.0;
			}

			Vector2 npp = points[i].position.bezier_interpolate(points[i].position + points[i].out, points[i + 1].position + points[i + 1].in, points[i + 1].position, np);
			real_t d = position.distance_to(npp);

			if (d > bake_interval) {
				// OK! between P and NP there _has_ to be Something, let's go searching!

				int iterations = 10; //lots of detail!

				real_t low = p;
				real_t hi = np;
				real_t mid = low + (hi - low) * 0.5;

				for (int j = 0; j < iterations; j++) {
					npp = points[i].position.bezier_interpolate(points[i].position + points[i].out, points[i + 1].position + points[i + 1].in, points[i + 1].position, mid);
					d = position.distance_to(npp);

					if (bake_interval < d) {
						hi = mid;
					} else {
						low = mid;
					}
					mid = low + (hi - low) * 0.5;
				}

				position = npp;
				p = mid;
				dist += d;

				pointlist.push_back(position);
				distlist.push_back(dist);
			} else {
				p = np;
			}
		}

		Vector2 npp = points[i + 1].position;
		real_t d = position.distance_to(npp);

		position = npp;
		dist += d;

		pointlist.push_back(position);
		distlist.push_back(dist);
	}

	baked_max_ofs = dist;

	baked_point_cache.resize(pointlist.size());
	baked_dist_cache.resize(distlist.size());

	Vector2 *w = baked_point_cache.ptrw();
	real_t *wd = baked_dist_cache.ptrw();

	for (int i = 0; i < pointlist.size(); i++) {
		w[i] = pointlist[i];
		wd[i] = distlist[i];
	}
}

real_t Curve2D::get_baked_length() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_max_ofs;
}

Vector2 Curve2D::sample_baked(real_t p_offset, bool p_cubic) const {
	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector2(), "No points in Curve2D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector2 *r = baked_point_cache.ptr();

	if (p_offset < 0) {
		return r[0];
	}
	if (p_offset >= baked_max_ofs) {
		return r[pc - 1];
	}

	int start = 0;
	int end = pc;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		} else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(p_offset < offset_begin || p_offset > offset_end, Vector2(), "Couldn't find baked segment.");

	real_t frac = (p_offset - offset_begin) / idx_interval;

	if (p_cubic) {
		Vector2 pre = idx > 0 ? r[idx - 1] : r[idx];
		Vector2 post = (idx < (pc - 2)) ? r[idx + 2] : r[idx + 1];
		return r[idx].cubic_interpolate(r[idx + 1], pre, post, frac);
	} else {
		return r[idx].lerp(r[idx + 1], frac);
	}
}

PackedVector2Array Curve2D::get_baked_points() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_point_cache;
}

void Curve2D::set_bake_interval(real_t p_tolerance) {
	bake_interval = p_tolerance;
	mark_dirty();
}

real_t Curve2D::get_bake_interval() const {
	return bake_interval;
}

Vector2 Curve2D::get_closest_point(const Vector2 &p_to_point) const {
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector2(), "No points in Curve2D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector2 *r = baked_point_cache.ptr();

	Vector2 nearest;
	real_t nearest_dist = -1.0f;

	for (int i = 0; i < pc - 1; i++) {
		Vector2 origin = r[i];
		Vector2 direction = (r[i + 1] - origin) / bake_interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, bake_interval);
		Vector2 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = proj;
			nearest_dist = dist;
		}
	}

	return nearest;
}

real_t Curve2D::get_closest_offset(const Vector2 &p_to_point) const {
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0.0f, "No points in Curve2D.");

	if (pc == 1) {
		return 0.0f;
	}

	const Vector2 *r = baked_point_cache.ptr();

	real_t nearest = 0.0f;
	real_t nearest_dist = -1.0f;
	real_t offset = 0.0f;

	for (int i = 0; i < pc - 1; i++) {
		Vector2 origin = r[i];
		Vector2 direction = (r[i + 1] - origin) / bake_interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, bake_interval);
		Vector2 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = offset + d;
			nearest_dist = dist;
		}

		offset += bake_interval;
	}

	return nearest;
}

Dictionary Curve2D::_get_data() const {
	Dictionary dc;

	PackedVector2Array d;
	d.resize(points.size() * 3);
	Vector2 *w = d.ptrw();

	for (int i = 0; i < points.size(); i++) {
		w[i * 3 + 0] = points[i].in;
		w[i * 3 + 1] = points[i].out;
		w[i * 3 + 2] = points[i].position;
	}

	dc["points"] = d;

	return dc;
}

void Curve2D::_set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(!p_data.has("points"));

	PackedVector2Array rp = p_data["points"];
	int pc = rp.size();
	ERR_FAIL_COND(pc % 3 != 0);
	points.resize(pc / 3);
	const Vector2 *r = rp.ptr();

	for (int i = 0; i < points.size(); i++) {
		points.write[i].in = r[i * 3 + 0];
		points.write[i].out = r[i * 3 + 1];
		points.write[i].position = r[i * 3 + 2];
	}

	mark_dirty();
	notify_property_list_changed();
}

PackedVector2Array Curve2D::tessellate(int p_max_stages, real_t p_tolerance) const {
	PackedVector2Array tess;

	if (points.size() == 0) {
		return tess;
	}

	// The current implementation requires a sorted map.
	Vector<RBMap<real_t, Vector2>> midpoints;

	midpoints.resize(points.size() - 1);

	int pc = 1;
	for (int i = 0; i < points.size() - 1; i++) {
		_bake_segment2d(midpoints.write[i], 0, 1, points[i].position, points[i].out, points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_tolerance);
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector2 *bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (int i = 0; i < points.size() - 1; i++) {
		for (const KeyValue<real_t, Vector2> &E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		bpw[pidx] = points[i + 1].position;
	}

	return tess;
}

bool Curve2D::_set(const StringName &p_name, const Variant &p_value) {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			set_point_position(point_index, p_value);
			return true;
		} else if (property == "in") {
			set_point_in(point_index, p_value);
			return true;
		} else if (property == "out") {
			set_point_out(point_index, p_value);
			return true;
		}
	}
	return false;
}

bool Curve2D::_get(const StringName &p_name, Variant &r_ret) const {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			r_ret = get_point_position(point_index);
			return true;
		} else if (property == "in") {
			r_ret = get_point_in(point_index);
			return true;
		} else if (property == "out") {
			r_ret = get_point_out(point_index);
			return true;
		}
	}
	return false;
}

void Curve2D::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < points.size(); i++) {
		PropertyInfo pi = PropertyInfo(Variant::VECTOR2, vformat("point_%d/position", i));
		pi.usage &= ~PROPERTY_USAGE_STORAGE;
		p_list->push_back(pi);

		if (i != 0) {
			pi = PropertyInfo(Variant::VECTOR2, vformat("point_%d/in", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}

		if (i != points.size() - 1) {
			pi = PropertyInfo(Variant::VECTOR2, vformat("point_%d/out", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}
	}
}

void Curve2D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_point_count"), &Curve2D::get_point_count);
	ClassDB::bind_method(D_METHOD("set_point_count", "count"), &Curve2D::set_point_count);
	ClassDB::bind_method(D_METHOD("add_point", "position", "in", "out", "index"), &Curve2D::add_point, DEFVAL(Vector2()), DEFVAL(Vector2()), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("set_point_position", "idx", "position"), &Curve2D::set_point_position);
	ClassDB::bind_method(D_METHOD("get_point_position", "idx"), &Curve2D::get_point_position);
	ClassDB::bind_method(D_METHOD("set_point_in", "idx", "position"), &Curve2D::set_point_in);
	ClassDB::bind_method(D_METHOD("get_point_in", "idx"), &Curve2D::get_point_in);
	ClassDB::bind_method(D_METHOD("set_point_out", "idx", "position"), &Curve2D::set_point_out);
	ClassDB::bind_method(D_METHOD("get_point_out", "idx"), &Curve2D::get_point_out);
	ClassDB::bind_method(D_METHOD("remove_point", "idx"), &Curve2D::remove_point);
	ClassDB::bind_method(D_METHOD("clear_points"), &Curve2D::clear_points);
	ClassDB::bind_method(D_METHOD("sample", "idx", "t"), &Curve2D::sample);
	ClassDB::bind_method(D_METHOD("samplef", "fofs"), &Curve2D::samplef);
	//ClassDB::bind_method(D_METHOD("bake","subdivs"),&Curve2D::bake,DEFVAL(10));
	ClassDB::bind_method(D_METHOD("set_bake_interval", "distance"), &Curve2D::set_bake_interval);
	ClassDB::bind_method(D_METHOD("get_bake_interval"), &Curve2D::get_bake_interval);

	ClassDB::bind_method(D_METHOD("get_baked_length"), &Curve2D::get_baked_length);
	ClassDB::bind_method(D_METHOD("sample_baked", "offset", "cubic"), &Curve2D::sample_baked, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_baked_points"), &Curve2D::get_baked_points);
	ClassDB::bind_method(D_METHOD("get_closest_point", "to_point"), &Curve2D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_closest_offset", "to_point"), &Curve2D::get_closest_offset);
	ClassDB::bind_method(D_METHOD("tessellate", "max_stages", "tolerance_degrees"), &Curve2D::tessellate, DEFVAL(5), DEFVAL(4));

	ClassDB::bind_method(D_METHOD("_get_data"), &Curve2D::_get_data);
	ClassDB::bind_method(D_METHOD("_set_data", "data"), &Curve2D::_set_data);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bake_interval", PROPERTY_HINT_RANGE, "0.01,512,0.01"), "set_bake_interval", "get_bake_interval");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "_set_data", "_get_data");
	ADD_ARRAY_COUNT("Points", "point_count", "set_point_count", "get_point_count", "point_");
}

Curve2D::Curve2D() {}

/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/

int Curve3D::get_point_count() const {
	return points.size();
}

void Curve3D::set_point_count(int p_count) {
	ERR_FAIL_COND(p_count < 0);
	if (points.size() >= p_count) {
		points.resize(p_count);
		mark_dirty();
	} else {
		for (int i = p_count - points.size(); i > 0; i--) {
			_add_point(Vector3());
		}
	}
	notify_property_list_changed();
}

void Curve3D::_add_point(const Vector3 &p_position, const Vector3 &p_in, const Vector3 &p_out, int p_atpos) {
	Point n;
	n.position = p_position;
	n.in = p_in;
	n.out = p_out;
	if (p_atpos >= 0 && p_atpos < points.size()) {
		points.insert(p_atpos, n);
	} else {
		points.push_back(n);
	}

	mark_dirty();
}

void Curve3D::add_point(const Vector3 &p_position, const Vector3 &p_in, const Vector3 &p_out, int p_atpos) {
	_add_point(p_position, p_in, p_out, p_atpos);
	notify_property_list_changed();
}

void Curve3D::set_point_position(int p_index, const Vector3 &p_position) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].position = p_position;
	mark_dirty();
}

Vector3 Curve3D::get_point_position(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector3());
	return points[p_index].position;
}

void Curve3D::set_point_tilt(int p_index, real_t p_tilt) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].tilt = p_tilt;
	mark_dirty();
}

real_t Curve3D::get_point_tilt(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), 0);
	return points[p_index].tilt;
}

void Curve3D::set_point_in(int p_index, const Vector3 &p_in) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].in = p_in;
	mark_dirty();
}

Vector3 Curve3D::get_point_in(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector3());
	return points[p_index].in;
}

void Curve3D::set_point_out(int p_index, const Vector3 &p_out) {
	ERR_FAIL_INDEX(p_index, points.size());

	points.write[p_index].out = p_out;
	mark_dirty();
}

Vector3 Curve3D::get_point_out(int p_index) const {
	ERR_FAIL_INDEX_V(p_index, points.size(), Vector3());
	return points[p_index].out;
}

void Curve3D::_remove_point(int p_index) {
	ERR_FAIL_INDEX(p_index, points.size());
	points.remove_at(p_index);
	mark_dirty();
}

void Curve3D::remove_point(int p_index) {
	_remove_point(p_index);
	notify_property_list_changed();
}

void Curve3D::clear_points() {
	if (!points.is_empty()) {
		points.clear();
		mark_dirty();
		notify_property_list_changed();
	}
}

Vector3 Curve3D::sample(int p_index, real_t p_offset) const {
	int pc = points.size();
	ERR_FAIL_COND_V(pc == 0, Vector3());

	if (p_index >= pc - 1) {
		return points[pc - 1].position;
	} else if (p_index < 0) {
		return points[0].position;
	}

	Vector3 p0 = points[p_index].position;
	Vector3 p1 = p0 + points[p_index].out;
	Vector3 p3 = points[p_index + 1].position;
	Vector3 p2 = p3 + points[p_index + 1].in;

	return p0.bezier_interpolate(p1, p2, p3, p_offset);
}

Vector3 Curve3D::samplef(real_t p_findex) const {
	if (p_findex < 0) {
		p_findex = 0;
	} else if (p_findex >= points.size()) {
		p_findex = points.size();
	}

	return sample((int)p_findex, Math::fmod(p_findex, (real_t)1.0));
}

void Curve3D::mark_dirty() {
	baked_cache_dirty = true;
	emit_signal(CoreStringNames::get_singleton()->changed);
}

void Curve3D::_bake_segment3d(RBMap<real_t, Vector3> &r_bake, real_t p_begin, real_t p_end, const Vector3 &p_a, const Vector3 &p_out, const Vector3 &p_b, const Vector3 &p_in, int p_depth, int p_max_depth, real_t p_tol) const {
	real_t mp = p_begin + (p_end - p_begin) * 0.5;
	Vector3 beg = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_begin);
	Vector3 mid = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, mp);
	Vector3 end = p_a.bezier_interpolate(p_a + p_out, p_b + p_in, p_b, p_end);

	Vector3 na = (mid - beg).normalized();
	Vector3 nb = (end - mid).normalized();
	real_t dp = na.dot(nb);

	if (dp < Math::cos(Math::deg_to_rad(p_tol))) {
		r_bake[mp] = mid;
	}
	if (p_depth < p_max_depth) {
		_bake_segment3d(r_bake, p_begin, mp, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
		_bake_segment3d(r_bake, mp, p_end, p_a, p_out, p_b, p_in, p_depth + 1, p_max_depth, p_tol);
	}
}

void Curve3D::_bake() const {
	if (!baked_cache_dirty) {
		return;
	}

	baked_max_ofs = 0;
	baked_cache_dirty = false;

	if (points.size() == 0) {
		baked_point_cache.clear();
		baked_tilt_cache.clear();
		baked_up_vector_cache.clear();
		baked_dist_cache.clear();
		return;
	}

	if (points.size() == 1) {
		baked_point_cache.resize(1);
		baked_point_cache.set(0, points[0].position);
		baked_tilt_cache.resize(1);
		baked_tilt_cache.set(0, points[0].tilt);
		baked_dist_cache.resize(1);
		baked_dist_cache.set(0, 0.0);

		if (up_vector_enabled) {
			baked_up_vector_cache.resize(1);
			baked_up_vector_cache.set(0, Vector3(0, 1, 0));
		} else {
			baked_up_vector_cache.clear();
		}

		return;
	}

	Vector3 position = points[0].position;
	real_t dist = 0.0;
	List<Plane> pointlist;
	List<real_t> distlist;

	// Start always from origin.
	pointlist.push_back(Plane(position, points[0].tilt));
	distlist.push_back(0.0);

	for (int i = 0; i < points.size() - 1; i++) {
		real_t step = 0.1; // at least 10 substeps ought to be enough?
		real_t p = 0.0;

		while (p < 1.0) {
			real_t np = p + step;
			if (np > 1.0) {
				np = 1.0;
			}

			Vector3 npp = points[i].position.bezier_interpolate(points[i].position + points[i].out, points[i + 1].position + points[i + 1].in, points[i + 1].position, np);
			real_t d = position.distance_to(npp);

			if (d > bake_interval) {
				// OK! between P and NP there _has_ to be Something, let's go searching!

				int iterations = 10; //lots of detail!

				real_t low = p;
				real_t hi = np;
				real_t mid = low + (hi - low) * 0.5;

				for (int j = 0; j < iterations; j++) {
					npp = points[i].position.bezier_interpolate(points[i].position + points[i].out, points[i + 1].position + points[i + 1].in, points[i + 1].position, mid);
					d = position.distance_to(npp);

					if (bake_interval < d) {
						hi = mid;
					} else {
						low = mid;
					}
					mid = low + (hi - low) * 0.5;
				}

				position = npp;
				p = mid;
				Plane post;
				post.normal = position;
				post.d = Math::lerp(points[i].tilt, points[i + 1].tilt, mid);
				dist += d;

				pointlist.push_back(post);
				distlist.push_back(dist);
			} else {
				p = np;
			}
		}

		Vector3 npp = points[i + 1].position;
		real_t d = position.distance_to(npp);

		position = npp;
		Plane post;
		post.normal = position;
		post.d = points[i + 1].tilt;

		dist += d;

		pointlist.push_back(post);
		distlist.push_back(dist);
	}

	baked_max_ofs = dist;

	baked_point_cache.resize(pointlist.size());
	Vector3 *w = baked_point_cache.ptrw();
	int idx = 0;

	baked_tilt_cache.resize(pointlist.size());
	real_t *wt = baked_tilt_cache.ptrw();

	baked_up_vector_cache.resize(up_vector_enabled ? pointlist.size() : 0);
	Vector3 *up_write = baked_up_vector_cache.ptrw();

	baked_dist_cache.resize(pointlist.size());
	real_t *wd = baked_dist_cache.ptrw();

	Vector3 sideways;
	Vector3 up;
	Vector3 forward;

	Vector3 prev_sideways = Vector3(1, 0, 0);
	Vector3 prev_up = Vector3(0, 1, 0);
	Vector3 prev_forward = Vector3(0, 0, 1);

	for (const Plane &E : pointlist) {
		w[idx] = E.normal;
		wt[idx] = E.d;
		wd[idx] = distlist[idx];

		if (!up_vector_enabled) {
			idx++;
			continue;
		}

		forward = idx > 0 ? (w[idx] - w[idx - 1]).normalized() : prev_forward;

		real_t y_dot = prev_up.dot(forward);

		if (y_dot > (1.0f - CMP_EPSILON)) {
			sideways = prev_sideways;
			up = -prev_forward;
		} else if (y_dot < -(1.0f - CMP_EPSILON)) {
			sideways = prev_sideways;
			up = prev_forward;
		} else {
			sideways = prev_up.cross(forward).normalized();
			up = forward.cross(sideways).normalized();
		}

		if (idx == 1) {
			up_write[0] = up;
		}

		up_write[idx] = up;

		prev_sideways = sideways;
		prev_up = up;
		prev_forward = forward;

		idx++;
	}
}

real_t Curve3D::get_baked_length() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_max_ofs;
}

Vector3 Curve3D::sample_baked(real_t p_offset, bool p_cubic) const {
	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector3(), "No points in Curve3D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector3 *r = baked_point_cache.ptr();

	if (p_offset < 0) {
		return r[0];
	}
	if (p_offset >= baked_max_ofs) {
		return r[pc - 1];
	}

	int start = 0;
	int end = pc;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		} else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(p_offset < offset_begin || p_offset > offset_end, Vector3(), "Couldn't find baked segment.");

	real_t frac = (p_offset - offset_begin) / idx_interval;

	if (p_cubic) {
		Vector3 pre = idx > 0 ? r[idx - 1] : r[idx];
		Vector3 post = (idx < (pc - 2)) ? r[idx + 2] : r[idx + 1];
		return r[idx].cubic_interpolate(r[idx + 1], pre, post, frac);
	} else {
		return r[idx].lerp(r[idx + 1], frac);
	}
}

real_t Curve3D::sample_baked_tilt(real_t p_offset) const {
	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked tilts.
	int pc = baked_tilt_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0, "No tilts in Curve3D.");

	if (pc == 1) {
		return baked_tilt_cache.get(0);
	}

	const real_t *r = baked_tilt_cache.ptr();

	if (p_offset < 0) {
		return r[0];
	}
	if (p_offset >= baked_max_ofs) {
		return r[pc - 1];
	}

	int start = 0;
	int end = pc;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		} else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(p_offset < offset_begin || p_offset > offset_end, 0, "Couldn't find baked segment.");

	real_t frac = (p_offset - offset_begin) / idx_interval;

	return Math::lerp(r[idx], r[idx + 1], (real_t)frac);
}

Vector3 Curve3D::sample_baked_up_vector(real_t p_offset, bool p_apply_tilt) const {
	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked up vectors.
	int count = baked_up_vector_cache.size();
	ERR_FAIL_COND_V_MSG(count == 0, Vector3(0, 1, 0), "No up vectors in Curve3D.");

	if (count == 1) {
		return baked_up_vector_cache.get(0);
	}

	const Vector3 *r = baked_up_vector_cache.ptr();
	const Vector3 *rp = baked_point_cache.ptr();
	const real_t *rt = baked_tilt_cache.ptr();

	int start = 0;
	int end = count;
	int idx = (end + start) / 2;
	// Binary search to find baked points.
	while (start < idx) {
		real_t offset = baked_dist_cache[idx];
		if (p_offset <= offset) {
			end = idx;
		} else {
			start = idx;
		}
		idx = (end + start) / 2;
	}

	if (idx == count - 1) {
		return p_apply_tilt ? r[idx].rotated((rp[idx] - rp[idx - 1]).normalized(), rt[idx]) : r[idx];
	}

	real_t offset_begin = baked_dist_cache[idx];
	real_t offset_end = baked_dist_cache[idx + 1];

	real_t idx_interval = offset_end - offset_begin;
	ERR_FAIL_COND_V_MSG(p_offset < offset_begin || p_offset > offset_end, Vector3(0, 1, 0), "Couldn't find baked segment.");

	real_t frac = (p_offset - offset_begin) / idx_interval;

	Vector3 forward = (rp[idx + 1] - rp[idx]).normalized();
	Vector3 up = r[idx];
	Vector3 up1 = r[idx + 1];

	if (p_apply_tilt) {
		up.rotate(forward, rt[idx]);
		up1.rotate(idx + 2 >= count ? forward : (rp[idx + 2] - rp[idx + 1]).normalized(), rt[idx + 1]);
	}

	Vector3 axis = up.cross(up1);

	if (axis.length_squared() < CMP_EPSILON2) {
		axis = forward;
	} else {
		axis.normalize();
	}

	return up.rotated(axis, up.angle_to(up1) * frac);
}

PackedVector3Array Curve3D::get_baked_points() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_point_cache;
}

Vector<real_t> Curve3D::get_baked_tilts() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_tilt_cache;
}

PackedVector3Array Curve3D::get_baked_up_vectors() const {
	if (baked_cache_dirty) {
		_bake();
	}

	return baked_up_vector_cache;
}

Vector3 Curve3D::get_closest_point(const Vector3 &p_to_point) const {
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, Vector3(), "No points in Curve3D.");

	if (pc == 1) {
		return baked_point_cache.get(0);
	}

	const Vector3 *r = baked_point_cache.ptr();

	Vector3 nearest;
	real_t nearest_dist = -1.0f;

	for (int i = 0; i < pc - 1; i++) {
		Vector3 origin = r[i];
		Vector3 direction = (r[i + 1] - origin) / bake_interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, bake_interval);
		Vector3 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = proj;
			nearest_dist = dist;
		}
	}

	return nearest;
}

real_t Curve3D::get_closest_offset(const Vector3 &p_to_point) const {
	// Brute force method.

	if (baked_cache_dirty) {
		_bake();
	}

	// Validate: Curve may not have baked points.
	int pc = baked_point_cache.size();
	ERR_FAIL_COND_V_MSG(pc == 0, 0.0f, "No points in Curve3D.");

	if (pc == 1) {
		return 0.0f;
	}

	const Vector3 *r = baked_point_cache.ptr();

	real_t nearest = 0.0f;
	real_t nearest_dist = -1.0f;
	real_t offset = 0.0f;

	for (int i = 0; i < pc - 1; i++) {
		Vector3 origin = r[i];
		Vector3 direction = (r[i + 1] - origin) / bake_interval;

		real_t d = CLAMP((p_to_point - origin).dot(direction), 0.0f, bake_interval);
		Vector3 proj = origin + direction * d;

		real_t dist = proj.distance_squared_to(p_to_point);

		if (nearest_dist < 0.0f || dist < nearest_dist) {
			nearest = offset + d;
			nearest_dist = dist;
		}

		offset += bake_interval;
	}

	return nearest;
}

void Curve3D::set_bake_interval(real_t p_tolerance) {
	bake_interval = p_tolerance;
	mark_dirty();
}

real_t Curve3D::get_bake_interval() const {
	return bake_interval;
}

void Curve3D::set_up_vector_enabled(bool p_enable) {
	up_vector_enabled = p_enable;
	mark_dirty();
}

bool Curve3D::is_up_vector_enabled() const {
	return up_vector_enabled;
}

Dictionary Curve3D::_get_data() const {
	Dictionary dc;

	PackedVector3Array d;
	d.resize(points.size() * 3);
	Vector3 *w = d.ptrw();
	Vector<real_t> t;
	t.resize(points.size());
	real_t *wt = t.ptrw();

	for (int i = 0; i < points.size(); i++) {
		w[i * 3 + 0] = points[i].in;
		w[i * 3 + 1] = points[i].out;
		w[i * 3 + 2] = points[i].position;
		wt[i] = points[i].tilt;
	}

	dc["points"] = d;
	dc["tilts"] = t;

	return dc;
}

void Curve3D::_set_data(const Dictionary &p_data) {
	ERR_FAIL_COND(!p_data.has("points"));
	ERR_FAIL_COND(!p_data.has("tilts"));

	PackedVector3Array rp = p_data["points"];
	int pc = rp.size();
	ERR_FAIL_COND(pc % 3 != 0);
	points.resize(pc / 3);
	const Vector3 *r = rp.ptr();
	Vector<real_t> rtl = p_data["tilts"];
	const real_t *rt = rtl.ptr();

	for (int i = 0; i < points.size(); i++) {
		points.write[i].in = r[i * 3 + 0];
		points.write[i].out = r[i * 3 + 1];
		points.write[i].position = r[i * 3 + 2];
		points.write[i].tilt = rt[i];
	}

	mark_dirty();
	notify_property_list_changed();
}

PackedVector3Array Curve3D::tessellate(int p_max_stages, real_t p_tolerance) const {
	PackedVector3Array tess;

	if (points.size() == 0) {
		return tess;
	}
	Vector<RBMap<real_t, Vector3>> midpoints;

	midpoints.resize(points.size() - 1);

	int pc = 1;
	for (int i = 0; i < points.size() - 1; i++) {
		_bake_segment3d(midpoints.write[i], 0, 1, points[i].position, points[i].out, points[i + 1].position, points[i + 1].in, 0, p_max_stages, p_tolerance);
		pc++;
		pc += midpoints[i].size();
	}

	tess.resize(pc);
	Vector3 *bpw = tess.ptrw();
	bpw[0] = points[0].position;
	int pidx = 0;

	for (int i = 0; i < points.size() - 1; i++) {
		for (const KeyValue<real_t, Vector3> &E : midpoints[i]) {
			pidx++;
			bpw[pidx] = E.value;
		}

		pidx++;
		bpw[pidx] = points[i + 1].position;
	}

	return tess;
}

bool Curve3D::_set(const StringName &p_name, const Variant &p_value) {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			set_point_position(point_index, p_value);
			return true;
		} else if (property == "in") {
			set_point_in(point_index, p_value);
			return true;
		} else if (property == "out") {
			set_point_out(point_index, p_value);
			return true;
		} else if (property == "tilt") {
			set_point_tilt(point_index, p_value);
			return true;
		}
	}
	return false;
}

bool Curve3D::_get(const StringName &p_name, Variant &r_ret) const {
	Vector<String> components = String(p_name).split("/", true, 2);
	if (components.size() >= 2 && components[0].begins_with("point_") && components[0].trim_prefix("point_").is_valid_int()) {
		int point_index = components[0].trim_prefix("point_").to_int();
		String property = components[1];
		if (property == "position") {
			r_ret = get_point_position(point_index);
			return true;
		} else if (property == "in") {
			r_ret = get_point_in(point_index);
			return true;
		} else if (property == "out") {
			r_ret = get_point_out(point_index);
			return true;
		} else if (property == "tilt") {
			r_ret = get_point_tilt(point_index);
			return true;
		}
	}
	return false;
}

void Curve3D::_get_property_list(List<PropertyInfo> *p_list) const {
	for (int i = 0; i < points.size(); i++) {
		PropertyInfo pi = PropertyInfo(Variant::VECTOR3, vformat("point_%d/position", i));
		pi.usage &= ~PROPERTY_USAGE_STORAGE;
		p_list->push_back(pi);

		if (i != 0) {
			pi = PropertyInfo(Variant::VECTOR3, vformat("point_%d/in", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}

		if (i != points.size() - 1) {
			pi = PropertyInfo(Variant::VECTOR3, vformat("point_%d/out", i));
			pi.usage &= ~PROPERTY_USAGE_STORAGE;
			p_list->push_back(pi);
		}

		pi = PropertyInfo(Variant::FLOAT, vformat("point_%d/tilt", i));
		pi.usage &= ~PROPERTY_USAGE_STORAGE;
		p_list->push_back(pi);
	}
}

void Curve3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("get_point_count"), &Curve3D::get_point_count);
	ClassDB::bind_method(D_METHOD("set_point_count", "count"), &Curve3D::set_point_count);
	ClassDB::bind_method(D_METHOD("add_point", "position", "in", "out", "index"), &Curve3D::add_point, DEFVAL(Vector3()), DEFVAL(Vector3()), DEFVAL(-1));
	ClassDB::bind_method(D_METHOD("set_point_position", "idx", "position"), &Curve3D::set_point_position);
	ClassDB::bind_method(D_METHOD("get_point_position", "idx"), &Curve3D::get_point_position);
	ClassDB::bind_method(D_METHOD("set_point_tilt", "idx", "tilt"), &Curve3D::set_point_tilt);
	ClassDB::bind_method(D_METHOD("get_point_tilt", "idx"), &Curve3D::get_point_tilt);
	ClassDB::bind_method(D_METHOD("set_point_in", "idx", "position"), &Curve3D::set_point_in);
	ClassDB::bind_method(D_METHOD("get_point_in", "idx"), &Curve3D::get_point_in);
	ClassDB::bind_method(D_METHOD("set_point_out", "idx", "position"), &Curve3D::set_point_out);
	ClassDB::bind_method(D_METHOD("get_point_out", "idx"), &Curve3D::get_point_out);
	ClassDB::bind_method(D_METHOD("remove_point", "idx"), &Curve3D::remove_point);
	ClassDB::bind_method(D_METHOD("clear_points"), &Curve3D::clear_points);
	ClassDB::bind_method(D_METHOD("sample", "idx", "t"), &Curve3D::sample);
	ClassDB::bind_method(D_METHOD("samplef", "fofs"), &Curve3D::samplef);
	//ClassDB::bind_method(D_METHOD("bake","subdivs"),&Curve3D::bake,DEFVAL(10));
	ClassDB::bind_method(D_METHOD("set_bake_interval", "distance"), &Curve3D::set_bake_interval);
	ClassDB::bind_method(D_METHOD("get_bake_interval"), &Curve3D::get_bake_interval);
	ClassDB::bind_method(D_METHOD("set_up_vector_enabled", "enable"), &Curve3D::set_up_vector_enabled);
	ClassDB::bind_method(D_METHOD("is_up_vector_enabled"), &Curve3D::is_up_vector_enabled);

	ClassDB::bind_method(D_METHOD("get_baked_length"), &Curve3D::get_baked_length);
	ClassDB::bind_method(D_METHOD("sample_baked", "offset", "cubic"), &Curve3D::sample_baked, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("sample_baked_up_vector", "offset", "apply_tilt"), &Curve3D::sample_baked_up_vector, DEFVAL(false));
	ClassDB::bind_method(D_METHOD("get_baked_points"), &Curve3D::get_baked_points);
	ClassDB::bind_method(D_METHOD("get_baked_tilts"), &Curve3D::get_baked_tilts);
	ClassDB::bind_method(D_METHOD("get_baked_up_vectors"), &Curve3D::get_baked_up_vectors);
	ClassDB::bind_method(D_METHOD("get_closest_point", "to_point"), &Curve3D::get_closest_point);
	ClassDB::bind_method(D_METHOD("get_closest_offset", "to_point"), &Curve3D::get_closest_offset);
	ClassDB::bind_method(D_METHOD("tessellate", "max_stages", "tolerance_degrees"), &Curve3D::tessellate, DEFVAL(5), DEFVAL(4));

	ClassDB::bind_method(D_METHOD("_get_data"), &Curve3D::_get_data);
	ClassDB::bind_method(D_METHOD("_set_data", "data"), &Curve3D::_set_data);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "bake_interval", PROPERTY_HINT_RANGE, "0.01,512,0.01"), "set_bake_interval", "get_bake_interval");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "_data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NO_EDITOR | PROPERTY_USAGE_INTERNAL), "_set_data", "_get_data");
	ADD_ARRAY_COUNT("Points", "point_count", "set_point_count", "get_point_count", "point_");

	ADD_GROUP("Up Vector", "up_vector_");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "up_vector_enabled"), "set_up_vector_enabled", "is_up_vector_enabled");
}

Curve3D::Curve3D() {}
