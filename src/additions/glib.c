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

#include <string.h>
#include <glib.h>

/*
 * Version Information
 */

const gchar *
glib_get_runtime_version_string(void)
{
	static gchar *version_string;

	if (version_string == NULL) {
		version_string = g_strdup_printf("GLib %u.%u.%u",
		                                 glib_major_version,
		                                 glib_minor_version,
		                                 glib_micro_version);
	}

	return version_string;
}

const gchar *
glib_get_compile_version_string(void)
{
	static gchar *version_string;

	if (version_string == NULL) {
		version_string = g_strdup_printf("GLib %u.%u.%u",
		                                 GLIB_MAJOR_VERSION,
		                                 GLIB_MINOR_VERSION,
		                                 GLIB_MICRO_VERSION);
	}

	return version_string;
}

/*
 * String Utilities Functions
 */

gchar *
g_strjoin_null(const gchar *separator, unsigned int n_strings, ...)
{
	gchar *string, *s;
	va_list args;
	gsize len;
	gsize separator_len;
	gchar *ptr;
	guint i;

	if (separator == NULL)
		separator = "";

	separator_len = strlen(separator);

	/* First part, getting length */
	len = 1;
	va_start(args, n_strings);
	for (i = 0; i < n_strings; i++) {
		s = va_arg(args, gchar *);
		if (s) {
			if (len != 1)
				len += separator_len;
			len += strlen(s);
		}
	}
	va_end(args);

	/* Second part, building string */
	string = g_new(gchar, len);
	ptr = string;
	va_start(args, n_strings);
	for (i = 0; i < n_strings; i++) {
		s = va_arg(args, gchar *);
		if (s) {
			if (ptr != string)
				ptr = g_stpcpy(ptr, separator);
			ptr = g_stpcpy(ptr, s);
		}
	}
	va_end(args);

	if (ptr == string)
		string[0] = '\0';

	return string;
}

/*
 * GMarkup
 * - g_markup_collect_attributes copied verbatim, the only difference being that,
 *   in case we don't collect every element, we still succeed.
 */

static gboolean
g_markup_parse_boolean (const char  *string,
                        gboolean    *value)
{
  char const * const falses[] = { "false", "f", "no", "n", "0" };
  char const * const trues[] = { "true", "t", "yes", "y", "1" };
  unsigned int i;

  for (i = 0; i < G_N_ELEMENTS (falses); i++)
    {
      if (g_ascii_strcasecmp (string, falses[i]) == 0)
        {
          if (value != NULL)
            *value = FALSE;

          return TRUE;
        }
    }

  for (i = 0; i < G_N_ELEMENTS (trues); i++)
    {
      if (g_ascii_strcasecmp (string, trues[i]) == 0)
        {
          if (value != NULL)
            *value = TRUE;

          return TRUE;
        }
    }

  return FALSE;
}

gboolean
g_markup_collect_attributes_allow_unknown(const gchar         *element_name,
                                          const gchar        **attribute_names,
                                          const gchar        **attribute_values,
                                          GError             **error,
                                          GMarkupCollectType   first_type,
                                          const gchar         *first_attr,
                                          ...)
{
  GMarkupCollectType type;
  const gchar *attr;
  guint64 collected;
  int written;
  va_list ap;
  int i;

  type = first_type;
  attr = first_attr;
  collected = 0;
  written = 0;

  va_start (ap, first_attr);
  while (type != G_MARKUP_COLLECT_INVALID)
    {
      gboolean mandatory;
      const gchar *value;

      mandatory = !(type & G_MARKUP_COLLECT_OPTIONAL);
      type &= (G_MARKUP_COLLECT_OPTIONAL - 1);

      /* tristate records a value != TRUE and != FALSE
       * for the case where the attribute is missing
       */
      if (type == G_MARKUP_COLLECT_TRISTATE)
        mandatory = FALSE;

      for (i = 0; attribute_names[i]; i++)
        if (i >= 40 || !(collected & (G_GUINT64_CONSTANT(1) << i)))
          if (!strcmp (attribute_names[i], attr))
            break;

      /* ISO C99 only promises that the user can pass up to 127 arguments.
       * Subtracting the first 4 arguments plus the final NULL and dividing
       * by 3 arguments per collected attribute, we are left with a maximum
       * number of supported attributes of (127 - 5) / 3 = 40.
       *
       * In reality, nobody is ever going to call us with anywhere close to
       * 40 attributes to collect, so it is safe to assume that if i > 40
       * then the user has given some invalid or repeated arguments.  These
       * problems will be caught and reported at the end of the function.
       *
       * We know at this point that we have an error, but we don't know
       * what error it is, so just continue...
       */
      if (i < 40)
        collected |= (G_GUINT64_CONSTANT(1) << i);

      value = attribute_values[i];

      if (value == NULL && mandatory)
        {
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_MISSING_ATTRIBUTE,
                       "element '%s' requires attribute '%s'",
                       element_name, attr);

          va_end (ap);
          goto failure;
        }
      switch (type)
        {
        case G_MARKUP_COLLECT_STRING:
          {
            const char **str_ptr;

            str_ptr = va_arg (ap, const char **);

            if (str_ptr != NULL)
              *str_ptr = value;
          }
          break;

        case G_MARKUP_COLLECT_STRDUP:
          {
            char **str_ptr;

            str_ptr = va_arg (ap, char **);

            if (str_ptr != NULL)
              *str_ptr = g_strdup (value);
          }
          break;

        case G_MARKUP_COLLECT_BOOLEAN:
        case G_MARKUP_COLLECT_TRISTATE:
          if (value == NULL)
            {
              gboolean *bool_ptr;

              bool_ptr = va_arg (ap, gboolean *);

              if (bool_ptr != NULL)
                {
                  if (type == G_MARKUP_COLLECT_TRISTATE)
                    /* constructivists rejoice!
                     * neither false nor true...
                     */
                    *bool_ptr = -1;

                  else /* G_MARKUP_COLLECT_BOOLEAN */
                    *bool_ptr = FALSE;
                }
            }
          else
            {
              if (!g_markup_parse_boolean (value, va_arg (ap, gboolean *)))
                {
                  g_set_error (error, G_MARKUP_ERROR,
                               G_MARKUP_ERROR_INVALID_CONTENT,
                               "element '%s', attribute '%s', value '%s' "
                               "cannot be parsed as a boolean value",
                               element_name, attr, value);

                  va_end (ap);
                  goto failure;
                }
            }

          break;

        default:
          g_assert_not_reached ();
        }

      type = va_arg (ap, GMarkupCollectType);
      attr = va_arg (ap, const char *);
      written++;
    }
  va_end (ap);

  /* ensure we collected all the arguments */
  for (i = 0; attribute_names[i]; i++)
    if ((collected & (G_GUINT64_CONSTANT(1) << i)) == 0)
      {
        /* attribute not collected:  could be caused by two things.
         *
         * 1) it doesn't exist in our list of attributes
         * 2) it existed but was matched by a duplicate attribute earlier
         *
         * find out.
         */
        int j;

        for (j = 0; j < i; j++)
          if (strcmp (attribute_names[i], attribute_names[j]) == 0)
            /* duplicate! */
            break;

        /* j is now the first occurrence of attribute_names[i] */
        if (i == j)
	  /* unknown attributes are allowed, this is not an error */
	  goto success;

        else
          g_set_error (error, G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       "attribute '%s' given multiple times for element '%s'",
                       attribute_names[i], element_name);

        goto failure;
      }

success:
  return TRUE;

failure:
  /* replay the above to free allocations */
  type = first_type;
  attr = first_attr;

  va_start (ap, first_attr);
  while (type != G_MARKUP_COLLECT_INVALID)
    {
      gpointer ptr;

      ptr = va_arg (ap, gpointer);

      if (ptr != NULL)
        {
          switch (type & (G_MARKUP_COLLECT_OPTIONAL - 1))
            {
            case G_MARKUP_COLLECT_STRDUP:
              if (written)
                g_free (*(char **) ptr);

            case G_MARKUP_COLLECT_STRING:
              *(char **) ptr = NULL;
              break;

            case G_MARKUP_COLLECT_BOOLEAN:
              *(gboolean *) ptr = FALSE;
              break;

            case G_MARKUP_COLLECT_TRISTATE:
              *(gboolean *) ptr = -1;
              break;
            }
        }

      type = va_arg (ap, GMarkupCollectType);
      attr = va_arg (ap, const char *);
    }
  va_end (ap);

  return FALSE;
}

/*
 * GVariant
 */

void
g_variant_builder_add_dictentry_array_string(GVariantBuilder *b, const gchar *key, ...)
{
	GVariantBuilder ab;
	va_list ap;
	gchar *s;

	g_variant_builder_init(&ab, G_VARIANT_TYPE_ARRAY);

	va_start(ap, key);
	s = va_arg(ap, gchar *);
	while (s) {
		GVariant *tmp = g_variant_new_string(s);
		g_variant_builder_add(&ab, "v", tmp);
		s = va_arg(ap, gchar *);
	}

	g_variant_builder_add(b, "{sv}", key, g_variant_builder_end(&ab));
}
