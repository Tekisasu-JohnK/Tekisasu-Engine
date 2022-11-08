/*************************************************************************/
/*  openxr_palm_pose_extension.cpp                                       */
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

#include "openxr_palm_pose_extension.h"
#include "core/string/print_string.h"

OpenXRPalmPoseExtension *OpenXRPalmPoseExtension::singleton = nullptr;

OpenXRPalmPoseExtension *OpenXRPalmPoseExtension::get_singleton() {
	return singleton;
}

OpenXRPalmPoseExtension::OpenXRPalmPoseExtension(OpenXRAPI *p_openxr_api) :
		OpenXRExtensionWrapper(p_openxr_api) {
	singleton = this;

	request_extensions[XR_EXT_PALM_POSE_EXTENSION_NAME] = &available;
}

OpenXRPalmPoseExtension::~OpenXRPalmPoseExtension() {
	singleton = nullptr;
}

bool OpenXRPalmPoseExtension::is_available() {
	return available;
}

bool OpenXRPalmPoseExtension::is_path_supported(const String &p_path) {
	if (p_path == "/user/hand/left/input/palm_ext/pose") {
		return available;
	}

	if (p_path == "/user/hand/right/input/palm_ext/pose") {
		return available;
	}

	// Not a path under this extensions control, so we return true;
	return true;
}
