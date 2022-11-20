/*************************************************************************/
/*  gdnative_interface.cpp                                               */
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

#include "gdnative_interface.h"

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "core/object/script_language_extension.h"
#include "core/os/memory.h"
#include "core/variant/variant.h"
#include "core/version.h"

// Memory Functions
static void *gdnative_alloc(size_t p_size) {
	return memalloc(p_size);
}

static void *gdnative_realloc(void *p_mem, size_t p_size) {
	return memrealloc(p_mem, p_size);
}

static void gdnative_free(void *p_mem) {
	memfree(p_mem);
}

// Helper print functions.
static void gdnative_print_error(const char *p_description, const char *p_function, const char *p_file, int32_t p_line) {
	_err_print_error(p_function, p_file, p_line, p_description, false, ERR_HANDLER_ERROR);
}
static void gdnative_print_warning(const char *p_description, const char *p_function, const char *p_file, int32_t p_line) {
	_err_print_error(p_function, p_file, p_line, p_description, false, ERR_HANDLER_WARNING);
}
static void gdnative_print_script_error(const char *p_description, const char *p_function, const char *p_file, int32_t p_line) {
	_err_print_error(p_function, p_file, p_line, p_description, false, ERR_HANDLER_SCRIPT);
}

uint64_t gdnative_get_native_struct_size(const GDNativeStringNamePtr p_name) {
	const StringName name = *reinterpret_cast<const StringName *>(p_name);
	return ClassDB::get_native_struct_size(name);
}

// Variant functions

static void gdnative_variant_new_copy(GDNativeVariantPtr r_dest, const GDNativeVariantPtr p_src) {
	memnew_placement(reinterpret_cast<Variant *>(r_dest), Variant(*reinterpret_cast<Variant *>(p_src)));
}
static void gdnative_variant_new_nil(GDNativeVariantPtr r_dest) {
	memnew_placement(reinterpret_cast<Variant *>(r_dest), Variant);
}
static void gdnative_variant_destroy(GDNativeVariantPtr p_self) {
	reinterpret_cast<Variant *>(p_self)->~Variant();
}

// variant type

static void gdnative_variant_call(GDNativeVariantPtr p_self, const GDNativeStringNamePtr p_method, const GDNativeVariantPtr *p_args, const GDNativeInt p_argcount, GDNativeVariantPtr r_return, GDNativeCallError *r_error) {
	Variant *self = (Variant *)p_self;
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	const Variant **args = (const Variant **)p_args;
	Variant ret;
	Callable::CallError error;
	self->callp(method, args, p_argcount, ret, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDNativeCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdnative_variant_call_static(GDNativeVariantType p_type, const GDNativeStringNamePtr p_method, const GDNativeVariantPtr *p_args, const GDNativeInt p_argcount, GDNativeVariantPtr r_return, GDNativeCallError *r_error) {
	Variant::Type type = (Variant::Type)p_type;
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	const Variant **args = (const Variant **)p_args;
	Variant ret;
	Callable::CallError error;
	Variant::call_static(type, method, args, p_argcount, ret, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDNativeCallErrorType)error.error;
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdnative_variant_evaluate(GDNativeVariantOperator p_op, const GDNativeVariantPtr p_a, const GDNativeVariantPtr p_b, GDNativeVariantPtr r_return, GDNativeBool *r_valid) {
	Variant::Operator op = (Variant::Operator)p_op;
	const Variant *a = (const Variant *)p_a;
	const Variant *b = (const Variant *)p_b;
	Variant *ret = (Variant *)r_return;
	bool valid;
	Variant::evaluate(op, *a, *b, *ret, valid);
	*r_valid = valid;
}

static void gdnative_variant_set(GDNativeVariantPtr p_self, const GDNativeVariantPtr p_key, const GDNativeVariantPtr p_value, GDNativeBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set(*key, *value, &valid);
	*r_valid = valid;
}

static void gdnative_variant_set_named(GDNativeVariantPtr p_self, const GDNativeStringNamePtr p_key, const GDNativeVariantPtr p_value, GDNativeBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const StringName *key = (const StringName *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set_named(*key, *value, valid);
	*r_valid = valid;
}

static void gdnative_variant_set_keyed(GDNativeVariantPtr p_self, const GDNativeVariantPtr p_key, const GDNativeVariantPtr p_value, GDNativeBool *r_valid) {
	Variant *self = (Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	self->set_keyed(*key, *value, valid);
	*r_valid = valid;
}

static void gdnative_variant_set_indexed(GDNativeVariantPtr p_self, GDNativeInt p_index, const GDNativeVariantPtr p_value, GDNativeBool *r_valid, GDNativeBool *r_oob) {
	Variant *self = (Variant *)p_self;
	const Variant *value = (const Variant *)p_value;

	bool valid;
	bool oob;
	self->set_indexed(p_index, *value, valid, oob);
	*r_valid = valid;
	*r_oob = oob;
}

static void gdnative_variant_get(const GDNativeVariantPtr p_self, const GDNativeVariantPtr p_key, GDNativeVariantPtr r_ret, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get(*key, &valid)));
	*r_valid = valid;
}

static void gdnative_variant_get_named(const GDNativeVariantPtr p_self, const GDNativeStringNamePtr p_key, GDNativeVariantPtr r_ret, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const StringName *key = (const StringName *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get_named(*key, valid)));
	*r_valid = valid;
}

static void gdnative_variant_get_keyed(const GDNativeVariantPtr p_self, const GDNativeVariantPtr p_key, GDNativeVariantPtr r_ret, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;

	bool valid;
	memnew_placement(r_ret, Variant(self->get_keyed(*key, valid)));
	*r_valid = valid;
}

static void gdnative_variant_get_indexed(const GDNativeVariantPtr p_self, GDNativeInt p_index, GDNativeVariantPtr r_ret, GDNativeBool *r_valid, GDNativeBool *r_oob) {
	const Variant *self = (const Variant *)p_self;

	bool valid;
	bool oob;
	memnew_placement(r_ret, Variant(self->get_indexed(p_index, valid, oob)));
	*r_valid = valid;
	*r_oob = oob;
}

/// Iteration.
static GDNativeBool gdnative_variant_iter_init(const GDNativeVariantPtr p_self, GDNativeVariantPtr r_iter, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	bool ret = self->iter_init(*iter, valid);
	*r_valid = valid;
	return ret;
}

static GDNativeBool gdnative_variant_iter_next(const GDNativeVariantPtr p_self, GDNativeVariantPtr r_iter, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	bool ret = self->iter_next(*iter, valid);
	*r_valid = valid;
	return ret;
}

static void gdnative_variant_iter_get(const GDNativeVariantPtr p_self, GDNativeVariantPtr r_iter, GDNativeVariantPtr r_ret, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	Variant *iter = (Variant *)r_iter;

	bool valid;
	memnew_placement(r_ret, Variant(self->iter_next(*iter, valid)));
	*r_valid = valid;
}

/// Variant functions.
static GDNativeInt gdnative_variant_hash(const GDNativeVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return self->hash();
}

static GDNativeInt gdnative_variant_recursive_hash(const GDNativeVariantPtr p_self, GDNativeInt p_recursion_count) {
	const Variant *self = (const Variant *)p_self;
	return self->recursive_hash(p_recursion_count);
}

static GDNativeBool gdnative_variant_hash_compare(const GDNativeVariantPtr p_self, const GDNativeVariantPtr p_other) {
	const Variant *self = (const Variant *)p_self;
	const Variant *other = (const Variant *)p_other;
	return self->hash_compare(*other);
}

static GDNativeBool gdnative_variant_booleanize(const GDNativeVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return self->booleanize();
}

static void gdnative_variant_duplicate(const GDNativeVariantPtr p_self, GDNativeVariantPtr r_ret, GDNativeBool p_deep) {
	const Variant *self = (const Variant *)p_self;
	memnew_placement(r_ret, Variant(self->duplicate(p_deep)));
}

static void gdnative_variant_stringify(const GDNativeVariantPtr p_self, GDNativeStringPtr r_ret) {
	const Variant *self = (const Variant *)p_self;
	memnew_placement(r_ret, String(*self));
}

static GDNativeVariantType gdnative_variant_get_type(const GDNativeVariantPtr p_self) {
	const Variant *self = (const Variant *)p_self;
	return (GDNativeVariantType)self->get_type();
}

static GDNativeBool gdnative_variant_has_method(const GDNativeVariantPtr p_self, const GDNativeStringNamePtr p_method) {
	const Variant *self = (const Variant *)p_self;
	const StringName *method = (const StringName *)p_method;
	return self->has_method(*method);
}

static GDNativeBool gdnative_variant_has_member(GDNativeVariantType p_type, const GDNativeStringNamePtr p_member) {
	return Variant::has_member((Variant::Type)p_type, *((const StringName *)p_member));
}

static GDNativeBool gdnative_variant_has_key(const GDNativeVariantPtr p_self, const GDNativeVariantPtr p_key, GDNativeBool *r_valid) {
	const Variant *self = (const Variant *)p_self;
	const Variant *key = (const Variant *)p_key;
	bool valid;
	bool ret = self->has_key(*key, valid);
	*r_valid = valid;
	return ret;
}

static void gdnative_variant_get_type_name(GDNativeVariantType p_type, GDNativeStringPtr r_ret) {
	String name = Variant::get_type_name((Variant::Type)p_type);
	memnew_placement(r_ret, String(name));
}

static GDNativeBool gdnative_variant_can_convert(GDNativeVariantType p_from, GDNativeVariantType p_to) {
	return Variant::can_convert((Variant::Type)p_from, (Variant::Type)p_to);
}

static GDNativeBool gdnative_variant_can_convert_strict(GDNativeVariantType p_from, GDNativeVariantType p_to) {
	return Variant::can_convert_strict((Variant::Type)p_from, (Variant::Type)p_to);
}

// Variant interaction.
static GDNativeVariantFromTypeConstructorFunc gdnative_get_variant_from_type_constructor(GDNativeVariantType p_type) {
	switch (p_type) {
		case GDNATIVE_VARIANT_TYPE_BOOL:
			return VariantTypeConstructor<bool>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_INT:
			return VariantTypeConstructor<int64_t>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_FLOAT:
			return VariantTypeConstructor<double>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_STRING:
			return VariantTypeConstructor<String>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR2:
			return VariantTypeConstructor<Vector2>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR2I:
			return VariantTypeConstructor<Vector2i>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_RECT2:
			return VariantTypeConstructor<Rect2>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_RECT2I:
			return VariantTypeConstructor<Rect2i>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR3:
			return VariantTypeConstructor<Vector3>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR3I:
			return VariantTypeConstructor<Vector3i>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_TRANSFORM2D:
			return VariantTypeConstructor<Transform2D>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR4:
			return VariantTypeConstructor<Vector4>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_VECTOR4I:
			return VariantTypeConstructor<Vector4i>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PLANE:
			return VariantTypeConstructor<Plane>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_QUATERNION:
			return VariantTypeConstructor<Quaternion>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_AABB:
			return VariantTypeConstructor<AABB>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_BASIS:
			return VariantTypeConstructor<Basis>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_TRANSFORM3D:
			return VariantTypeConstructor<Transform3D>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PROJECTION:
			return VariantTypeConstructor<Projection>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_COLOR:
			return VariantTypeConstructor<Color>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_STRING_NAME:
			return VariantTypeConstructor<StringName>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_NODE_PATH:
			return VariantTypeConstructor<NodePath>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_RID:
			return VariantTypeConstructor<RID>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_OBJECT:
			return VariantTypeConstructor<Object *>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_CALLABLE:
			return VariantTypeConstructor<Callable>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_SIGNAL:
			return VariantTypeConstructor<Signal>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_DICTIONARY:
			return VariantTypeConstructor<Dictionary>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_ARRAY:
			return VariantTypeConstructor<Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_BYTE_ARRAY:
			return VariantTypeConstructor<PackedByteArray>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_INT32_ARRAY:
			return VariantTypeConstructor<PackedInt32Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_INT64_ARRAY:
			return VariantTypeConstructor<PackedInt64Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_FLOAT32_ARRAY:
			return VariantTypeConstructor<PackedFloat32Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_FLOAT64_ARRAY:
			return VariantTypeConstructor<PackedFloat64Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_STRING_ARRAY:
			return VariantTypeConstructor<PackedStringArray>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_VECTOR2_ARRAY:
			return VariantTypeConstructor<PackedVector2Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_VECTOR3_ARRAY:
			return VariantTypeConstructor<PackedVector3Array>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_PACKED_COLOR_ARRAY:
			return VariantTypeConstructor<PackedColorArray>::variant_from_type;
		case GDNATIVE_VARIANT_TYPE_NIL:
		case GDNATIVE_VARIANT_TYPE_VARIANT_MAX:
			ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
	}
	ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
}

static GDNativeTypeFromVariantConstructorFunc gdnative_get_type_from_variant_constructor(GDNativeVariantType p_type) {
	switch (p_type) {
		case GDNATIVE_VARIANT_TYPE_BOOL:
			return VariantTypeConstructor<bool>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_INT:
			return VariantTypeConstructor<int64_t>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_FLOAT:
			return VariantTypeConstructor<double>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_STRING:
			return VariantTypeConstructor<String>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR2:
			return VariantTypeConstructor<Vector2>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR2I:
			return VariantTypeConstructor<Vector2i>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_RECT2:
			return VariantTypeConstructor<Rect2>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_RECT2I:
			return VariantTypeConstructor<Rect2i>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR3:
			return VariantTypeConstructor<Vector3>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR3I:
			return VariantTypeConstructor<Vector3i>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_TRANSFORM2D:
			return VariantTypeConstructor<Transform2D>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR4:
			return VariantTypeConstructor<Vector4>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_VECTOR4I:
			return VariantTypeConstructor<Vector4i>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PLANE:
			return VariantTypeConstructor<Plane>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_QUATERNION:
			return VariantTypeConstructor<Quaternion>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_AABB:
			return VariantTypeConstructor<AABB>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_BASIS:
			return VariantTypeConstructor<Basis>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_TRANSFORM3D:
			return VariantTypeConstructor<Transform3D>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PROJECTION:
			return VariantTypeConstructor<Projection>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_COLOR:
			return VariantTypeConstructor<Color>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_STRING_NAME:
			return VariantTypeConstructor<StringName>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_NODE_PATH:
			return VariantTypeConstructor<NodePath>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_RID:
			return VariantTypeConstructor<RID>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_OBJECT:
			return VariantTypeConstructor<Object *>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_CALLABLE:
			return VariantTypeConstructor<Callable>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_SIGNAL:
			return VariantTypeConstructor<Signal>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_DICTIONARY:
			return VariantTypeConstructor<Dictionary>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_ARRAY:
			return VariantTypeConstructor<Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_BYTE_ARRAY:
			return VariantTypeConstructor<PackedByteArray>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_INT32_ARRAY:
			return VariantTypeConstructor<PackedInt32Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_INT64_ARRAY:
			return VariantTypeConstructor<PackedInt64Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_FLOAT32_ARRAY:
			return VariantTypeConstructor<PackedFloat32Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_FLOAT64_ARRAY:
			return VariantTypeConstructor<PackedFloat64Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_STRING_ARRAY:
			return VariantTypeConstructor<PackedStringArray>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_VECTOR2_ARRAY:
			return VariantTypeConstructor<PackedVector2Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_VECTOR3_ARRAY:
			return VariantTypeConstructor<PackedVector3Array>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_PACKED_COLOR_ARRAY:
			return VariantTypeConstructor<PackedColorArray>::type_from_variant;
		case GDNATIVE_VARIANT_TYPE_NIL:
		case GDNATIVE_VARIANT_TYPE_VARIANT_MAX:
			ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
	}
	ERR_FAIL_V_MSG(nullptr, "Getting Variant conversion function with invalid type");
}

// ptrcalls
static GDNativePtrOperatorEvaluator gdnative_variant_get_ptr_operator_evaluator(GDNativeVariantOperator p_operator, GDNativeVariantType p_type_a, GDNativeVariantType p_type_b) {
	return (GDNativePtrOperatorEvaluator)Variant::get_ptr_operator_evaluator(Variant::Operator(p_operator), Variant::Type(p_type_a), Variant::Type(p_type_b));
}
static GDNativePtrBuiltInMethod gdnative_variant_get_ptr_builtin_method(GDNativeVariantType p_type, const GDNativeStringNamePtr p_method, GDNativeInt p_hash) {
	const StringName method = *reinterpret_cast<const StringName *>(p_method);
	uint32_t hash = Variant::get_builtin_method_hash(Variant::Type(p_type), method);
	if (hash != p_hash) {
		ERR_PRINT_ONCE("Error getting method " + method + ", hash mismatch.");
		return nullptr;
	}

	return (GDNativePtrBuiltInMethod)Variant::get_ptr_builtin_method(Variant::Type(p_type), method);
}
static GDNativePtrConstructor gdnative_variant_get_ptr_constructor(GDNativeVariantType p_type, int32_t p_constructor) {
	return (GDNativePtrConstructor)Variant::get_ptr_constructor(Variant::Type(p_type), p_constructor);
}
static GDNativePtrDestructor gdnative_variant_get_ptr_destructor(GDNativeVariantType p_type) {
	return (GDNativePtrDestructor)Variant::get_ptr_destructor(Variant::Type(p_type));
}
static void gdnative_variant_construct(GDNativeVariantType p_type, GDNativeVariantPtr p_base, const GDNativeVariantPtr *p_args, int32_t p_argument_count, GDNativeCallError *r_error) {
	memnew_placement(p_base, Variant);

	Callable::CallError error;
	Variant::construct(Variant::Type(p_type), *(Variant *)p_base, (const Variant **)p_args, p_argument_count, error);

	if (r_error) {
		r_error->error = (GDNativeCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}
static GDNativePtrSetter gdnative_variant_get_ptr_setter(GDNativeVariantType p_type, const GDNativeStringNamePtr p_member) {
	const StringName member = *reinterpret_cast<const StringName *>(p_member);
	return (GDNativePtrSetter)Variant::get_member_ptr_setter(Variant::Type(p_type), member);
}
static GDNativePtrGetter gdnative_variant_get_ptr_getter(GDNativeVariantType p_type, const GDNativeStringNamePtr p_member) {
	const StringName member = *reinterpret_cast<const StringName *>(p_member);
	return (GDNativePtrGetter)Variant::get_member_ptr_getter(Variant::Type(p_type), member);
}
static GDNativePtrIndexedSetter gdnative_variant_get_ptr_indexed_setter(GDNativeVariantType p_type) {
	return (GDNativePtrIndexedSetter)Variant::get_member_ptr_indexed_setter(Variant::Type(p_type));
}
static GDNativePtrIndexedGetter gdnative_variant_get_ptr_indexed_getter(GDNativeVariantType p_type) {
	return (GDNativePtrIndexedGetter)Variant::get_member_ptr_indexed_getter(Variant::Type(p_type));
}
static GDNativePtrKeyedSetter gdnative_variant_get_ptr_keyed_setter(GDNativeVariantType p_type) {
	return (GDNativePtrKeyedSetter)Variant::get_member_ptr_keyed_setter(Variant::Type(p_type));
}
static GDNativePtrKeyedGetter gdnative_variant_get_ptr_keyed_getter(GDNativeVariantType p_type) {
	return (GDNativePtrKeyedGetter)Variant::get_member_ptr_keyed_getter(Variant::Type(p_type));
}
static GDNativePtrKeyedChecker gdnative_variant_get_ptr_keyed_checker(GDNativeVariantType p_type) {
	return (GDNativePtrKeyedChecker)Variant::get_member_ptr_keyed_checker(Variant::Type(p_type));
}
static void gdnative_variant_get_constant_value(GDNativeVariantType p_type, const GDNativeStringNamePtr p_constant, GDNativeVariantPtr r_ret) {
	StringName constant = *reinterpret_cast<const StringName *>(p_constant);
	memnew_placement(r_ret, Variant(Variant::get_constant_value(Variant::Type(p_type), constant)));
}
static GDNativePtrUtilityFunction gdnative_variant_get_ptr_utility_function(const GDNativeStringNamePtr p_function, GDNativeInt p_hash) {
	StringName function = *reinterpret_cast<const StringName *>(p_function);
	uint32_t hash = Variant::get_utility_function_hash(function);
	if (hash != p_hash) {
		ERR_PRINT_ONCE("Error getting utility function " + function + ", hash mismatch.");
		return nullptr;
	}
	return (GDNativePtrUtilityFunction)Variant::get_ptr_utility_function(function);
}

//string helpers

static void gdnative_string_new_with_latin1_chars(GDNativeStringPtr r_dest, const char *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String(p_contents);
}

static void gdnative_string_new_with_utf8_chars(GDNativeStringPtr r_dest, const char *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf8(p_contents);
}

static void gdnative_string_new_with_utf16_chars(GDNativeStringPtr r_dest, const char16_t *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf16(p_contents);
}

static void gdnative_string_new_with_utf32_chars(GDNativeStringPtr r_dest, const char32_t *p_contents) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String((const char32_t *)p_contents);
}

static void gdnative_string_new_with_wide_chars(GDNativeStringPtr r_dest, const wchar_t *p_contents) {
	String *dest = (String *)r_dest;
	if constexpr (sizeof(wchar_t) == 2) {
		// wchar_t is 16 bit, parse.
		memnew_placement(dest, String);
		dest->parse_utf16((const char16_t *)p_contents);
	} else {
		// wchar_t is 32 bit, copy.
		memnew_placement(dest, String);
		*dest = String((const char32_t *)p_contents);
	}
}

static void gdnative_string_new_with_latin1_chars_and_len(GDNativeStringPtr r_dest, const char *p_contents, const GDNativeInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String(p_contents, p_size);
}

static void gdnative_string_new_with_utf8_chars_and_len(GDNativeStringPtr r_dest, const char *p_contents, const GDNativeInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf8(p_contents, p_size);
}

static void gdnative_string_new_with_utf16_chars_and_len(GDNativeStringPtr r_dest, const char16_t *p_contents, const GDNativeInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	dest->parse_utf16(p_contents, p_size);
}

static void gdnative_string_new_with_utf32_chars_and_len(GDNativeStringPtr r_dest, const char32_t *p_contents, const GDNativeInt p_size) {
	String *dest = (String *)r_dest;
	memnew_placement(dest, String);
	*dest = String((const char32_t *)p_contents, p_size);
}

static void gdnative_string_new_with_wide_chars_and_len(GDNativeStringPtr r_dest, const wchar_t *p_contents, const GDNativeInt p_size) {
	String *dest = (String *)r_dest;
	if constexpr (sizeof(wchar_t) == 2) {
		// wchar_t is 16 bit, parse.
		memnew_placement(dest, String);
		dest->parse_utf16((const char16_t *)p_contents, p_size);
	} else {
		// wchar_t is 32 bit, copy.
		memnew_placement(dest, String);
		*dest = String((const char32_t *)p_contents, p_size);
	}
}

static GDNativeInt gdnative_string_to_latin1_chars(const GDNativeStringPtr p_self, char *r_text, GDNativeInt p_max_write_length) {
	String *self = (String *)p_self;
	CharString cs = self->ascii(true);
	GDNativeInt len = cs.length();
	if (r_text) {
		const char *s_text = cs.ptr();
		for (GDNativeInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDNativeInt gdnative_string_to_utf8_chars(const GDNativeStringPtr p_self, char *r_text, GDNativeInt p_max_write_length) {
	String *self = (String *)p_self;
	CharString cs = self->utf8();
	GDNativeInt len = cs.length();
	if (r_text) {
		const char *s_text = cs.ptr();
		for (GDNativeInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDNativeInt gdnative_string_to_utf16_chars(const GDNativeStringPtr p_self, char16_t *r_text, GDNativeInt p_max_write_length) {
	String *self = (String *)p_self;
	Char16String cs = self->utf16();
	GDNativeInt len = cs.length();
	if (r_text) {
		const char16_t *s_text = cs.ptr();
		for (GDNativeInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDNativeInt gdnative_string_to_utf32_chars(const GDNativeStringPtr p_self, char32_t *r_text, GDNativeInt p_max_write_length) {
	String *self = (String *)p_self;
	GDNativeInt len = self->length();
	if (r_text) {
		const char32_t *s_text = self->ptr();
		for (GDNativeInt i = 0; i < MIN(len, p_max_write_length); i++) {
			r_text[i] = s_text[i];
		}
	}
	return len;
}
static GDNativeInt gdnative_string_to_wide_chars(const GDNativeStringPtr p_self, wchar_t *r_text, GDNativeInt p_max_write_length) {
	if constexpr (sizeof(wchar_t) == 4) {
		return gdnative_string_to_utf32_chars(p_self, (char32_t *)r_text, p_max_write_length);
	} else {
		return gdnative_string_to_utf16_chars(p_self, (char16_t *)r_text, p_max_write_length);
	}
}

static char32_t *gdnative_string_operator_index(GDNativeStringPtr p_self, GDNativeInt p_index) {
	String *self = (String *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->length() + 1, nullptr);
	return &self->ptrw()[p_index];
}

static const char32_t *gdnative_string_operator_index_const(const GDNativeStringPtr p_self, GDNativeInt p_index) {
	const String *self = (const String *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->length() + 1, nullptr);
	return &self->ptr()[p_index];
}

/* Packed array functions */

static uint8_t *gdnative_packed_byte_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedByteArray *self = (PackedByteArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const uint8_t *gdnative_packed_byte_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedByteArray *self = (const PackedByteArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static GDNativeTypePtr gdnative_packed_color_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedColorArray *self = (PackedColorArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptrw()[p_index];
}

static GDNativeTypePtr gdnative_packed_color_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedColorArray *self = (const PackedColorArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptr()[p_index];
}

static float *gdnative_packed_float32_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedFloat32Array *self = (PackedFloat32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const float *gdnative_packed_float32_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedFloat32Array *self = (const PackedFloat32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static double *gdnative_packed_float64_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedFloat64Array *self = (PackedFloat64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const double *gdnative_packed_float64_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedFloat64Array *self = (const PackedFloat64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static int32_t *gdnative_packed_int32_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedInt32Array *self = (PackedInt32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const int32_t *gdnative_packed_int32_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedInt32Array *self = (const PackedInt32Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static int64_t *gdnative_packed_int64_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedInt64Array *self = (PackedInt64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptrw()[p_index];
}

static const int64_t *gdnative_packed_int64_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedInt64Array *self = (const PackedInt64Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return &self->ptr()[p_index];
}

static GDNativeStringPtr gdnative_packed_string_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedStringArray *self = (PackedStringArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeStringPtr)&self->ptrw()[p_index];
}

static GDNativeStringPtr gdnative_packed_string_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedStringArray *self = (const PackedStringArray *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeStringPtr)&self->ptr()[p_index];
}

static GDNativeTypePtr gdnative_packed_vector2_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedVector2Array *self = (PackedVector2Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptrw()[p_index];
}

static GDNativeTypePtr gdnative_packed_vector2_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedVector2Array *self = (const PackedVector2Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptr()[p_index];
}

static GDNativeTypePtr gdnative_packed_vector3_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	PackedVector3Array *self = (PackedVector3Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptrw()[p_index];
}

static GDNativeTypePtr gdnative_packed_vector3_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const PackedVector3Array *self = (const PackedVector3Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeTypePtr)&self->ptr()[p_index];
}

static GDNativeVariantPtr gdnative_array_operator_index(GDNativeTypePtr p_self, GDNativeInt p_index) {
	Array *self = (Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeVariantPtr)&self->operator[](p_index);
}

static GDNativeVariantPtr gdnative_array_operator_index_const(const GDNativeTypePtr p_self, GDNativeInt p_index) {
	const Array *self = (const Array *)p_self;
	ERR_FAIL_INDEX_V(p_index, self->size(), nullptr);
	return (GDNativeVariantPtr)&self->operator[](p_index);
}

/* Dictionary functions */

static GDNativeVariantPtr gdnative_dictionary_operator_index(GDNativeTypePtr p_self, const GDNativeVariantPtr p_key) {
	Dictionary *self = (Dictionary *)p_self;
	return (GDNativeVariantPtr)&self->operator[](*(const Variant *)p_key);
}

static GDNativeVariantPtr gdnative_dictionary_operator_index_const(const GDNativeTypePtr p_self, const GDNativeVariantPtr p_key) {
	const Dictionary *self = (const Dictionary *)p_self;
	return (GDNativeVariantPtr)&self->operator[](*(const Variant *)p_key);
}

/* OBJECT API */

static void gdnative_object_method_bind_call(const GDNativeMethodBindPtr p_method_bind, GDNativeObjectPtr p_instance, const GDNativeVariantPtr *p_args, GDNativeInt p_arg_count, GDNativeVariantPtr r_return, GDNativeCallError *r_error) {
	MethodBind *mb = (MethodBind *)p_method_bind;
	Object *o = (Object *)p_instance;
	const Variant **args = (const Variant **)p_args;
	Callable::CallError error;

	Variant ret = mb->call(o, args, p_arg_count, error);
	memnew_placement(r_return, Variant(ret));

	if (r_error) {
		r_error->error = (GDNativeCallErrorType)(error.error);
		r_error->argument = error.argument;
		r_error->expected = error.expected;
	}
}

static void gdnative_object_method_bind_ptrcall(const GDNativeMethodBindPtr p_method_bind, GDNativeObjectPtr p_instance, const GDNativeTypePtr *p_args, GDNativeTypePtr p_ret) {
	MethodBind *mb = (MethodBind *)p_method_bind;
	Object *o = (Object *)p_instance;
	mb->ptrcall(o, (const void **)p_args, p_ret);
}

static void gdnative_object_destroy(GDNativeObjectPtr p_o) {
	memdelete((Object *)p_o);
}

static GDNativeObjectPtr gdnative_global_get_singleton(const GDNativeStringNamePtr p_name) {
	const StringName name = *reinterpret_cast<const StringName *>(p_name);
	return (GDNativeObjectPtr)Engine::get_singleton()->get_singleton_object(name);
}

static void *gdnative_object_get_instance_binding(GDNativeObjectPtr p_object, void *p_token, const GDNativeInstanceBindingCallbacks *p_callbacks) {
	Object *o = (Object *)p_object;
	return o->get_instance_binding(p_token, p_callbacks);
}

static void gdnative_object_set_instance_binding(GDNativeObjectPtr p_object, void *p_token, void *p_binding, const GDNativeInstanceBindingCallbacks *p_callbacks) {
	Object *o = (Object *)p_object;
	o->set_instance_binding(p_token, p_binding, p_callbacks);
}

static void gdnative_object_set_instance(GDNativeObjectPtr p_object, const GDNativeStringNamePtr p_classname, GDExtensionClassInstancePtr p_instance) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	Object *o = (Object *)p_object;
	ClassDB::set_object_extension_instance(o, classname, p_instance);
}

static GDNativeObjectPtr gdnative_object_get_instance_from_id(GDObjectInstanceID p_instance_id) {
	return (GDNativeObjectPtr)ObjectDB::get_instance(ObjectID(p_instance_id));
}

static GDNativeObjectPtr gdnative_object_cast_to(const GDNativeObjectPtr p_object, void *p_class_tag) {
	if (!p_object) {
		return nullptr;
	}
	Object *o = (Object *)p_object;

	return o->is_class_ptr(p_class_tag) ? (GDNativeObjectPtr)o : (GDNativeObjectPtr) nullptr;
}

static GDObjectInstanceID gdnative_object_get_instance_id(const GDNativeObjectPtr p_object) {
	const Object *o = (const Object *)p_object;
	return (GDObjectInstanceID)o->get_instance_id();
}

static GDNativeScriptInstancePtr gdnative_script_instance_create(const GDNativeExtensionScriptInstanceInfo *p_info, GDNativeExtensionScriptInstanceDataPtr p_instance_data) {
	ScriptInstanceExtension *script_instance_extension = memnew(ScriptInstanceExtension);
	script_instance_extension->instance = p_instance_data;
	script_instance_extension->native_info = p_info;
	return reinterpret_cast<GDNativeScriptInstancePtr>(script_instance_extension);
}

static GDNativeMethodBindPtr gdnative_classdb_get_method_bind(const GDNativeStringNamePtr p_classname, const GDNativeStringNamePtr p_methodname, GDNativeInt p_hash) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	const StringName methodname = *reinterpret_cast<const StringName *>(p_methodname);
	MethodBind *mb = ClassDB::get_method(classname, methodname);
	ERR_FAIL_COND_V(!mb, nullptr);
	if (mb->get_hash() != p_hash) {
		ERR_PRINT("Hash mismatch for method '" + classname + "." + methodname + "'.");
		return nullptr;
	}
	return (GDNativeMethodBindPtr)mb;
}

static GDNativeObjectPtr gdnative_classdb_construct_object(const GDNativeStringNamePtr p_classname) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	return (GDNativeObjectPtr)ClassDB::instantiate(classname);
}

static void *gdnative_classdb_get_class_tag(const GDNativeStringNamePtr p_classname) {
	const StringName classname = *reinterpret_cast<const StringName *>(p_classname);
	ClassDB::ClassInfo *class_info = ClassDB::classes.getptr(classname);
	return class_info ? class_info->class_ptr : nullptr;
}

void gdnative_setup_interface(GDNativeInterface *p_interface) {
	GDNativeInterface &gdni = *p_interface;

	gdni.version_major = VERSION_MAJOR;
	gdni.version_minor = VERSION_MINOR;
#if VERSION_PATCH
	gdni.version_patch = VERSION_PATCH;
#else
	gdni.version_patch = 0;
#endif
	gdni.version_string = VERSION_FULL_NAME;

	/* GODOT CORE */

	gdni.mem_alloc = gdnative_alloc;
	gdni.mem_realloc = gdnative_realloc;
	gdni.mem_free = gdnative_free;

	gdni.print_error = gdnative_print_error;
	gdni.print_warning = gdnative_print_warning;
	gdni.print_script_error = gdnative_print_script_error;

	gdni.get_native_struct_size = gdnative_get_native_struct_size;

	/* GODOT VARIANT */

	// variant general
	gdni.variant_new_copy = gdnative_variant_new_copy;
	gdni.variant_new_nil = gdnative_variant_new_nil;
	gdni.variant_destroy = gdnative_variant_destroy;

	gdni.variant_call = gdnative_variant_call;
	gdni.variant_call_static = gdnative_variant_call_static;
	gdni.variant_evaluate = gdnative_variant_evaluate;
	gdni.variant_set = gdnative_variant_set;
	gdni.variant_set_named = gdnative_variant_set_named;
	gdni.variant_set_keyed = gdnative_variant_set_keyed;
	gdni.variant_set_indexed = gdnative_variant_set_indexed;
	gdni.variant_get = gdnative_variant_get;
	gdni.variant_get_named = gdnative_variant_get_named;
	gdni.variant_get_keyed = gdnative_variant_get_keyed;
	gdni.variant_get_indexed = gdnative_variant_get_indexed;
	gdni.variant_iter_init = gdnative_variant_iter_init;
	gdni.variant_iter_next = gdnative_variant_iter_next;
	gdni.variant_iter_get = gdnative_variant_iter_get;
	gdni.variant_hash = gdnative_variant_hash;
	gdni.variant_recursive_hash = gdnative_variant_recursive_hash;
	gdni.variant_hash_compare = gdnative_variant_hash_compare;
	gdni.variant_booleanize = gdnative_variant_booleanize;
	gdni.variant_duplicate = gdnative_variant_duplicate;
	gdni.variant_stringify = gdnative_variant_stringify;

	gdni.variant_get_type = gdnative_variant_get_type;
	gdni.variant_has_method = gdnative_variant_has_method;
	gdni.variant_has_member = gdnative_variant_has_member;
	gdni.variant_has_key = gdnative_variant_has_key;
	gdni.variant_get_type_name = gdnative_variant_get_type_name;
	gdni.variant_can_convert = gdnative_variant_can_convert;
	gdni.variant_can_convert_strict = gdnative_variant_can_convert_strict;

	gdni.get_variant_from_type_constructor = gdnative_get_variant_from_type_constructor;
	gdni.get_variant_to_type_constructor = gdnative_get_type_from_variant_constructor;

	// ptrcalls.

	gdni.variant_get_ptr_operator_evaluator = gdnative_variant_get_ptr_operator_evaluator;
	gdni.variant_get_ptr_builtin_method = gdnative_variant_get_ptr_builtin_method;
	gdni.variant_get_ptr_constructor = gdnative_variant_get_ptr_constructor;
	gdni.variant_get_ptr_destructor = gdnative_variant_get_ptr_destructor;
	gdni.variant_construct = gdnative_variant_construct;
	gdni.variant_get_ptr_setter = gdnative_variant_get_ptr_setter;
	gdni.variant_get_ptr_getter = gdnative_variant_get_ptr_getter;
	gdni.variant_get_ptr_indexed_setter = gdnative_variant_get_ptr_indexed_setter;
	gdni.variant_get_ptr_indexed_getter = gdnative_variant_get_ptr_indexed_getter;
	gdni.variant_get_ptr_keyed_setter = gdnative_variant_get_ptr_keyed_setter;
	gdni.variant_get_ptr_keyed_getter = gdnative_variant_get_ptr_keyed_getter;
	gdni.variant_get_ptr_keyed_checker = gdnative_variant_get_ptr_keyed_checker;
	gdni.variant_get_constant_value = gdnative_variant_get_constant_value;
	gdni.variant_get_ptr_utility_function = gdnative_variant_get_ptr_utility_function;

	// extra utilities

	gdni.string_new_with_latin1_chars = gdnative_string_new_with_latin1_chars;
	gdni.string_new_with_utf8_chars = gdnative_string_new_with_utf8_chars;
	gdni.string_new_with_utf16_chars = gdnative_string_new_with_utf16_chars;
	gdni.string_new_with_utf32_chars = gdnative_string_new_with_utf32_chars;
	gdni.string_new_with_wide_chars = gdnative_string_new_with_wide_chars;
	gdni.string_new_with_latin1_chars_and_len = gdnative_string_new_with_latin1_chars_and_len;
	gdni.string_new_with_utf8_chars_and_len = gdnative_string_new_with_utf8_chars_and_len;
	gdni.string_new_with_utf16_chars_and_len = gdnative_string_new_with_utf16_chars_and_len;
	gdni.string_new_with_utf32_chars_and_len = gdnative_string_new_with_utf32_chars_and_len;
	gdni.string_new_with_wide_chars_and_len = gdnative_string_new_with_wide_chars_and_len;
	gdni.string_to_latin1_chars = gdnative_string_to_latin1_chars;
	gdni.string_to_utf8_chars = gdnative_string_to_utf8_chars;
	gdni.string_to_utf16_chars = gdnative_string_to_utf16_chars;
	gdni.string_to_utf32_chars = gdnative_string_to_utf32_chars;
	gdni.string_to_wide_chars = gdnative_string_to_wide_chars;
	gdni.string_operator_index = gdnative_string_operator_index;
	gdni.string_operator_index_const = gdnative_string_operator_index_const;

	/* Packed array functions */

	gdni.packed_byte_array_operator_index = gdnative_packed_byte_array_operator_index;
	gdni.packed_byte_array_operator_index_const = gdnative_packed_byte_array_operator_index_const;

	gdni.packed_color_array_operator_index = gdnative_packed_color_array_operator_index;
	gdni.packed_color_array_operator_index_const = gdnative_packed_color_array_operator_index_const;

	gdni.packed_float32_array_operator_index = gdnative_packed_float32_array_operator_index;
	gdni.packed_float32_array_operator_index_const = gdnative_packed_float32_array_operator_index_const;
	gdni.packed_float64_array_operator_index = gdnative_packed_float64_array_operator_index;
	gdni.packed_float64_array_operator_index_const = gdnative_packed_float64_array_operator_index_const;

	gdni.packed_int32_array_operator_index = gdnative_packed_int32_array_operator_index;
	gdni.packed_int32_array_operator_index_const = gdnative_packed_int32_array_operator_index_const;
	gdni.packed_int64_array_operator_index = gdnative_packed_int64_array_operator_index;
	gdni.packed_int64_array_operator_index_const = gdnative_packed_int64_array_operator_index_const;

	gdni.packed_string_array_operator_index = gdnative_packed_string_array_operator_index;
	gdni.packed_string_array_operator_index_const = gdnative_packed_string_array_operator_index_const;

	gdni.packed_vector2_array_operator_index = gdnative_packed_vector2_array_operator_index;
	gdni.packed_vector2_array_operator_index_const = gdnative_packed_vector2_array_operator_index_const;
	gdni.packed_vector3_array_operator_index = gdnative_packed_vector3_array_operator_index;
	gdni.packed_vector3_array_operator_index_const = gdnative_packed_vector3_array_operator_index_const;

	gdni.array_operator_index = gdnative_array_operator_index;
	gdni.array_operator_index_const = gdnative_array_operator_index_const;

	/* Dictionary functions */

	gdni.dictionary_operator_index = gdnative_dictionary_operator_index;
	gdni.dictionary_operator_index_const = gdnative_dictionary_operator_index_const;

	/* OBJECT */

	gdni.object_method_bind_call = gdnative_object_method_bind_call;
	gdni.object_method_bind_ptrcall = gdnative_object_method_bind_ptrcall;
	gdni.object_destroy = gdnative_object_destroy;
	gdni.global_get_singleton = gdnative_global_get_singleton;
	gdni.object_get_instance_binding = gdnative_object_get_instance_binding;
	gdni.object_set_instance_binding = gdnative_object_set_instance_binding;
	gdni.object_set_instance = gdnative_object_set_instance;

	gdni.object_cast_to = gdnative_object_cast_to;
	gdni.object_get_instance_from_id = gdnative_object_get_instance_from_id;
	gdni.object_get_instance_id = gdnative_object_get_instance_id;

	/* SCRIPT INSTANCE */

	gdni.script_instance_create = gdnative_script_instance_create;

	/* CLASSDB */

	gdni.classdb_construct_object = gdnative_classdb_construct_object;
	gdni.classdb_get_method_bind = gdnative_classdb_get_method_bind;
	gdni.classdb_get_class_tag = gdnative_classdb_get_class_tag;

	/* CLASSDB EXTENSION */

	//these are filled by implementation, since it will want to keep track of registered classes
	gdni.classdb_register_extension_class = nullptr;
	gdni.classdb_register_extension_class_method = nullptr;
	gdni.classdb_register_extension_class_integer_constant = nullptr;
	gdni.classdb_register_extension_class_property = nullptr;
	gdni.classdb_register_extension_class_property_group = nullptr;
	gdni.classdb_register_extension_class_property_subgroup = nullptr;
	gdni.classdb_register_extension_class_signal = nullptr;
	gdni.classdb_unregister_extension_class = nullptr;

	gdni.get_library_path = nullptr;
}
