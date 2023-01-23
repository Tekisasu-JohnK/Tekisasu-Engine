/**************************************************************************/
/*  stream_peer_tls.cpp                                                   */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "stream_peer_tls.h"

#include "core/config/engine.h"

StreamPeerTLS *(*StreamPeerTLS::_create)() = nullptr;

StreamPeerTLS *StreamPeerTLS::create() {
	if (_create) {
		return _create();
	}
	return nullptr;
}

bool StreamPeerTLS::available = false;

bool StreamPeerTLS::is_available() {
	return available;
}

void StreamPeerTLS::set_blocking_handshake_enabled(bool p_enabled) {
	blocking_handshake = p_enabled;
}

bool StreamPeerTLS::is_blocking_handshake_enabled() const {
	return blocking_handshake;
}

void StreamPeerTLS::_bind_methods() {
	ClassDB::bind_method(D_METHOD("poll"), &StreamPeerTLS::poll);
	ClassDB::bind_method(D_METHOD("accept_stream", "stream", "private_key", "certificate", "chain"), &StreamPeerTLS::accept_stream, DEFVAL(Ref<X509Certificate>()));
	ClassDB::bind_method(D_METHOD("connect_to_stream", "stream", "validate_certs", "for_hostname", "valid_certificate"), &StreamPeerTLS::connect_to_stream, DEFVAL(false), DEFVAL(String()), DEFVAL(Ref<X509Certificate>()));
	ClassDB::bind_method(D_METHOD("get_status"), &StreamPeerTLS::get_status);
	ClassDB::bind_method(D_METHOD("get_stream"), &StreamPeerTLS::get_stream);
	ClassDB::bind_method(D_METHOD("disconnect_from_stream"), &StreamPeerTLS::disconnect_from_stream);
	ClassDB::bind_method(D_METHOD("set_blocking_handshake_enabled", "enabled"), &StreamPeerTLS::set_blocking_handshake_enabled);
	ClassDB::bind_method(D_METHOD("is_blocking_handshake_enabled"), &StreamPeerTLS::is_blocking_handshake_enabled);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "blocking_handshake"), "set_blocking_handshake_enabled", "is_blocking_handshake_enabled");

	BIND_ENUM_CONSTANT(STATUS_DISCONNECTED);
	BIND_ENUM_CONSTANT(STATUS_HANDSHAKING);
	BIND_ENUM_CONSTANT(STATUS_CONNECTED);
	BIND_ENUM_CONSTANT(STATUS_ERROR);
	BIND_ENUM_CONSTANT(STATUS_ERROR_HOSTNAME_MISMATCH);
}
