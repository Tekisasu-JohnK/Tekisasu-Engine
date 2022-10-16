/*************************************************************************/
/*  sprite_3d.h                                                          */
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

#ifndef SPRITE_3D_H
#define SPRITE_3D_H

#include "scene/3d/visual_instance_3d.h"
#include "scene/resources/sprite_frames.h"

class SpriteBase3D : public GeometryInstance3D {
	GDCLASS(SpriteBase3D, GeometryInstance3D);

	mutable Ref<TriangleMesh> triangle_mesh; //cached

public:
	enum DrawFlags {
		FLAG_TRANSPARENT,
		FLAG_SHADED,
		FLAG_DOUBLE_SIDED,
		FLAG_DISABLE_DEPTH_TEST,
		FLAG_FIXED_SIZE,
		FLAG_MAX

	};

	enum AlphaCutMode {
		ALPHA_CUT_DISABLED,
		ALPHA_CUT_DISCARD,
		ALPHA_CUT_OPAQUE_PREPASS
	};

private:
	bool color_dirty = true;
	Color color_accum;

	SpriteBase3D *parent_sprite = nullptr;
	List<SpriteBase3D *> children;
	List<SpriteBase3D *>::Element *pI = nullptr;

	bool centered = true;
	Point2 offset;

	bool hflip = false;
	bool vflip = false;

	Color modulate = Color(1, 1, 1, 1);
	int render_priority = 0;

	Vector3::Axis axis = Vector3::AXIS_Z;
	real_t pixel_size = 0.01;
	AABB aabb;

	RID mesh;
	RID material;

	RID last_shader;
	RID last_texture;

	bool flags[FLAG_MAX] = {};
	AlphaCutMode alpha_cut = ALPHA_CUT_DISABLED;
	StandardMaterial3D::BillboardMode billboard_mode = StandardMaterial3D::BILLBOARD_DISABLED;
	StandardMaterial3D::TextureFilter texture_filter = StandardMaterial3D::TEXTURE_FILTER_LINEAR_WITH_MIPMAPS;
	bool pending_update = false;
	void _im_update();

	void _propagate_color_changed();

protected:
	Color _get_color_accum();
	void _notification(int p_what);
	static void _bind_methods();
	virtual void _draw() = 0;
	void draw_texture_rect(Ref<Texture2D> p_texture, Rect2 p_dst_rect, Rect2 p_src_rect);
	_FORCE_INLINE_ void set_aabb(const AABB &p_aabb) { aabb = p_aabb; }
	_FORCE_INLINE_ RID &get_mesh() { return mesh; }
	_FORCE_INLINE_ RID &get_material() { return material; }

	uint32_t mesh_surface_offsets[RS::ARRAY_MAX];
	PackedByteArray vertex_buffer;
	PackedByteArray attribute_buffer;
	uint32_t vertex_stride = 0;
	uint32_t attrib_stride = 0;
	uint32_t skin_stride = 0;
	uint32_t mesh_surface_format = 0;

	void _queue_redraw();

public:
	void set_centered(bool p_center);
	bool is_centered() const;

	void set_offset(const Point2 &p_offset);
	Point2 get_offset() const;

	void set_flip_h(bool p_flip);
	bool is_flipped_h() const;

	void set_flip_v(bool p_flip);
	bool is_flipped_v() const;

	void set_render_priority(int p_priority);
	int get_render_priority() const;

	void set_modulate(const Color &p_color);
	Color get_modulate() const;

	void set_pixel_size(real_t p_amount);
	real_t get_pixel_size() const;

	void set_axis(Vector3::Axis p_axis);
	Vector3::Axis get_axis() const;

	void set_draw_flag(DrawFlags p_flag, bool p_enable);
	bool get_draw_flag(DrawFlags p_flag) const;

	void set_alpha_cut_mode(AlphaCutMode p_mode);
	AlphaCutMode get_alpha_cut_mode() const;

	void set_billboard_mode(StandardMaterial3D::BillboardMode p_mode);
	StandardMaterial3D::BillboardMode get_billboard_mode() const;

	void set_texture_filter(StandardMaterial3D::TextureFilter p_filter);
	StandardMaterial3D::TextureFilter get_texture_filter() const;

	virtual Rect2 get_item_rect() const = 0;

	virtual AABB get_aabb() const override;

	Ref<TriangleMesh> generate_triangle_mesh() const;

	SpriteBase3D();
	~SpriteBase3D();
};

class Sprite3D : public SpriteBase3D {
	GDCLASS(Sprite3D, SpriteBase3D);
	Ref<Texture2D> texture;

	bool region = false;
	Rect2 region_rect;

	int frame = 0;

	int vframes = 1;
	int hframes = 1;

protected:
	virtual void _draw() override;
	static void _bind_methods();

	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_texture(const Ref<Texture2D> &p_texture);
	Ref<Texture2D> get_texture() const;

	void set_region_enabled(bool p_region);
	bool is_region_enabled() const;

	void set_region_rect(const Rect2 &p_region_rect);
	Rect2 get_region_rect() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_frame_coords(const Vector2i &p_coord);
	Vector2i get_frame_coords() const;

	void set_vframes(int p_amount);
	int get_vframes() const;

	void set_hframes(int p_amount);
	int get_hframes() const;

	virtual Rect2 get_item_rect() const override;

	Sprite3D();
	//~Sprite3D();
};

class AnimatedSprite3D : public SpriteBase3D {
	GDCLASS(AnimatedSprite3D, SpriteBase3D);

	Ref<SpriteFrames> frames;
	bool playing = false;
	bool playing_backwards = false;
	bool backwards = false;
	StringName animation = "default";
	int frame = 0;
	float speed_scale = 1.0f;

	bool centered = false;

	bool is_over = false;
	double timeout = 0.0;

	void _res_changed();

	double _get_frame_duration();
	void _reset_timeout();

protected:
	virtual void _draw() override;
	static void _bind_methods();
	void _notification(int p_what);
	void _validate_property(PropertyInfo &p_property) const;

public:
	void set_sprite_frames(const Ref<SpriteFrames> &p_frames);
	Ref<SpriteFrames> get_sprite_frames() const;

	void play(const StringName &p_animation = StringName(), bool p_backwards = false);
	void stop();

	void set_playing(bool p_playing);
	bool is_playing() const;

	void set_animation(const StringName &p_animation);
	StringName get_animation() const;

	void set_frame(int p_frame);
	int get_frame() const;

	void set_speed_scale(double p_speed_scale);
	double get_speed_scale() const;

	virtual Rect2 get_item_rect() const override;

	virtual PackedStringArray get_configuration_warnings() const override;
	virtual void get_argument_options(const StringName &p_function, int p_idx, List<String> *r_options) const override;

	AnimatedSprite3D();
};

VARIANT_ENUM_CAST(SpriteBase3D::DrawFlags);
VARIANT_ENUM_CAST(SpriteBase3D::AlphaCutMode);

#endif // SPRITE_3D_H
