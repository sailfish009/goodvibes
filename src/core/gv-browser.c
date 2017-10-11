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

#include "additions/glib.h"
#include "additions/glib-object.h"

#include "framework/log.h"
#include "core/gv-core-internal.h"
#include "core/gv-station-details.h"

#include "core/gv-browser.h"

#define RADIO_BROWSER_WEBSERVICE "http://www.radio-browser.info/webservice"
#define RADIO_BROWSER_XML         RADIO_BROWSER_WEBSERVICE "/xml"

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
	SoupSession *soup_session;
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

static void
nullify_if_empty(gchar **str)
{
	if (str == NULL)
		return;

	if (*str == NULL)
		return;

	if (**str == '\0') {
		g_free(*str);
		*str = NULL;
	}
}

static void
markup_on_start_element(GMarkupParseContext  *context G_GNUC_UNUSED,
                        const gchar          *element_name,
                        const gchar         **attribute_names,
                        const gchar         **attribute_values,
                        gpointer              user_data,
                        GError              **error)
{
	GList **llink = (GList **) user_data;
	GvStationDetails *details;
	const gchar *click_count_str;

	/* We use error, it needs to be non-NULL */
	g_assert_nonnull(error);

	/* 'result' is the root node */
	if (!g_strcmp0(element_name, "result"))
		return;

	/* From now on we only expect 'station' nodes */
	if (g_strcmp0(element_name, "station")) {
		DEBUG("Unexpected element '%s', ignoring", element_name);
		return;
	}

	/* Allocate a new station details */
	details = gv_station_details_new();

	/* Collect attributes */
	g_markup_collect_attributes_allow_unknown
	(element_name, attribute_names,
	 attribute_values, error,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "id", &details->id,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "name", &details->name,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "homepage", &details->homepage,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "tags", &details->tags,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "country", &details->country,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "state", &details->state,
	 G_MARKUP_COLLECT_STRDUP | G_MARKUP_COLLECT_OPTIONAL,
	 "language", &details->language,
	 G_MARKUP_COLLECT_STRING | G_MARKUP_COLLECT_OPTIONAL,
	 "clickcount", &click_count_str,
	 G_MARKUP_COLLECT_INVALID);

	/* For any other error, return */
	if (*error)
		return;

	/* Get rid of empty strings */
	nullify_if_empty(&details->id);
	nullify_if_empty(&details->name);
	nullify_if_empty(&details->homepage);
	nullify_if_empty(&details->tags);
	nullify_if_empty(&details->country);
	nullify_if_empty(&details->state);
	nullify_if_empty(&details->language);

	/* Parse some fields if needed */
	if (click_count_str)
		details->click_count = g_ascii_strtoll(click_count_str, NULL, 10);

	/* Add to list, use prepend for efficiency */
	*llink = g_list_prepend(*llink, details);
}

static void
markup_on_error(GMarkupParseContext *context G_GNUC_UNUSED,
                GError              *error G_GNUC_UNUSED,
                gpointer             user_data)
{
	GList **llink = (GList **) user_data;

	g_list_free_full(*llink, (GDestroyNotify) gv_station_details_free);
	*llink = NULL;
}

static GList *
parse_search_result(const gchar *text, GError **error)
{
	GMarkupParseContext *context;
	GMarkupParser parser = {
		markup_on_start_element,
		NULL,
		NULL,
		NULL,
		markup_on_error,
	};
	GList *list = NULL;

	context = g_markup_parse_context_new(&parser, 0, &list, NULL);
	g_markup_parse_context_parse(context, text, -1, error);
	g_markup_parse_context_free(context);

	return g_list_reverse(list);
}

/*
 * Signal handlers & callbacks
 */

static void
on_message_completed(SoupSession *session,
                     SoupMessage *msg,
                     GTask *task)
{
	GList *stations;
	GError *error = NULL;

	TRACE("%p, %p, %p", session, msg, task);

	//DEBUG("Res: %s", msg->response_body->data);

	/* Check the response */
	if (!SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		WARNING("Failed to download: %s", msg->reason_phrase);
		g_task_return_new_error(task, SOUP_HTTP_ERROR,
		                        msg->status_code,
		                        "Failed to download: %s",
		                        msg->reason_phrase);
		goto cleanup;
	}

	/* Parse */
	stations = parse_search_result(msg->response_body->data, &error);
//	stations = parse_search_result("afaffa", &error);
	if (error) {
		WARNING("Failed to parse: %s", error->message);
		/* Task takes ownership of error */
		g_task_return_error(task, error);
		goto cleanup;
	}

	/* Return */
	g_task_return_pointer(task, stations, (GDestroyNotify) gv_station_details_list_free);

	/* After that call, 'stations' might be invalid, don't use it anymore */

cleanup:
	g_object_unref(task);
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

GList *
gv_browser_search_finish(GvBrowser     *self,
                         GAsyncResult  *result,
                         GError       **error)
{
	g_return_val_if_fail(g_task_is_valid(result, self), NULL);
	g_return_val_if_fail(error == NULL || *error == NULL, NULL);

	return g_task_propagate_pointer(G_TASK(result), error);
}

void
gv_browser_search_async(GvBrowser           *self,
                        const gchar         *station_name,
                        GAsyncReadyCallback  callback,
                        gpointer             user_data)
{
	GvBrowserPrivate *priv = self->priv;

	g_return_if_fail(station_name != NULL);

	/* Create the task */
	GTask *task;
	task = g_task_new(self, NULL, callback, user_data);

	/* Create valid uri */
	// TODO maybe this can be done automatically by libsoup somehow
	gchar *escaped, *uri;
	escaped = g_uri_escape_string(station_name, NULL, FALSE);

	uri = g_strdup_printf(RADIO_BROWSER_XML "/stations/byname/%s",
	                      escaped);

	// TODO check that with spaces, I don't believe it
	DEBUG("Uri: %s", uri);

	/* Soup it */
	SoupMessage *msg;
	msg = soup_message_new("GET", uri);
	soup_session_queue_message(priv->soup_session, msg,
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

	/* No more soup */
	g_object_unref(priv->soup_session);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_browser, object);
}

static void
gv_browser_constructed(GObject *object)
{
	GvBrowser *self = GV_BROWSER(object);
	GvBrowserPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Create the session */
	const gchar *user_agent = gv_core_user_agent;
	priv->soup_session = soup_session_new_with_options(SOUP_SESSION_USER_AGENT, user_agent,
	                     NULL);

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
