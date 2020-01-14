#pragma once

#include <glib-object.h>

/* GObject declarations */

#define GV_TYPE_DUMMY gv_dummy_get_type()

G_DECLARE_FINAL_TYPE(GvDummy, gv_dummy, GV, DUMMY, GObject)

/* Data types */

/* Methods */

GvDummy *gv_dummy_new(void);

/* Property accessors */
