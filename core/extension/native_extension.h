/*************************************************************************/
/*  native_extension.h                                                   */
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

#ifndef NATIVE_EXTENSION_H
#define NATIVE_EXTENSION_H

#include "core/extension/gdnative_interface.h"
#include "core/io/resource_loader.h"
#include "core/object/ref_counted.h"

class NativeExtension : public Resource {
	GDCLASS(NativeExtension, Resource)

	void *library = nullptr; // pointer if valid,
	String library_path;

	struct Extension {
		ObjectNativeExtension native_extension;
	};

	HashMap<StringName, Extension> extension_classes;

	static void _register_extension_class(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_parent_class_name, const GDNativeExtensionClassCreationInfo *p_extension_funcs);
	static void _register_extension_class_method(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeExtensionClassMethodInfo *p_method_info);
	static void _register_extension_class_integer_constant(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_enum_name, const GDNativeStringNamePtr p_constant_name, GDNativeInt p_constant_value, GDNativeBool p_is_bitfield);
	static void _register_extension_class_property(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativePropertyInfo *p_info, const GDNativeStringNamePtr p_setter, const GDNativeStringNamePtr p_getter);
	static void _register_extension_class_property_group(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_group_name, const GDNativeStringNamePtr p_prefix);
	static void _register_extension_class_property_subgroup(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_subgroup_name, const GDNativeStringNamePtr p_prefix);
	static void _register_extension_class_signal(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_signal_name, const GDNativePropertyInfo *p_argument_info, GDNativeInt p_argument_count);
	static void _unregister_extension_class(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name);
	static void _get_library_path(const GDNativeExtensionClassLibraryPtr p_library, GDNativeStringPtr r_path);

	GDNativeInitialization initialization;
	int32_t level_initialized = -1;

protected:
	static void _bind_methods();

public:
	static String get_extension_list_config_file();

	Error open_library(const String &p_path, const String &p_entry_symbol);
	void close_library();

	enum InitializationLevel {
		INITIALIZATION_LEVEL_CORE = GDNATIVE_INITIALIZATION_CORE,
		INITIALIZATION_LEVEL_SERVERS = GDNATIVE_INITIALIZATION_SERVERS,
		INITIALIZATION_LEVEL_SCENE = GDNATIVE_INITIALIZATION_SCENE,
		INITIALIZATION_LEVEL_EDITOR = GDNATIVE_INITIALIZATION_EDITOR
	};

	bool is_library_open() const;

	InitializationLevel get_minimum_library_initialization_level() const;
	void initialize_library(InitializationLevel p_level);
	void deinitialize_library(InitializationLevel p_level);

	static void initialize_native_extensions();
	NativeExtension();
	~NativeExtension();
};

VARIANT_ENUM_CAST(NativeExtension::InitializationLevel)

class NativeExtensionResourceLoader : public ResourceFormatLoader {
public:
	virtual Ref<Resource> load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads = false, float *r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

#endif // NATIVE_EXTENSION_H
