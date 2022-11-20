/*************************************************************************/
/*  native_extension.cpp                                                 */
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

#include "native_extension.h"
#include "core/config/project_settings.h"
#include "core/io/config_file.h"
#include "core/object/class_db.h"
#include "core/object/method_bind.h"
#include "core/os/os.h"

String NativeExtension::get_extension_list_config_file() {
	return ProjectSettings::get_singleton()->get_project_data_path().path_join("extension_list.cfg");
}

class NativeExtensionMethodBind : public MethodBind {
	GDNativeExtensionClassMethodCall call_func;
	GDNativeExtensionClassMethodPtrCall ptrcall_func;
	void *method_userdata;
	bool vararg;
	PropertyInfo return_value_info;
	GodotTypeInfo::Metadata return_value_metadata;
	List<PropertyInfo> arguments_info;
	List<GodotTypeInfo::Metadata> arguments_metadata;

protected:
	virtual Variant::Type _gen_argument_type(int p_arg) const override {
		if (p_arg < 0) {
			return return_value_info.type;
		} else {
			return arguments_info[p_arg].type;
		}
	}
	virtual PropertyInfo _gen_argument_type_info(int p_arg) const override {
		if (p_arg < 0) {
			return return_value_info;
		} else {
			return arguments_info[p_arg];
		}
	}

public:
#ifdef DEBUG_METHODS_ENABLED
	virtual GodotTypeInfo::Metadata get_argument_meta(int p_arg) const override {
		if (p_arg < 0) {
			return return_value_metadata;
		} else {
			return arguments_metadata[p_arg];
		}
	}
#endif

	virtual Variant call(Object *p_object, const Variant **p_args, int p_arg_count, Callable::CallError &r_error) override {
		Variant ret;
		GDExtensionClassInstancePtr extension_instance = is_static() ? nullptr : p_object->_get_extension_instance();
		GDNativeCallError ce{ GDNATIVE_CALL_OK, 0, 0 };
		call_func(method_userdata, extension_instance, (const GDNativeVariantPtr *)p_args, p_arg_count, (GDNativeVariantPtr)&ret, &ce);
		r_error.error = Callable::CallError::Error(ce.error);
		r_error.argument = ce.argument;
		r_error.expected = ce.expected;
		return ret;
	}
	virtual void ptrcall(Object *p_object, const void **p_args, void *r_ret) override {
		ERR_FAIL_COND_MSG(vararg, "Vararg methods don't have ptrcall support. This is most likely an engine bug.");
		GDExtensionClassInstancePtr extension_instance = p_object->_get_extension_instance();
		ptrcall_func(method_userdata, extension_instance, (const GDNativeTypePtr *)p_args, (GDNativeTypePtr)r_ret);
	}

	virtual bool is_vararg() const override {
		return false;
	}

	explicit NativeExtensionMethodBind(const GDNativeExtensionClassMethodInfo *p_method_info) {
		method_userdata = p_method_info->method_userdata;
		call_func = p_method_info->call_func;
		ptrcall_func = p_method_info->ptrcall_func;
		set_name(*reinterpret_cast<StringName *>(p_method_info->name));

		if (p_method_info->has_return_value) {
			return_value_info = PropertyInfo(*p_method_info->return_value_info);
			return_value_metadata = GodotTypeInfo::Metadata(p_method_info->return_value_metadata);
		}

		for (uint32_t i = 0; i < p_method_info->argument_count; i++) {
			arguments_info.push_back(PropertyInfo(p_method_info->arguments_info[i]));
			arguments_metadata.push_back(GodotTypeInfo::Metadata(p_method_info->arguments_metadata[i]));
		}

		set_hint_flags(p_method_info->method_flags);

		vararg = p_method_info->method_flags & GDNATIVE_EXTENSION_METHOD_FLAG_VARARG;
		_set_returns(p_method_info->has_return_value);
		_set_const(p_method_info->method_flags & GDNATIVE_EXTENSION_METHOD_FLAG_CONST);
		_set_static(p_method_info->method_flags & GDNATIVE_EXTENSION_METHOD_FLAG_STATIC);
#ifdef DEBUG_METHODS_ENABLED
		_generate_argument_types(p_method_info->argument_count);
#endif
		set_argument_count(p_method_info->argument_count);

		Vector<Variant> defargs;
		defargs.resize(p_method_info->default_argument_count);
		for (uint32_t i = 0; i < p_method_info->default_argument_count; i++) {
			defargs.write[i] = *static_cast<Variant *>(p_method_info->default_arguments[i]);
		}

		set_default_arguments(defargs);
	}
};

static GDNativeInterface gdnative_interface;

void NativeExtension::_register_extension_class(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_parent_class_name, const GDNativeExtensionClassCreationInfo *p_extension_funcs) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<StringName *>(p_class_name);
	StringName parent_class_name = *reinterpret_cast<StringName *>(p_parent_class_name);
	ERR_FAIL_COND_MSG(!String(class_name).is_valid_identifier(), "Attempt to register extension class '" + class_name + "', which is not a valid class identifier.");
	ERR_FAIL_COND_MSG(ClassDB::class_exists(class_name), "Attempt to register extension class '" + class_name + "', which appears to be already registered.");

	Extension *parent_extension = nullptr;

	if (self->extension_classes.has(parent_class_name)) {
		parent_extension = &self->extension_classes[parent_class_name];
	} else if (ClassDB::class_exists(parent_class_name)) {
		if (ClassDB::get_api_type(parent_class_name) == ClassDB::API_EXTENSION || ClassDB::get_api_type(parent_class_name) == ClassDB::API_EDITOR_EXTENSION) {
			ERR_PRINT("Unimplemented yet");
			//inheriting from another extension
		} else {
			//inheriting from engine class
		}
	} else {
		ERR_FAIL_MSG("Attempt to register an extension class '" + String(class_name) + "' using non-existing parent class '" + String(parent_class_name) + "'");
	}

	self->extension_classes[class_name] = Extension();

	Extension *extension = &self->extension_classes[class_name];

	if (parent_extension) {
		extension->native_extension.parent = &parent_extension->native_extension;
		parent_extension->native_extension.children.push_back(&extension->native_extension);
	}

	extension->native_extension.parent_class_name = parent_class_name;
	extension->native_extension.class_name = class_name;
	extension->native_extension.editor_class = self->level_initialized == INITIALIZATION_LEVEL_EDITOR;
	extension->native_extension.is_virtual = p_extension_funcs->is_virtual;
	extension->native_extension.is_abstract = p_extension_funcs->is_abstract;
	extension->native_extension.set = p_extension_funcs->set_func;
	extension->native_extension.get = p_extension_funcs->get_func;
	extension->native_extension.get_property_list = p_extension_funcs->get_property_list_func;
	extension->native_extension.free_property_list = p_extension_funcs->free_property_list_func;
	extension->native_extension.property_can_revert = p_extension_funcs->property_can_revert_func;
	extension->native_extension.property_get_revert = p_extension_funcs->property_get_revert_func;
	extension->native_extension.notification = p_extension_funcs->notification_func;
	extension->native_extension.to_string = p_extension_funcs->to_string_func;
	extension->native_extension.reference = p_extension_funcs->reference_func;
	extension->native_extension.unreference = p_extension_funcs->unreference_func;
	extension->native_extension.class_userdata = p_extension_funcs->class_userdata;
	extension->native_extension.create_instance = p_extension_funcs->create_instance_func;
	extension->native_extension.free_instance = p_extension_funcs->free_instance_func;
	extension->native_extension.get_virtual = p_extension_funcs->get_virtual_func;
	extension->native_extension.get_rid = p_extension_funcs->get_rid_func;

	ClassDB::register_extension_class(&extension->native_extension);
}
void NativeExtension::_register_extension_class_method(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeExtensionClassMethodInfo *p_method_info) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	StringName method_name = *reinterpret_cast<const StringName *>(p_method_info->name);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension method '" + String(method_name) + "' for unexisting class '" + class_name + "'.");

	//Extension *extension = &self->extension_classes[class_name];

	NativeExtensionMethodBind *method = memnew(NativeExtensionMethodBind(p_method_info));
	method->set_instance_class(class_name);

	ClassDB::bind_method_custom(class_name, method);
}
void NativeExtension::_register_extension_class_integer_constant(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_enum_name, const GDNativeStringNamePtr p_constant_name, GDNativeInt p_constant_value, GDNativeBool p_is_bitfield) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	StringName enum_name = *reinterpret_cast<const StringName *>(p_enum_name);
	StringName constant_name = *reinterpret_cast<const StringName *>(p_constant_name);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension constant '" + constant_name + "' for unexisting class '" + class_name + "'.");

	ClassDB::bind_integer_constant(class_name, enum_name, constant_name, p_constant_value, p_is_bitfield);
}

void NativeExtension::_register_extension_class_property(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativePropertyInfo *p_info, const GDNativeStringNamePtr p_setter, const GDNativeStringNamePtr p_getter) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	StringName setter = *reinterpret_cast<const StringName *>(p_setter);
	StringName getter = *reinterpret_cast<const StringName *>(p_getter);
	String property_name = *reinterpret_cast<const StringName *>(p_info->name);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension class property '" + property_name + "' for unexisting class '" + class_name + "'.");

	//Extension *extension = &self->extension_classes[class_name];
	PropertyInfo pinfo(*p_info);

	ClassDB::add_property(class_name, pinfo, setter, getter);
}

void NativeExtension::_register_extension_class_property_group(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringPtr p_group_name, const GDNativeStringPtr p_prefix) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	String group_name = *reinterpret_cast<const String *>(p_group_name);
	String prefix = *reinterpret_cast<const String *>(p_prefix);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension class property group '" + group_name + "' for unexisting class '" + class_name + "'.");

	ClassDB::add_property_group(class_name, group_name, prefix);
}

void NativeExtension::_register_extension_class_property_subgroup(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringPtr p_subgroup_name, const GDNativeStringPtr p_prefix) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	String subgroup_name = *reinterpret_cast<const String *>(p_subgroup_name);
	String prefix = *reinterpret_cast<const String *>(p_prefix);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension class property subgroup '" + subgroup_name + "' for unexisting class '" + class_name + "'.");

	ClassDB::add_property_subgroup(class_name, subgroup_name, prefix);
}

void NativeExtension::_register_extension_class_signal(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name, const GDNativeStringNamePtr p_signal_name, const GDNativePropertyInfo *p_argument_info, GDNativeInt p_argument_count) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	StringName signal_name = *reinterpret_cast<const StringName *>(p_signal_name);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to register extension class signal '" + signal_name + "' for unexisting class '" + class_name + "'.");

	MethodInfo s;
	s.name = signal_name;
	for (int i = 0; i < p_argument_count; i++) {
		PropertyInfo arg(p_argument_info[i]);
		s.arguments.push_back(arg);
	}
	ClassDB::add_signal(class_name, s);
}

void NativeExtension::_unregister_extension_class(const GDNativeExtensionClassLibraryPtr p_library, const GDNativeStringNamePtr p_class_name) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	StringName class_name = *reinterpret_cast<const StringName *>(p_class_name);
	ERR_FAIL_COND_MSG(!self->extension_classes.has(class_name), "Attempt to unregister unexisting extension class '" + class_name + "'.");
	Extension *ext = &self->extension_classes[class_name];
	ERR_FAIL_COND_MSG(ext->native_extension.children.size(), "Attempt to unregister class '" + class_name + "' while other extension classes inherit from it.");

	ClassDB::unregister_extension_class(class_name);
	if (ext->native_extension.parent != nullptr) {
		ext->native_extension.parent->children.erase(&ext->native_extension);
	}
	self->extension_classes.erase(class_name);
}

void NativeExtension::_get_library_path(const GDNativeExtensionClassLibraryPtr p_library, GDNativeStringPtr r_path) {
	NativeExtension *self = static_cast<NativeExtension *>(p_library);

	*(String *)r_path = self->library_path;
}

Error NativeExtension::open_library(const String &p_path, const String &p_entry_symbol) {
	Error err = OS::get_singleton()->open_dynamic_library(p_path, library, true, &library_path);
	if (err != OK) {
		ERR_PRINT("GDExtension dynamic library not found: " + p_path);
		return err;
	}

	void *entry_funcptr = nullptr;

	err = OS::get_singleton()->get_dynamic_library_symbol_handle(library, p_entry_symbol, entry_funcptr, false);

	if (err != OK) {
		ERR_PRINT("GDExtension entry point '" + p_entry_symbol + "' not found in library " + p_path);
		OS::get_singleton()->close_dynamic_library(library);
		return err;
	}

	GDNativeInitializationFunction initialization_function = (GDNativeInitializationFunction)entry_funcptr;

	if (initialization_function(&gdnative_interface, this, &initialization)) {
		level_initialized = -1;
		return OK;
	} else {
		ERR_PRINT("GDExtension initialization function '" + p_entry_symbol + "' returned an error.");
		return FAILED;
	}
}

void NativeExtension::close_library() {
	ERR_FAIL_COND(library == nullptr);
	OS::get_singleton()->close_dynamic_library(library);

	library = nullptr;
}

bool NativeExtension::is_library_open() const {
	return library != nullptr;
}

NativeExtension::InitializationLevel NativeExtension::get_minimum_library_initialization_level() const {
	ERR_FAIL_COND_V(library == nullptr, INITIALIZATION_LEVEL_CORE);
	return InitializationLevel(initialization.minimum_initialization_level);
}

void NativeExtension::initialize_library(InitializationLevel p_level) {
	ERR_FAIL_COND(library == nullptr);
	ERR_FAIL_COND_MSG(p_level <= int32_t(level_initialized), vformat("Level '%d' must be higher than the current level '%d'", p_level, level_initialized));

	level_initialized = int32_t(p_level);

	ERR_FAIL_COND(initialization.initialize == nullptr);

	initialization.initialize(initialization.userdata, GDNativeInitializationLevel(p_level));
}
void NativeExtension::deinitialize_library(InitializationLevel p_level) {
	ERR_FAIL_COND(library == nullptr);
	ERR_FAIL_COND(p_level > int32_t(level_initialized));

	level_initialized = int32_t(p_level) - 1;
	initialization.deinitialize(initialization.userdata, GDNativeInitializationLevel(p_level));
}

void NativeExtension::_bind_methods() {
	ClassDB::bind_method(D_METHOD("open_library", "path", "entry_symbol"), &NativeExtension::open_library);
	ClassDB::bind_method(D_METHOD("close_library"), &NativeExtension::close_library);
	ClassDB::bind_method(D_METHOD("is_library_open"), &NativeExtension::is_library_open);

	ClassDB::bind_method(D_METHOD("get_minimum_library_initialization_level"), &NativeExtension::get_minimum_library_initialization_level);
	ClassDB::bind_method(D_METHOD("initialize_library", "level"), &NativeExtension::initialize_library);

	BIND_ENUM_CONSTANT(INITIALIZATION_LEVEL_CORE);
	BIND_ENUM_CONSTANT(INITIALIZATION_LEVEL_SERVERS);
	BIND_ENUM_CONSTANT(INITIALIZATION_LEVEL_SCENE);
	BIND_ENUM_CONSTANT(INITIALIZATION_LEVEL_EDITOR);
}

NativeExtension::NativeExtension() {
}

NativeExtension::~NativeExtension() {
	if (library != nullptr) {
		close_library();
	}
}

extern void gdnative_setup_interface(GDNativeInterface *p_interface);

void NativeExtension::initialize_native_extensions() {
	gdnative_setup_interface(&gdnative_interface);

	gdnative_interface.classdb_register_extension_class = _register_extension_class;
	gdnative_interface.classdb_register_extension_class_method = _register_extension_class_method;
	gdnative_interface.classdb_register_extension_class_integer_constant = _register_extension_class_integer_constant;
	gdnative_interface.classdb_register_extension_class_property = _register_extension_class_property;
	gdnative_interface.classdb_register_extension_class_property_group = _register_extension_class_property_group;
	gdnative_interface.classdb_register_extension_class_property_subgroup = _register_extension_class_property_subgroup;
	gdnative_interface.classdb_register_extension_class_signal = _register_extension_class_signal;
	gdnative_interface.classdb_unregister_extension_class = _unregister_extension_class;
	gdnative_interface.get_library_path = _get_library_path;
}

Ref<Resource> NativeExtensionResourceLoader::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_use_sub_threads, float *r_progress, CacheMode p_cache_mode) {
	Ref<ConfigFile> config;
	config.instantiate();

	Error err = config->load(p_path);

	if (r_error) {
		*r_error = err;
	}

	if (err != OK) {
		ERR_PRINT("Error loading GDExtension configuration file: " + p_path);
		return Ref<Resource>();
	}

	if (!config->has_section_key("configuration", "entry_symbol")) {
		if (r_error) {
			*r_error = ERR_INVALID_DATA;
		}
		ERR_PRINT("GDExtension configuration file must contain a \"configuration/entry_symbol\" key: " + p_path);
		return Ref<Resource>();
	}

	String entry_symbol = config->get_value("configuration", "entry_symbol");

	List<String> libraries;

	config->get_section_keys("libraries", &libraries);

	String library_path;

	for (const String &E : libraries) {
		Vector<String> tags = E.split(".");
		bool all_tags_met = true;
		for (int i = 0; i < tags.size(); i++) {
			String tag = tags[i].strip_edges();
			if (!OS::get_singleton()->has_feature(tag)) {
				all_tags_met = false;
				break;
			}
		}

		if (all_tags_met) {
			library_path = config->get_value("libraries", E);
			break;
		}
	}

	if (library_path.is_empty()) {
		if (r_error) {
			*r_error = ERR_FILE_NOT_FOUND;
		}
		const String os_arch = OS::get_singleton()->get_name().to_lower() + "." + Engine::get_singleton()->get_architecture_name();
		ERR_PRINT(vformat("No GDExtension library found for current OS and architecture (%s) in configuration file: %s", os_arch, p_path));
		return Ref<Resource>();
	}

	if (!library_path.is_resource_file() && !library_path.is_absolute_path()) {
		library_path = p_path.get_base_dir().path_join(library_path);
	}

	Ref<NativeExtension> lib;
	lib.instantiate();
	String abs_path = ProjectSettings::get_singleton()->globalize_path(library_path);
	err = lib->open_library(abs_path, entry_symbol);

	if (r_error) {
		*r_error = err;
	}

	if (err != OK) {
		// Errors already logged in open_library()
		return Ref<Resource>();
	}

	return lib;
}

void NativeExtensionResourceLoader::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("gdextension");
}

bool NativeExtensionResourceLoader::handles_type(const String &p_type) const {
	return p_type == "NativeExtension";
}

String NativeExtensionResourceLoader::get_resource_type(const String &p_path) const {
	String el = p_path.get_extension().to_lower();
	if (el == "gdextension") {
		return "NativeExtension";
	}
	return "";
}
