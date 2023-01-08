/*************************************************************************/
/*  connections_dialog.cpp                                               */
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

#include "connections_dialog.h"

#include "editor/doc_tools.h"
#include "editor/editor_help.h"
#include "editor/editor_node.h"
#include "editor/editor_scale.h"
#include "editor/editor_settings.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene_tree_dock.h"
#include "plugins/script_editor_plugin.h"
#include "scene/resources/packed_scene.h"

static Node *_find_first_script(Node *p_root, Node *p_node) {
	if (p_node != p_root && p_node->get_owner() != p_root) {
		return nullptr;
	}
	if (!p_node->get_script().is_null()) {
		return p_node;
	}

	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *ret = _find_first_script(p_root, p_node->get_child(i));
		if (ret) {
			return ret;
		}
	}

	return nullptr;
}

class ConnectDialogBinds : public Object {
	GDCLASS(ConnectDialogBinds, Object);

public:
	Vector<Variant> params;

	bool _set(const StringName &p_name, const Variant &p_value) {
		String name = p_name;

		if (name.begins_with("bind/argument_")) {
			int which = name.get_slice("_", 1).to_int() - 1;
			ERR_FAIL_INDEX_V(which, params.size(), false);
			params.write[which] = p_value;
		} else {
			return false;
		}

		return true;
	}

	bool _get(const StringName &p_name, Variant &r_ret) const {
		String name = p_name;

		if (name.begins_with("bind/argument_")) {
			int which = name.get_slice("_", 1).to_int() - 1;
			ERR_FAIL_INDEX_V(which, params.size(), false);
			r_ret = params[which];
		} else {
			return false;
		}

		return true;
	}

	void _get_property_list(List<PropertyInfo> *p_list) const {
		for (int i = 0; i < params.size(); i++) {
			p_list->push_back(PropertyInfo(params[i].get_type(), "bind/argument_" + itos(i + 1)));
		}
	}

	void notify_changed() {
		notify_property_list_changed();
	}

	ConnectDialogBinds() {
	}
};

/*
 * Signal automatically called by parent dialog.
 */
void ConnectDialog::ok_pressed() {
	String method_name = dst_method->get_text();

	if (method_name.is_empty()) {
		error->set_text(TTR("Method in target node must be specified."));
		error->popup_centered();
		return;
	}

	if (!method_name.strip_edges().is_valid_identifier()) {
		error->set_text(TTR("Method name must be a valid identifier."));
		error->popup_centered();
		return;
	}

	Node *target = tree->get_selected();
	if (!target) {
		return; // Nothing selected in the tree, not an error.
	}
	if (target->get_script().is_null()) {
		if (!target->has_method(method_name)) {
			error->set_text(TTR("Target method not found. Specify a valid method or attach a script to the target node."));
			error->popup_centered();
			return;
		}
	}
	emit_signal(SNAME("connected"));
	hide();
}

void ConnectDialog::_cancel_pressed() {
	hide();
}

void ConnectDialog::_item_activated() {
	_ok_pressed(); // From AcceptDialog.
}

void ConnectDialog::_text_submitted(const String &p_text) {
	_ok_pressed(); // From AcceptDialog.
}

/*
 * Called each time a target node is selected within the target node tree.
 */
void ConnectDialog::_tree_node_selected() {
	Node *current = tree->get_selected();

	if (!current) {
		return;
	}

	dst_path = source->get_path_to(current);
	if (!edit_mode) {
		set_dst_method(generate_method_callback_name(source, signal, current));
	}
	_update_ok_enabled();
}

void ConnectDialog::_unbind_count_changed(double p_count) {
	for (Control *control : bind_controls) {
		BaseButton *b = Object::cast_to<BaseButton>(control);
		if (b) {
			b->set_disabled(p_count > 0);
		}

		EditorInspector *e = Object::cast_to<EditorInspector>(control);
		if (e) {
			e->set_read_only(p_count > 0);
		}
	}
}

/*
 * Adds a new parameter bind to connection.
 */
void ConnectDialog::_add_bind() {
	Variant::Type type = (Variant::Type)type_list->get_item_id(type_list->get_selected());

	Variant value;
	Callable::CallError err;
	Variant::construct(type, value, nullptr, 0, err);

	cdbinds->params.push_back(value);
	cdbinds->notify_changed();
}

/*
 * Remove parameter bind from connection.
 */
void ConnectDialog::_remove_bind() {
	String st = bind_editor->get_selected_path();
	if (st.is_empty()) {
		return;
	}
	int idx = st.get_slice("/", 1).to_int() - 1;

	ERR_FAIL_INDEX(idx, cdbinds->params.size());
	cdbinds->params.remove_at(idx);
	cdbinds->notify_changed();
}
/*
 * Automatically generates a name for the callback method.
 */
StringName ConnectDialog::generate_method_callback_name(Node *p_source, String p_signal_name, Node *p_target) {
	String node_name = p_source->get_name();
	for (int i = 0; i < node_name.length(); i++) { // TODO: Regex filter may be cleaner.
		char32_t c = node_name[i];
		if (!is_ascii_identifier_char(c)) {
			if (c == ' ') {
				// Replace spaces with underlines.
				c = '_';
			} else {
				// Remove any other characters.
				node_name.remove_at(i);
				i--;
				continue;
			}
		}
		node_name[i] = c;
	}

	Dictionary subst;
	subst["NodeName"] = node_name.to_pascal_case();
	subst["nodeName"] = node_name.to_camel_case();
	subst["node_name"] = node_name.to_snake_case();

	subst["SignalName"] = p_signal_name.to_pascal_case();
	subst["signalName"] = p_signal_name.to_camel_case();
	subst["signal_name"] = p_signal_name.to_snake_case();

	String dst_method;
	if (p_source == p_target) {
		dst_method = String(EDITOR_GET("interface/editors/default_signal_callback_to_self_name")).format(subst);
	} else {
		dst_method = String(EDITOR_GET("interface/editors/default_signal_callback_name")).format(subst);
	}

	return dst_method;
}

/*
 * Enables or disables the connect button. The connect button is enabled if a
 * node is selected and valid in the selected mode.
 */
void ConnectDialog::_update_ok_enabled() {
	Node *target = tree->get_selected();

	if (target == nullptr) {
		get_ok_button()->set_disabled(true);
		return;
	}

	if (!advanced->is_pressed() && target->get_script().is_null()) {
		get_ok_button()->set_disabled(true);
		return;
	}

	get_ok_button()->set_disabled(false);
}

void ConnectDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			bind_editor->edit(cdbinds);

			[[fallthrough]];
		}
		case NOTIFICATION_THEME_CHANGED: {
			for (int i = 0; i < type_list->get_item_count(); i++) {
				String type_name = Variant::get_type_name((Variant::Type)type_list->get_item_id(i));
				type_list->set_item_icon(i, get_theme_icon(type_name, SNAME("EditorIcons")));
			}

			Ref<StyleBox> style = get_theme_stylebox("normal", "LineEdit")->duplicate();
			if (style.is_valid()) {
				style->set_default_margin(SIDE_TOP, style->get_default_margin(SIDE_TOP) + 1.0);
				from_signal->add_theme_style_override("normal", style);
			}
		} break;
	}
}

void ConnectDialog::_bind_methods() {
	ClassDB::bind_method("_cancel", &ConnectDialog::_cancel_pressed);
	ClassDB::bind_method("_update_ok_enabled", &ConnectDialog::_update_ok_enabled);

	ADD_SIGNAL(MethodInfo("connected"));
}

Node *ConnectDialog::get_source() const {
	return source;
}

StringName ConnectDialog::get_signal_name() const {
	return signal;
}

NodePath ConnectDialog::get_dst_path() const {
	return dst_path;
}

void ConnectDialog::set_dst_node(Node *p_node) {
	tree->set_selected(p_node);
}

StringName ConnectDialog::get_dst_method_name() const {
	String txt = dst_method->get_text();
	if (txt.contains("(")) {
		txt = txt.left(txt.find("(")).strip_edges();
	}
	return txt;
}

void ConnectDialog::set_dst_method(const StringName &p_method) {
	dst_method->set_text(p_method);
}

int ConnectDialog::get_unbinds() const {
	return int(unbind_count->get_value());
}

Vector<Variant> ConnectDialog::get_binds() const {
	return cdbinds->params;
}

bool ConnectDialog::get_deferred() const {
	return deferred->is_pressed();
}

bool ConnectDialog::get_one_shot() const {
	return one_shot->is_pressed();
}

/*
 * Returns true if ConnectDialog is being used to edit an existing connection.
 */
bool ConnectDialog::is_editing() const {
	return edit_mode;
}

/*
 * Initialize ConnectDialog and populate fields with expected data.
 * If creating a connection from scratch, sensible defaults are used.
 * If editing an existing connection, previous data is retained.
 */
void ConnectDialog::init(ConnectionData p_cd, bool p_edit) {
	set_hide_on_ok(false);

	source = static_cast<Node *>(p_cd.source);
	signal = p_cd.signal;

	tree->set_selected(nullptr);
	tree->set_marked(source, true);

	if (p_cd.target) {
		set_dst_node(static_cast<Node *>(p_cd.target));
		set_dst_method(p_cd.method);
	}

	_update_ok_enabled();

	bool b_deferred = (p_cd.flags & CONNECT_DEFERRED) == CONNECT_DEFERRED;
	bool b_oneshot = (p_cd.flags & CONNECT_ONE_SHOT) == CONNECT_ONE_SHOT;

	deferred->set_pressed(b_deferred);
	one_shot->set_pressed(b_oneshot);

	MethodInfo r_signal;
	Ref<Script> source_script = source->get_script();
	if (source_script.is_valid() && source_script->has_script_signal(signal)) {
		List<MethodInfo> signals;
		source_script->get_script_signal_list(&signals);
		for (MethodInfo &mi : signals) {
			if (mi.name == signal) {
				r_signal = mi;
				break;
			}
		}
	} else {
		ClassDB::get_signal(source->get_class(), signal, &r_signal);
	}

	unbind_count->set_max(r_signal.arguments.size());

	unbind_count->set_value(p_cd.unbinds);
	_unbind_count_changed(p_cd.unbinds);

	cdbinds->params.clear();
	cdbinds->params = p_cd.binds;
	cdbinds->notify_changed();

	edit_mode = p_edit;
}

void ConnectDialog::popup_dialog(const String &p_for_signal) {
	from_signal->set_text(p_for_signal);
	error_label->add_theme_color_override("font_color", error_label->get_theme_color(SNAME("error_color"), SNAME("Editor")));
	if (!advanced->is_pressed()) {
		error_label->set_visible(!_find_first_script(get_tree()->get_edited_scene_root(), get_tree()->get_edited_scene_root()));
	}

	if (first_popup) {
		first_popup = false;
		_advanced_pressed();
	}

	popup_centered();
}

void ConnectDialog::_advanced_pressed() {
	if (advanced->is_pressed()) {
		set_min_size(Size2(900, 500) * EDSCALE);
		connect_to_label->set_text(TTR("Connect to Node:"));
		tree->set_connect_to_script_mode(false);

		vbc_right->show();
		error_label->hide();
	} else {
		set_min_size(Size2(600, 500) * EDSCALE);
		reset_size();
		connect_to_label->set_text(TTR("Connect to Script:"));
		tree->set_connect_to_script_mode(true);

		vbc_right->hide();
		error_label->set_visible(!_find_first_script(get_tree()->get_edited_scene_root(), get_tree()->get_edited_scene_root()));
	}

	_update_ok_enabled();
	EditorSettings::get_singleton()->set_project_metadata("editor_metadata", "use_advanced_connections", advanced->is_pressed());

	popup_centered();
}

ConnectDialog::ConnectDialog() {
	set_min_size(Size2(600, 500) * EDSCALE);

	VBoxContainer *vbc = memnew(VBoxContainer);
	add_child(vbc);

	HBoxContainer *main_hb = memnew(HBoxContainer);
	vbc->add_child(main_hb);
	main_hb->set_v_size_flags(Control::SIZE_EXPAND_FILL);

	VBoxContainer *vbc_left = memnew(VBoxContainer);
	main_hb->add_child(vbc_left);
	vbc_left->set_h_size_flags(Control::SIZE_EXPAND_FILL);

	from_signal = memnew(LineEdit);
	from_signal->set_editable(false);
	vbc_left->add_margin_child(TTR("From Signal:"), from_signal);

	tree = memnew(SceneTreeEditor(false));
	tree->set_connecting_signal(true);
	tree->set_show_enabled_subscene(true);
	tree->get_scene_tree()->connect("item_activated", callable_mp(this, &ConnectDialog::_item_activated));
	tree->connect("node_selected", callable_mp(this, &ConnectDialog::_tree_node_selected));
	tree->set_connect_to_script_mode(true);

	Node *mc = vbc_left->add_margin_child(TTR("Connect to Script:"), tree, true);
	connect_to_label = Object::cast_to<Label>(vbc_left->get_child(mc->get_index() - 1));

	error_label = memnew(Label);
	error_label->set_text(TTR("Scene does not contain any script."));
	vbc_left->add_child(error_label);
	error_label->hide();

	vbc_right = memnew(VBoxContainer);
	main_hb->add_child(vbc_right);
	vbc_right->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	vbc_right->hide();

	HBoxContainer *add_bind_hb = memnew(HBoxContainer);

	type_list = memnew(OptionButton);
	type_list->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	add_bind_hb->add_child(type_list);
	for (int i = 0; i < Variant::VARIANT_MAX; i++) {
		if (i == Variant::NIL || i == Variant::OBJECT || i == Variant::CALLABLE || i == Variant::SIGNAL || i == Variant::RID) {
			// These types can't be constructed or serialized properly, so skip them.
			continue;
		}

		type_list->add_item(Variant::get_type_name(Variant::Type(i)), i);
	}
	bind_controls.push_back(type_list);

	Button *add_bind = memnew(Button);
	add_bind->set_text(TTR("Add"));
	add_bind_hb->add_child(add_bind);
	add_bind->connect("pressed", callable_mp(this, &ConnectDialog::_add_bind));
	bind_controls.push_back(add_bind);

	Button *del_bind = memnew(Button);
	del_bind->set_text(TTR("Remove"));
	add_bind_hb->add_child(del_bind);
	del_bind->connect("pressed", callable_mp(this, &ConnectDialog::_remove_bind));
	bind_controls.push_back(del_bind);

	vbc_right->add_margin_child(TTR("Add Extra Call Argument:"), add_bind_hb);

	bind_editor = memnew(EditorInspector);
	bind_controls.push_back(bind_editor);

	vbc_right->add_margin_child(TTR("Extra Call Arguments:"), bind_editor, true);

	unbind_count = memnew(SpinBox);
	unbind_count->set_tooltip_text(TTR("Allows to drop arguments sent by signal emitter."));
	unbind_count->connect("value_changed", callable_mp(this, &ConnectDialog::_unbind_count_changed));

	vbc_right->add_margin_child(TTR("Unbind Signal Arguments:"), unbind_count);

	dst_method = memnew(LineEdit);
	dst_method->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	dst_method->connect("text_submitted", callable_mp(this, &ConnectDialog::_text_submitted));
	vbc_left->add_margin_child(TTR("Receiver Method:"), dst_method);

	advanced = memnew(CheckButton);
	vbc_left->add_child(advanced);
	advanced->set_text(TTR("Advanced"));
	advanced->set_h_size_flags(Control::SIZE_SHRINK_BEGIN | Control::SIZE_EXPAND);
	advanced->set_pressed(EditorSettings::get_singleton()->get_project_metadata("editor_metadata", "use_advanced_connections", false));
	advanced->connect("pressed", callable_mp(this, &ConnectDialog::_advanced_pressed));

	HBoxContainer *hbox = memnew(HBoxContainer);
	vbc_right->add_child(hbox);

	deferred = memnew(CheckBox);
	deferred->set_h_size_flags(0);
	deferred->set_text(TTR("Deferred"));
	deferred->set_tooltip_text(TTR("Defers the signal, storing it in a queue and only firing it at idle time."));
	hbox->add_child(deferred);

	one_shot = memnew(CheckBox);
	one_shot->set_h_size_flags(0);
	one_shot->set_text(TTR("One Shot"));
	one_shot->set_tooltip_text(TTR("Disconnects the signal after its first emission."));
	hbox->add_child(one_shot);

	cdbinds = memnew(ConnectDialogBinds);

	error = memnew(AcceptDialog);
	add_child(error);
	error->set_title(TTR("Cannot connect signal"));
	error->set_ok_button_text(TTR("Close"));
	set_ok_button_text(TTR("Connect"));
}

ConnectDialog::~ConnectDialog() {
	memdelete(cdbinds);
}

//////////////////////////////////////////

// Originally copied and adapted from EditorProperty, try to keep style in sync.
Control *ConnectionsDockTree::make_custom_tooltip(const String &p_text) const {
	EditorHelpBit *help_bit = memnew(EditorHelpBit);
	help_bit->get_rich_text()->set_fixed_size_to_width(360 * EDSCALE);

	// p_text is expected to be something like this:
	// "gui_input::(event: InputEvent)::<Signal description>"
	// with the latter being possibly empty.
	PackedStringArray slices = p_text.split("::", false);
	if (slices.size() < 2) {
		// Shouldn't happen here, but just in case pass the text along.
		help_bit->set_text(p_text);
		return help_bit;
	}

	String text = TTR("Signal:") + " [u][b]" + slices[0] + "[/b][/u]";
	text += slices[1].strip_edges() + "\n";
	if (slices.size() > 2) {
		text += slices[2].strip_edges();
	} else {
		text += "[i]" + TTR("No description.") + "[/i]";
	}
	help_bit->set_text(text);

	return help_bit;
}

struct _ConnectionsDockMethodInfoSort {
	_FORCE_INLINE_ bool operator()(const MethodInfo &a, const MethodInfo &b) const {
		return a.name < b.name;
	}
};

void ConnectionsDock::_filter_changed(const String &p_text) {
	update_tree();
}

/*
 * Post-ConnectDialog callback for creating/editing connections.
 * Creates or edits connections based on state of the ConnectDialog when "Connect" is pressed.
 */
void ConnectionsDock::_make_or_edit_connection() {
	TreeItem *it = tree->get_selected();
	ERR_FAIL_COND(!it);

	NodePath dst_path = connect_dialog->get_dst_path();
	Node *target = selected_node->get_node(dst_path);
	ERR_FAIL_COND(!target);

	ConnectDialog::ConnectionData cd;
	cd.source = connect_dialog->get_source();
	cd.target = target;
	cd.signal = connect_dialog->get_signal_name();
	cd.method = connect_dialog->get_dst_method_name();
	cd.unbinds = connect_dialog->get_unbinds();
	if (cd.unbinds == 0) {
		cd.binds = connect_dialog->get_binds();
	}
	bool b_deferred = connect_dialog->get_deferred();
	bool b_oneshot = connect_dialog->get_one_shot();
	cd.flags = CONNECT_PERSIST | (b_deferred ? CONNECT_DEFERRED : 0) | (b_oneshot ? CONNECT_ONE_SHOT : 0);

	// Conditions to add function: must have a script and must not have the method already
	// (in the class, the script itself, or inherited).
	bool add_script_function = false;
	Ref<Script> scr = target->get_script();
	if (!scr.is_null() && !ClassDB::has_method(target->get_class(), cd.method)) {
		// There is a chance that the method is inherited from another script.
		bool found_inherited_function = false;
		Ref<Script> inherited_scr = scr->get_base_script();
		while (!inherited_scr.is_null()) {
			int line = inherited_scr->get_language()->find_function(cd.method, inherited_scr->get_source_code());
			if (line != -1) {
				found_inherited_function = true;
				break;
			}

			inherited_scr = inherited_scr->get_base_script();
		}

		add_script_function = !found_inherited_function;
	}
	PackedStringArray script_function_args;
	if (add_script_function) {
		// Pick up args here before "it" is deleted by update_tree.
		script_function_args = it->get_metadata(0).operator Dictionary()["args"];
		script_function_args.resize(script_function_args.size() - cd.unbinds);
		for (int i = 0; i < cd.binds.size(); i++) {
			script_function_args.push_back("extra_arg_" + itos(i) + ":" + Variant::get_type_name(cd.binds[i].get_type()));
		}
	}

	if (connect_dialog->is_editing()) {
		_disconnect(*it);
		_connect(cd);
	} else {
		_connect(cd);
	}

	// IMPORTANT NOTE: _disconnect and _connect cause an update_tree, which will delete the object "it" is pointing to.
	it = nullptr;

	if (add_script_function) {
		EditorNode::get_singleton()->emit_signal(SNAME("script_add_function_request"), target, cd.method, script_function_args);
		hide();
	}

	update_tree();
}

/*
 * Creates single connection w/ undo-redo functionality.
 */
void ConnectionsDock::_connect(ConnectDialog::ConnectionData p_cd) {
	Node *source = Object::cast_to<Node>(p_cd.source);
	Node *target = Object::cast_to<Node>(p_cd.target);

	if (!source || !target) {
		return;
	}

	Callable callable = p_cd.get_callable();
	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->create_action(vformat(TTR("Connect '%s' to '%s'"), String(p_cd.signal), String(p_cd.method)));
	undo_redo->add_do_method(source, "connect", p_cd.signal, callable, p_cd.flags);
	undo_redo->add_undo_method(source, "disconnect", p_cd.signal, callable);
	undo_redo->add_do_method(this, "update_tree");
	undo_redo->add_undo_method(this, "update_tree");
	undo_redo->add_do_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree"); // To force redraw of scene tree.
	undo_redo->add_undo_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree");

	undo_redo->commit_action();
}

/*
 * Break single connection w/ undo-redo functionality.
 */
void ConnectionsDock::_disconnect(TreeItem &p_item) {
	Connection connection = p_item.get_metadata(0);
	ConnectDialog::ConnectionData cd = connection;

	ERR_FAIL_COND(cd.source != selected_node); // Shouldn't happen but... Bugcheck.

	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->create_action(vformat(TTR("Disconnect '%s' from '%s'"), cd.signal, cd.method));

	Callable callable = cd.get_callable();
	undo_redo->add_do_method(selected_node, "disconnect", cd.signal, callable);
	undo_redo->add_undo_method(selected_node, "connect", cd.signal, callable, cd.binds, cd.flags);
	undo_redo->add_do_method(this, "update_tree");
	undo_redo->add_undo_method(this, "update_tree");
	undo_redo->add_do_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree"); // To force redraw of scene tree.
	undo_redo->add_undo_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree");

	undo_redo->commit_action();
}

/*
 * Break all connections of currently selected signal.
 * Can undo-redo as a single action.
 */
void ConnectionsDock::_disconnect_all() {
	TreeItem *item = tree->get_selected();

	if (!_is_item_signal(*item)) {
		return;
	}

	TreeItem *child = item->get_first_child();
	String signal_name = item->get_metadata(0).operator Dictionary()["name"];
	Ref<EditorUndoRedoManager> &undo_redo = EditorNode::get_undo_redo();
	undo_redo->create_action(vformat(TTR("Disconnect all from signal: '%s'"), signal_name));

	while (child) {
		Connection connection = child->get_metadata(0);
		if (!_is_connection_inherited(connection)) {
			ConnectDialog::ConnectionData cd = connection;
			undo_redo->add_do_method(selected_node, "disconnect", cd.signal, cd.get_callable());
			undo_redo->add_undo_method(selected_node, "connect", cd.signal, cd.get_callable(), cd.binds, cd.flags);
		}
		child = child->get_next();
	}

	undo_redo->add_do_method(this, "update_tree");
	undo_redo->add_undo_method(this, "update_tree");
	undo_redo->add_do_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree");
	undo_redo->add_undo_method(SceneTreeDock::get_singleton()->get_tree_editor(), "update_tree");

	undo_redo->commit_action();
}

void ConnectionsDock::_tree_item_selected() {
	TreeItem *item = tree->get_selected();
	if (!item) { // Unlikely. Disable button just in case.
		connect_button->set_text(TTR("Connect..."));
		connect_button->set_disabled(true);
	} else if (_is_item_signal(*item)) {
		connect_button->set_text(TTR("Connect..."));
		connect_button->set_disabled(false);
	} else {
		connect_button->set_text(TTR("Disconnect"));
		connect_button->set_disabled(false);
	}
}

void ConnectionsDock::_tree_item_activated() { // "Activation" on double-click.

	TreeItem *item = tree->get_selected();

	if (!item) {
		return;
	}

	if (_is_item_signal(*item)) {
		_open_connection_dialog(*item);
	} else {
		_go_to_script(*item);
	}
}

bool ConnectionsDock::_is_item_signal(TreeItem &p_item) {
	return (p_item.get_parent() == tree->get_root() || p_item.get_parent()->get_parent() == tree->get_root());
}

bool ConnectionsDock::_is_connection_inherited(Connection &p_connection) {
	return bool(p_connection.flags & CONNECT_INHERITED);
}

/*
 * Open connection dialog with TreeItem data to CREATE a brand-new connection.
 */
void ConnectionsDock::_open_connection_dialog(TreeItem &p_item) {
	String signal_name = p_item.get_metadata(0).operator Dictionary()["name"];
	const String &signal_name_ref = signal_name;

	Node *dst_node = selected_node->get_owner() ? selected_node->get_owner() : selected_node;
	if (!dst_node || dst_node->get_script().is_null()) {
		dst_node = _find_first_script(get_tree()->get_edited_scene_root(), get_tree()->get_edited_scene_root());
	}

	ConnectDialog::ConnectionData cd;
	cd.source = selected_node;
	cd.signal = StringName(signal_name_ref);
	cd.target = dst_node;
	cd.method = ConnectDialog::generate_method_callback_name(cd.source, signal_name, cd.target);
	connect_dialog->popup_dialog(signal_name_ref);
	connect_dialog->init(cd);
	connect_dialog->set_title(TTR("Connect a Signal to a Method"));
}

/*
 * Open connection dialog with Connection data to EDIT an existing connection.
 */
void ConnectionsDock::_open_connection_dialog(ConnectDialog::ConnectionData p_cd) {
	Node *src = Object::cast_to<Node>(p_cd.source);
	Node *dst = Object::cast_to<Node>(p_cd.target);

	if (src && dst) {
		const String &signal_name_ref = p_cd.signal;
		connect_dialog->set_title(TTR("Edit Connection:") + p_cd.signal);
		connect_dialog->popup_dialog(signal_name_ref);
		connect_dialog->init(p_cd, true);
	}
}

/*
 * Open slot method location in script editor.
 */
void ConnectionsDock::_go_to_script(TreeItem &p_item) {
	if (_is_item_signal(p_item)) {
		return;
	}

	Connection connection = p_item.get_metadata(0);
	ConnectDialog::ConnectionData cd = connection;
	ERR_FAIL_COND(cd.source != selected_node); // Shouldn't happen but... bugcheck.

	if (!cd.target) {
		return;
	}

	Ref<Script> scr = cd.target->get_script();

	if (scr.is_null()) {
		return;
	}

	if (scr.is_valid() && ScriptEditor::get_singleton()->script_goto_method(scr, cd.method)) {
		EditorNode::get_singleton()->editor_select(EditorNode::EDITOR_SCRIPT);
	}
}

void ConnectionsDock::_handle_signal_menu_option(int p_option) {
	TreeItem *item = tree->get_selected();

	if (!item) {
		return;
	}

	switch (p_option) {
		case CONNECT: {
			_open_connection_dialog(*item);
		} break;
		case DISCONNECT_ALL: {
			StringName signal_name = item->get_metadata(0).operator Dictionary()["name"];
			disconnect_all_dialog->set_text(vformat(TTR("Are you sure you want to remove all connections from the \"%s\" signal?"), signal_name));
			disconnect_all_dialog->popup_centered();
		} break;
		case COPY_NAME: {
			DisplayServer::get_singleton()->clipboard_set(item->get_metadata(0).operator Dictionary()["name"]);
		} break;
	}
}

void ConnectionsDock::_signal_menu_about_to_popup() {
	TreeItem *signal_item = tree->get_selected();

	bool disable_disconnect_all = true;
	for (int i = 0; i < signal_item->get_child_count(); i++) {
		if (!signal_item->get_child(i)->has_meta("_inherited_connection")) {
			disable_disconnect_all = false;
		}
	}

	signal_menu->set_item_disabled(slot_menu->get_item_index(DISCONNECT_ALL), disable_disconnect_all);
}

void ConnectionsDock::_handle_slot_menu_option(int p_option) {
	TreeItem *item = tree->get_selected();

	if (!item) {
		return;
	}

	switch (p_option) {
		case EDIT: {
			Connection connection = item->get_metadata(0);
			_open_connection_dialog(connection);
		} break;
		case GO_TO_SCRIPT: {
			_go_to_script(*item);
		} break;
		case DISCONNECT: {
			_disconnect(*item);
			update_tree();
		} break;
	}
}

void ConnectionsDock::_slot_menu_about_to_popup() {
	bool connection_is_inherited = tree->get_selected()->has_meta("_inherited_connection");

	slot_menu->set_item_disabled(slot_menu->get_item_index(EDIT), connection_is_inherited);
	slot_menu->set_item_disabled(slot_menu->get_item_index(DISCONNECT), connection_is_inherited);
}

void ConnectionsDock::_rmb_pressed(Vector2 p_position, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	TreeItem *item = tree->get_selected();

	if (!item) {
		return;
	}

	Vector2 screen_position = tree->get_screen_position() + p_position;

	if (_is_item_signal(*item)) {
		signal_menu->set_position(screen_position);
		signal_menu->reset_size();
		signal_menu->popup();
	} else {
		slot_menu->set_position(screen_position);
		slot_menu->reset_size();
		slot_menu->popup();
	}
}

void ConnectionsDock::_close() {
	hide();
}

void ConnectionsDock::_connect_pressed() {
	TreeItem *item = tree->get_selected();
	if (!item) {
		connect_button->set_disabled(true);
		return;
	}

	if (_is_item_signal(*item)) {
		_open_connection_dialog(*item);
	} else {
		_disconnect(*item);
		update_tree();
	}
}

void ConnectionsDock::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE:
		case NOTIFICATION_THEME_CHANGED: {
			search_box->set_right_icon(get_theme_icon(SNAME("Search"), SNAME("EditorIcons")));
		} break;

		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			update_tree();
		} break;
	}
}

void ConnectionsDock::_bind_methods() {
	ClassDB::bind_method("update_tree", &ConnectionsDock::update_tree);
}

void ConnectionsDock::set_node(Node *p_node) {
	selected_node = p_node;
	update_tree();
}

void ConnectionsDock::update_tree() {
	tree->clear();

	if (!selected_node) {
		return;
	}

	TreeItem *root = tree->create_item();

	List<MethodInfo> node_signals;

	selected_node->get_signal_list(&node_signals);

	bool did_script = false;
	StringName base = selected_node->get_class();

	while (base) {
		List<MethodInfo> node_signals2;
		Ref<Texture2D> icon;
		String name;

		if (!did_script) {
			// Get script signals (including signals from any base scripts).
			Ref<Script> scr = selected_node->get_script();
			if (scr.is_valid()) {
				scr->get_script_signal_list(&node_signals2);
				if (scr->get_path().is_resource_file()) {
					name = scr->get_path().get_file();
				} else {
					name = scr->get_class();
				}

				if (has_theme_icon(scr->get_class(), SNAME("EditorIcons"))) {
					icon = get_theme_icon(scr->get_class(), SNAME("EditorIcons"));
				}
			}
		} else {
			ClassDB::get_signal_list(base, &node_signals2, true);
			if (has_theme_icon(base, SNAME("EditorIcons"))) {
				icon = get_theme_icon(base, SNAME("EditorIcons"));
			}
			name = base;
		}

		if (icon.is_null()) {
			icon = get_theme_icon(SNAME("Object"), SNAME("EditorIcons"));
		}

		TreeItem *section_item = nullptr;

		// Create subsections.
		if (node_signals2.size()) {
			section_item = tree->create_item(root);
			section_item->set_text(0, name);
			section_item->set_icon(0, icon);
			section_item->set_selectable(0, false);
			section_item->set_editable(0, false);
			section_item->set_custom_bg_color(0, get_theme_color(SNAME("prop_subsection"), SNAME("Editor")));
			node_signals2.sort();
		}

		for (MethodInfo &mi : node_signals2) {
			StringName signal_name = mi.name;
			String signaldesc = "(";
			PackedStringArray argnames;

			String filter_text = search_box->get_text();
			if (!filter_text.is_subsequence_ofn(signal_name)) {
				continue;
			}

			if (mi.arguments.size()) {
				for (int i = 0; i < mi.arguments.size(); i++) {
					PropertyInfo &pi = mi.arguments[i];

					if (i > 0) {
						signaldesc += ", ";
					}
					String tname = "var";
					if (pi.type == Variant::OBJECT && pi.class_name != StringName()) {
						tname = pi.class_name.operator String();
					} else if (pi.type != Variant::NIL) {
						tname = Variant::get_type_name(pi.type);
					}
					signaldesc += (pi.name.is_empty() ? String("arg " + itos(i)) : pi.name) + ": " + tname;
					argnames.push_back(pi.name + ":" + tname);
				}
			}
			signaldesc += ")";

			// Create the children of the subsection - the actual list of signals.
			TreeItem *signal_item = tree->create_item(section_item);
			signal_item->set_text(0, String(signal_name) + signaldesc);
			Dictionary sinfo;
			sinfo["name"] = signal_name;
			sinfo["args"] = argnames;
			signal_item->set_metadata(0, sinfo);
			signal_item->set_icon(0, get_theme_icon(SNAME("Signal"), SNAME("EditorIcons")));

			// Set tooltip with the signal's documentation.
			{
				String descr;
				bool found = false;

				HashMap<StringName, HashMap<StringName, String>>::Iterator G = descr_cache.find(base);
				if (G) {
					HashMap<StringName, String>::Iterator F = G->value.find(signal_name);
					if (F) {
						found = true;
						descr = F->value;
					}
				}

				if (!found) {
					DocTools *dd = EditorHelp::get_doc_data();
					HashMap<String, DocData::ClassDoc>::Iterator F = dd->class_list.find(base);
					while (F && descr.is_empty()) {
						for (int i = 0; i < F->value.signals.size(); i++) {
							if (F->value.signals[i].name == signal_name.operator String()) {
								descr = DTR(F->value.signals[i].description);
								break;
							}
						}
						if (!F->value.inherits.is_empty()) {
							F = dd->class_list.find(F->value.inherits);
						} else {
							break;
						}
					}
					descr_cache[base][signal_name] = descr;
				}

				// "::" separators used in make_custom_tooltip for formatting.
				signal_item->set_tooltip_text(0, String(signal_name) + "::" + signaldesc + "::" + descr);
			}

			// List existing connections.
			List<Object::Connection> existing_connections;
			selected_node->get_signal_connection_list(signal_name, &existing_connections);

			for (const Object::Connection &F : existing_connections) {
				Connection connection = F;
				if (!(connection.flags & CONNECT_PERSIST)) {
					continue;
				}
				ConnectDialog::ConnectionData cd = connection;

				Node *target = Object::cast_to<Node>(cd.target);
				if (!target) {
					continue;
				}

				String path = String(selected_node->get_path_to(target)) + " :: " + cd.method + "()";
				if (cd.flags & CONNECT_DEFERRED) {
					path += " (deferred)";
				}
				if (cd.flags & CONNECT_ONE_SHOT) {
					path += " (one-shot)";
				}
				if (cd.unbinds > 0) {
					path += " unbinds(" + itos(cd.unbinds) + ")";
				} else if (!cd.binds.is_empty()) {
					path += " binds(";
					for (int i = 0; i < cd.binds.size(); i++) {
						if (i > 0) {
							path += ", ";
						}
						path += cd.binds[i].operator String();
					}
					path += ")";
				}

				TreeItem *connection_item = tree->create_item(signal_item);
				connection_item->set_text(0, path);
				connection_item->set_metadata(0, connection);
				connection_item->set_icon(0, get_theme_icon(SNAME("Slot"), SNAME("EditorIcons")));

				if (_is_connection_inherited(connection)) {
					// The scene inherits this connection.
					connection_item->set_custom_color(0, get_theme_color(SNAME("warning_color"), SNAME("Editor")));
					connection_item->set_meta("_inherited_connection", true);
				}
			}
		}

		if (!did_script) {
			did_script = true;
		} else {
			base = ClassDB::get_parent_class(base);
		}
	}

	connect_button->set_text(TTR("Connect..."));
	connect_button->set_disabled(true);
}

ConnectionsDock::ConnectionsDock() {
	set_name(TTR("Signals"));

	VBoxContainer *vbc = this;

	search_box = memnew(LineEdit);
	search_box->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	search_box->set_placeholder(TTR("Filter Signals"));
	search_box->set_clear_button_enabled(true);
	search_box->connect("text_changed", callable_mp(this, &ConnectionsDock::_filter_changed));
	vbc->add_child(search_box);

	tree = memnew(ConnectionsDockTree);
	tree->set_columns(1);
	tree->set_select_mode(Tree::SELECT_ROW);
	tree->set_hide_root(true);
	vbc->add_child(tree);
	tree->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	tree->set_allow_rmb_select(true);

	connect_button = memnew(Button);
	HBoxContainer *hb = memnew(HBoxContainer);
	vbc->add_child(hb);
	hb->add_spacer();
	hb->add_child(connect_button);
	connect_button->connect("pressed", callable_mp(this, &ConnectionsDock::_connect_pressed));

	connect_dialog = memnew(ConnectDialog);
	add_child(connect_dialog);

	disconnect_all_dialog = memnew(ConfirmationDialog);
	add_child(disconnect_all_dialog);
	disconnect_all_dialog->connect("confirmed", callable_mp(this, &ConnectionsDock::_disconnect_all));
	disconnect_all_dialog->set_text(TTR("Are you sure you want to remove all connections from this signal?"));

	signal_menu = memnew(PopupMenu);
	add_child(signal_menu);
	signal_menu->connect("id_pressed", callable_mp(this, &ConnectionsDock::_handle_signal_menu_option));
	signal_menu->connect("about_to_popup", callable_mp(this, &ConnectionsDock::_signal_menu_about_to_popup));
	signal_menu->add_item(TTR("Connect..."), CONNECT);
	signal_menu->add_item(TTR("Disconnect All"), DISCONNECT_ALL);
	signal_menu->add_item(TTR("Copy Name"), COPY_NAME);

	slot_menu = memnew(PopupMenu);
	add_child(slot_menu);
	slot_menu->connect("id_pressed", callable_mp(this, &ConnectionsDock::_handle_slot_menu_option));
	slot_menu->connect("about_to_popup", callable_mp(this, &ConnectionsDock::_slot_menu_about_to_popup));
	slot_menu->add_item(TTR("Edit..."), EDIT);
	slot_menu->add_item(TTR("Go to Method"), GO_TO_SCRIPT);
	slot_menu->add_item(TTR("Disconnect"), DISCONNECT);

	connect_dialog->connect("connected", callable_mp(this, &ConnectionsDock::_make_or_edit_connection));
	tree->connect("item_selected", callable_mp(this, &ConnectionsDock::_tree_item_selected));
	tree->connect("item_activated", callable_mp(this, &ConnectionsDock::_tree_item_activated));
	tree->connect("item_mouse_selected", callable_mp(this, &ConnectionsDock::_rmb_pressed));

	add_theme_constant_override("separation", 3 * EDSCALE);

	EDITOR_DEF("interface/editors/default_signal_callback_name", "_on_{node_name}_{signal_name}");
	EDITOR_DEF("interface/editors/default_signal_callback_to_self_name", "_on_{signal_name}");
}

ConnectionsDock::~ConnectionsDock() {
}
