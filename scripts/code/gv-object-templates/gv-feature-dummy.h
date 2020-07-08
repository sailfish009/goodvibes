#pragma once

#include <glib-object.h>

#include "base/gv-feature.h"

/* GObject declarations */

#define GV_TYPE_DUMMY gv_dummy_get_type()

G_DECLARE_FINAL_TYPE(GvDummy, gv_dummy, GV, DUMMY, GvFeature)
