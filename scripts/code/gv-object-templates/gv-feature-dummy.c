#include <glib-object.h>
#include <glib.h>

#include "base/gv-feature.h"
#include "base/log.h"

#include "core/gv-core.h"

#include "feat/gv-dummy.h"

/*
 * GObject definitions
 */

struct _GvDummy {
	/* Parent instance structure */
	GvFeature parent_instance;
};

G_DEFINE_TYPE(GvDummy, gv_dummy, GV_TYPE_FEATURE)

/*
 * Helpers
 */

/*
 * Feature methods
 */

static void
gv_dummy_disable(GvFeature *feature)
{
	GvPlayer *player = gv_core_player;

	// FILL THAT

	/* Chain up */
	GV_FEATURE_CHAINUP_DISABLE(gv_dummy, feature);
}

static void
gv_dummy_enable(GvFeature *feature)
{
	GvPlayer *player = gv_core_player;

	/* Chain up */
	GV_FEATURE_CHAINUP_ENABLE(gv_dummy, feature);

	// FILL THAT

	/* Signal handlers */
	// g_signal_connect_object(player, "blabla", on_blabla, feature, 0);
}

/*
 * GObject methods
 */

static void
gv_dummy_init(GvDummy *self)
{
	TRACE("%p", self);
}

static void
gv_dummy_class_init(GvDummyClass *class)
{
	GvFeatureClass *feature_class = GV_FEATURE_CLASS(class);

	TRACE("%p", class);

	/* Override GvFeature methods */
	feature_class->enable = gv_dummy_enable;
	feature_class->disable = gv_dummy_disable;
}
