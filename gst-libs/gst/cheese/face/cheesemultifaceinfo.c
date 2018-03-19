/*
 * GStreamer Plugins Cheese
 * Copyright (C) 2018 Fabian Orccon <cfoch.fabian@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "cheesemultifaceinfo.h"

struct _GstCheeseMultifaceInfo {
  GstMiniObject mini_object;

  /* < private > */
  GHashTable *faces;
  gpointer _gst_reserved[GST_PADDING];
};

static void
_gst_cheese_multiface_info_free (GstCheeseMultifaceInfo * self)
{
  g_return_if_fail (self != NULL);
  g_hash_table_unref (self->faces);
  g_slice_free (GstCheeseMultifaceInfo, self);
}

GST_DEFINE_MINI_OBJECT_TYPE (GstCheeseMultifaceInfo, gst_cheese_multiface_info);

/**
 * gst_cheese_multiface_info_new:
 *
 * Allocates a new #GstCheeseMultifaceInfo.
 *
 * Returns: (transfer full): A newly-allocated #GstCheeseMultifaceInfo. Unref
 * with gst_cheese_multiface_info_unref () when no longer needed.
 */
GstCheeseMultifaceInfo *
gst_cheese_multiface_info_new ()
{
  GstCheeseMultifaceInfo *multiface_info;
  multiface_info = g_slice_new0 (GstCheeseMultifaceInfo);
  gst_mini_object_init (GST_MINI_OBJECT_CAST (multiface_info),
      GST_MINI_OBJECT_FLAG_LOCKABLE, gst_cheese_multiface_info_get_type (),
      (GstMiniObjectCopyFunction) gst_cheese_multiface_info_copy,
      (GstMiniObjectDisposeFunction) NULL,
      (GstMiniObjectFreeFunction) _gst_cheese_multiface_info_free);
  multiface_info->faces = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      NULL, (GDestroyNotify) gst_mini_object_unref);
  return multiface_info;
}

GstCheeseMultifaceInfo *
gst_cheese_multiface_info_copy (const GstCheeseMultifaceInfo * self)
{
  GstCheeseMultifaceInfo *ret;
  GHashTableIter iter;
  gpointer key, value;

  ret = gst_cheese_multiface_info_new ();

  g_hash_table_iter_init (&iter, self->faces);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    guint id = GPOINTER_TO_INT (key);
    const GstCheeseFaceInfo *face_info = (GstCheeseFaceInfo *) value;
    g_hash_table_insert (ret->faces, GINT_TO_POINTER (id),
        gst_cheese_face_info_copy (face_info));
  }
  return ret;
}

void
gst_cheese_multiface_info_insert (GstCheeseMultifaceInfo * self, guint i,
    GstCheeseFaceInfo * face_info)
{
  g_hash_table_insert (self->faces, GINT_TO_POINTER (i), face_info);
}

void
gst_cheese_multiface_info_remove (GstCheeseMultifaceInfo * self, guint i)
{
  g_hash_table_remove (self->faces, GINT_TO_POINTER (i));
}

void
gst_cheese_multiface_info_get (GstCheeseMultifaceInfo * self, guint i)
{
  g_hash_table_lookup (self->faces, GINT_TO_POINTER (i));
}

guint
gst_cheese_multiface_info_size (GstCheeseMultifaceInfo * self)
{
  return g_hash_table_size (self->faces);
}

void
gst_cheese_multiface_info_iter_init (GstCheeseMultifaceInfoIter * iter,
    GstCheeseMultifaceInfo * info)
{
  g_hash_table_iter_init ((GHashTableIter *) iter, info->faces);
}

gboolean
gst_cheese_multiface_info_iter_next (GstCheeseMultifaceInfoIter * iter,
    guint * face_id, GstCheeseFaceInfo ** face_info)
{
  gboolean ret;
  gpointer key, value;
  ret = g_hash_table_iter_next ((GHashTableIter *) iter, &key, &value);
  *face_id = GPOINTER_TO_INT (key);
  *face_info = (GstCheeseFaceInfo *) value;
  return ret;
}
