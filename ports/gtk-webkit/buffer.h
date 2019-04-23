/*
Copyright © 2018-2019 Atlas Engineer LLC.
Use of this file is governed by the license that can be found in LICENSE.
*/
#pragma once

#include <webkit2/webkit2.h>
#include <JavaScriptCore/JavaScript.h>

#include "javascript.h"

typedef struct {
	WebKitWebView *web_view;
	int callback_count;
	char *identifier;
} Buffer;

typedef struct {
	Buffer *buffer;
	int callback_id;
} BufferInfo;


static void buffer_web_view_load_changed(WebKitWebView *web_view,
	WebKitLoadEvent load_event,
	gpointer data) {
	const char *uri = webkit_web_view_get_uri(web_view);
	const char *method_name = "buffer.did.commit.navigation";

	switch (load_event) {
	case WEBKIT_LOAD_STARTED:
		/* New load, we have now a provisional URI */
		/* Here we could start a spinner or update the
		 * location bar with the provisional URI */
		break;
	case WEBKIT_LOAD_REDIRECTED:
		// TODO: Let the core know that we have been redirected?
		break;
	case WEBKIT_LOAD_COMMITTED:
		/* The load is being performed. Current URI is
		 * the final one and it won't change unless a new
		 * load is requested or a navigation within the
		 * same page is performed */
		uri = webkit_web_view_get_uri(web_view); // TODO: Only need to set URI at the beginning?

		// TODO: Notify Lisp core on invalid TLS certificate, leave to the Lisp core
		// the possibility to load the non-HTTPS URL.
		if (g_str_has_prefix(uri, "https://")) {
			GTlsCertificateFlags tls_flags;
			if (webkit_web_view_get_tls_info(web_view, NULL, &tls_flags) && tls_flags) {
				g_warning("Invalid TLS certificate");
			}
		}

		break;
	case WEBKIT_LOAD_FINISHED:
		/* Load finished, we can now stop the spinner */
		method_name = "buffer.did.finish.navigation";
	}

	if (uri == NULL) {
		return;
	}

	g_debug("Load changed: %s", uri);

	Buffer *buffer = data;
	GError *error = NULL;
	GVariant *arg = g_variant_new("(ss)", buffer->identifier, uri);
	g_message("XML-RPC message: %s %s", method_name, g_variant_print(arg, TRUE));

	SoupMessage *msg = soup_xmlrpc_message_new(state.core_socket,
			method_name, arg, &error);

	if (error) {
		g_warning("Malformed XML-RPC message: %s", error->message);
		g_error_free(error);
		return;
	}
	soup_session_queue_message(xmlrpc_env, msg, NULL, NULL);
	// 'msg' and 'uri' are freed automatically.
}

typedef struct  {
	WebKitPolicyDecision *decision;
	const gchar *uri;
} DecisionInfo;

void buffer_navigated_callback(SoupSession *session, SoupMessage *msg, gpointer data) {
	GError *error = NULL;
	g_debug("Buffer navigation XML-RPC response: %s", msg->response_body->data);

	// TODO: Use boolean instead of integer when Atlas' s-xml-rpc package is in
	// Quicklisp.
	GVariant *loadv = soup_xmlrpc_parse_response(msg->response_body->data,
			msg->response_body->length, "i", &error);

	if (error) {
		g_warning("%s: '%s'", error->message,
			strndup(msg->response_body->data, msg->response_body->length));
		g_error_free(error);
		return;
	}

	gint32 load = g_variant_get_int32(loadv);

	DecisionInfo *decision_info = data;
	WebKitPolicyDecision *decision = decision_info->decision;
	if (load != 0) {
		// TODO: Should we download or use when it's a RESPONSE?
		g_debug("Load resource '%s'", decision_info->uri);
		webkit_policy_decision_use(decision);
	} else {
		g_debug("Ignore resource '%s'", decision_info->uri);
		webkit_policy_decision_ignore(decision);
	}

	g_free(decision_info);
	return;
}

gboolean buffer_web_view_decide_policy(WebKitWebView *web_view,
	WebKitPolicyDecision *decision, WebKitPolicyDecisionType type, gpointer bufferp) {
	WebKitNavigationAction *action = NULL;

	gboolean is_new_window = false;
	gboolean is_known_type = true;
	switch (type) {
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION: {
		action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
		break;
	}
	case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION: {
		is_new_window = true;
		action = webkit_navigation_policy_decision_get_navigation_action(WEBKIT_NAVIGATION_POLICY_DECISION(decision));
		break;
	}
	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE: {
		if (!webkit_response_policy_decision_is_mime_type_supported(WEBKIT_RESPONSE_POLICY_DECISION(decision))) {
			is_known_type = false;
		}
		break;
	}
	}

	WebKitURIRequest *request;
	if (action) {
		request = webkit_navigation_action_get_request(action);
	} else {
		request = webkit_response_policy_decision_get_request(WEBKIT_RESPONSE_POLICY_DECISION(decision));
	}
	const gchar *uri = webkit_uri_request_get_uri(request);

	gchar *event_type = "other";
	if (action) {
		switch (webkit_navigation_action_get_navigation_type(action)) {
		case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED: {
			event_type = "link-click";
			break;
		}
		case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED: {
			event_type = "form-submission";
			break;
		}
		case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD: {
			event_type = "backward-of-forward";
			break;
		}
		case WEBKIT_NAVIGATION_TYPE_RELOAD: {
			event_type = "reload";
			break;
		}
		case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED: {
			event_type = "form-resubmission";
			break;
		}
		case WEBKIT_NAVIGATION_TYPE_OTHER: {
			break;
		}
		}
	}

	// No need for webkit_navigation_action_is_user_gesture if mouse_button and
	// modifiers tell us the same information.
	guint mouse_button = 0;
	guint modifiers = 0;
	if (action) {
		mouse_button = webkit_navigation_action_get_mouse_button(action);
		modifiers = webkit_navigation_action_get_modifiers(action);
	}

	const char *method_name = "request.resource";

	// TODO: Encode mouse + modifiers properly.
	// TODO: Test if it's a redirect?
	Buffer *buffer = bufferp;
	GVariant *arg = g_variant_new("(sssbbi)", buffer->identifier, uri,
			event_type,
			is_new_window,
			is_known_type,
			mouse_button+modifiers);
	g_message("XML-RPC message: %s (buffer id, URI, event_type, is_new_window, is_known_type, input) = %s", method_name, g_variant_print(arg, TRUE));

	GError *error = NULL;
	SoupMessage *msg = soup_xmlrpc_message_new(state.core_socket,
			method_name, arg, &error);

	if (error) {
		g_warning("Malformed XML-RPC message: %s", error->message);
		g_error_free(error);
		// TODO: Return TRUE or FALSE?
		return FALSE;
	}

	DecisionInfo *decision_info = g_new(DecisionInfo, 1);
	decision_info->decision = decision;
	decision_info->uri = uri;
	soup_session_queue_message(xmlrpc_env, msg, (SoupSessionCallback)buffer_navigated_callback, decision_info);

	// Keep a reference on the decision so that in won't be freed before the callback.
	g_object_ref(decision);
	return TRUE;
}

void buffer_set_cookie_file(Buffer *buffer, const char *path) {
	if (path == NULL) {
		return;
	}
	WebKitCookieManager *cookie_manager =
		webkit_web_context_get_cookie_manager(webkit_web_view_get_context(buffer->web_view));
	webkit_cookie_manager_set_persistent_storage(cookie_manager,
		path,
	        // TODO: Make format configurable?
		WEBKIT_COOKIE_PERSISTENT_STORAGE_TEXT);
}

// TODO: Remove this once all downloads have been transfered to the Lisp core.
void buffer_web_view_download_started(WebKitWebContext *context,
	WebKitDownload *download, Buffer *buffer) {
	const char *uri = webkit_uri_request_get_uri(webkit_download_get_request(download));
	g_warning("Download starting: %s", uri);
	/*
	Signal handlers accessible from here:
	        - decide-destination
	        - failed
	        - finished
	        - received-data
	*/
}

gboolean buffer_web_view_web_process_crashed(WebKitWebView *web_view, Buffer *buffer) {
	g_warning("Buffer %s web process crashed", buffer->identifier);
	return FALSE;
}

Buffer *buffer_init(const char *cookie_file) {
	Buffer *buffer = calloc(1, sizeof (Buffer));
	buffer->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	buffer_set_cookie_file(buffer, cookie_file);

	g_signal_connect(buffer->web_view, "load-changed",
		G_CALLBACK(buffer_web_view_load_changed), buffer);
	g_signal_connect(buffer->web_view, "decide-policy",
		G_CALLBACK(buffer_web_view_decide_policy), buffer);
	g_signal_connect(buffer->web_view, "web-process-crashed",
		G_CALLBACK(buffer_web_view_web_process_crashed), buffer);

	WebKitWebContext *context = webkit_web_view_get_context(buffer->web_view);
	g_signal_connect(context, "download-started", G_CALLBACK(buffer_web_view_download_started), buffer);

	// We need to hold a reference to the view, otherwise changing buffer in the a
	// window will unref+destroy the view.
	g_object_ref(buffer->web_view);
	g_debug("Init buffer %p with view %p", buffer, buffer->web_view);
	buffer->callback_count = 0;
	// So far we leave the core to set the default URL, otherwise the load-changed
	// signal would be emitted while the buffer identifier is still empty.
	return buffer;
}

void buffer_delete(Buffer *buffer) {
	// Remove the extra ref added in buffer_init()?
	/* g_object_unref(buffer->web_view); */
	// TODO: What happens to the Window's web view when current buffer is deleted?

	gtk_widget_destroy(GTK_WIDGET(buffer->web_view));
	g_free(buffer->identifier);
	g_free(buffer);
}

void buffer_load(Buffer *buffer, const char *uri) {
	webkit_web_view_load_uri(buffer->web_view, uri);
}

static void buffer_javascript_callback(GObject *object, GAsyncResult *result,
	gpointer user_data) {
	BufferInfo *buffer_info = (BufferInfo *)user_data;
	javascript_transform_result(object, result, buffer_info->buffer->identifier,
		buffer_info->callback_id);
	g_free(buffer_info);
}

// Caller must free the result.
char *buffer_evaluate(Buffer *buffer, const char *javascript) {
	// If another buffer_evaluate is run before the callback is called, there will
	// be a race condition upon accessing callback_count.
	// Thus we send a copy of callback_count via a BufferInfo to the callback.
	// The BufferInfo must be freed in the callback.
	BufferInfo *buffer_info = g_new(BufferInfo, 1);
	buffer_info->buffer = buffer;
	buffer_info->callback_id = buffer->callback_count;

	buffer->callback_count++;

	webkit_web_view_run_javascript(buffer->web_view, javascript,
		NULL, buffer_javascript_callback, buffer_info);
	g_debug("buffer_evaluate callback count: %i", buffer_info->callback_id);
	return g_strdup_printf("%i", buffer_info->callback_id);
}
