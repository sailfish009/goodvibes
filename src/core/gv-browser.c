/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <libsoup/soup.h>

#include "additions/glib-object.h"

#include "framework/log.h"
#include "core/gv-core-internal.h"

#include "core/gv-browser.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	// TODO fill with your properties
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

/*
 * GObject definitions
 */

struct _GvBrowserPrivate {
	// TODO fill with your data
};

typedef struct _GvBrowserPrivate GvBrowserPrivate;

struct _GvBrowser {
	/* Parent instance structure */
	GObject parent_instance;
	/* Private data */
	GvBrowserPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvBrowser, gv_browser, G_TYPE_OBJECT)

/*
 * Helpers
 */

/*
 * Signal handlers & callbacks
 */

static void
on_message_completed(SoupSession *session,
                     SoupMessage *msg,
                     GTask *task)
{
	TRACE("%p, %p, %p", session, msg, task);

	DEBUG("Res: %s", msg->response_body->data);

	/* Check the response */
	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		g_task_return_new_error(task, SOUP_HTTP_ERROR,
		                        msg->status_code,
		                        "Failed to download: %s",
		                        msg->reason_phrase);
		goto cleanup;
	}

	/* Parse */
	// TODO
	// We should get a new object here
	gpointer res = NULL;

	/* Return */
	// TODO: a correct unref function
	g_task_return_pointer(task, res, g_object_unref);
	/* After that call, res might be invalid, don't use it anymore */

cleanup:
	g_object_unref(task);

	/* No more soup */
	// TODO Is it ok to unref that here ?
	g_object_unref(session);
}

/*
 * Property accessors
 */

static void
gv_browser_get_property(GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
	GvBrowser *self = GV_BROWSER(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_browser_set_property(GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	GvBrowser *self = GV_BROWSER(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

// TODO fix pointer
gpointer
gv_browser_search_finish(GvBrowser     *self,
                         GAsyncResult  *result,
                         GError       **error)
{
	g_return_val_if_fail(g_task_is_valid(result, self), NULL);

	return g_task_propagate_pointer(G_TASK(result), error);
}

void
gv_browser_search_async(GvBrowser           *self,
                        const gchar         *station_name,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
	g_return_if_fail(station_name != NULL);

	/* Create the task */
	GTask *task;
	task = g_task_new(self, NULL, callback, user_data);

	/* Create valid uri */
	// TODO maybe this can be done automatically by libsoup somehow
	gchar *escaped, *uri;
	escaped = g_uri_escape_string(station_name, NULL, FALSE);

	uri = g_strdup_printf("%s/%s",
	                      "http://www.radio-browser.info/webservice/json/stations/byname",
	                      escaped);

	// TODO check that with spaces, I don't believe it
	DEBUG("Uri: %s", uri);

	/* Soup it */
	// TODO: user-agent
	SoupSession *session;
	SoupMessage *msg;
	const gchar *user_agent = gv_core_user_agent;

	DEBUG("Downloading playlist '%s' (user-agent: '%s')", uri, user_agent);
	session = soup_session_new_with_options(SOUP_SESSION_USER_AGENT, user_agent,
	                                        NULL);
	msg = soup_message_new("GET", uri);

	soup_session_queue_message(session, msg,
	                           (SoupSessionCallback) on_message_completed,
	                           task);

	/* Cleanup */
	g_free(escaped);
	g_free(uri);
}

GvBrowser *
gv_browser_new(void)
{
	return g_object_new(GV_TYPE_BROWSER, NULL);
}

/*
 * GObject methods
 */

static void
gv_browser_finalize(GObject *object)
{
	GvBrowser *self = GV_BROWSER(object);
	GvBrowserPrivate *priv = self->priv;

	TRACE("%p", object);

	// TODO job to be done
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_browser, object);
}

static void
gv_browser_constructed(GObject *object)
{
	GvBrowser *self = GV_BROWSER(object);
	GvBrowserPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Initialize properties */
	// TODO
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_browser, object);
}

static void
gv_browser_init(GvBrowser *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_browser_get_instance_private(self);
}

static void
gv_browser_class_init(GvBrowserClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_browser_finalize;
	object_class->constructed = gv_browser_constructed;

	/* Properties */
	object_class->get_property = gv_browser_get_property;
	object_class->set_property = gv_browser_set_property;

	// TODO define your properties here
	//      use GV_PARAM_DEFAULT_FLAGS

	g_object_class_install_properties(object_class, PROP_N, properties);
}
