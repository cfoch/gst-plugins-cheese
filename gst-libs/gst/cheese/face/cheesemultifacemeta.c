/* GStreamer
 * Copyright (C) <2015> British Broadcasting Corporation
 *   Author: Chris Bass <dash@rd.bbc.co.uk>
 * Copyright (C) 2018 Fabian Orccon <cfoch.fabian@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:gstcheesemultifacemeta
 * @title: GstCheeseMultifaceMeta
 * @short_description: Metadata class for faces detection information.
 *
 * The GstCheeseMultifaceMeta class adds coordinate information about read
 * coordinates.
 */

#include "cheesemultifacemeta.h"

GType
gst_cheese_multiface_meta_api_get_type (void)
{
  static volatile GType type;
  static const gchar *tags[] = { NULL };

  if (g_once_init_enter (&type)) {
    GType _type = gst_meta_api_type_register ("GstCheeseMultifaceMetaAPI", tags);
    g_once_init_leave (&type, _type);
  }
  return type;
}

gboolean
gst_cheese_multiface_meta_init (GstMeta * meta, gpointer params,
    GstBuffer * buffer)
{
  GstCheeseMultifaceMeta *cheese_multiface_meta = (GstCheeseMultifaceMeta *) meta;

  cheese_multiface_meta->faces = gst_cheese_multiface_info_new ();
  cheese_multiface_meta->removed_faces =
      g_array_new (FALSE, FALSE, sizeof (guint));
  return TRUE;
}

void
gst_cheese_multiface_meta_free (GstMeta * meta, GstBuffer * buffer)
{
  GstCheeseMultifaceMeta *cheese_multiface_meta;

  cheese_multiface_meta = (GstCheeseMultifaceMeta *) meta;
  gst_mini_object_unref ((GstMiniObject *) cheese_multiface_meta->faces);
  g_array_unref (cheese_multiface_meta->removed_faces);
}

static gboolean
gst_cheese_multiface_meta_transform (GstBuffer * dst_buf, GstMeta * meta,
    GstBuffer * src_buf, GQuark type, gpointer data)
{
  GstCheeseMultifaceMeta *src_meta, *dst_meta;

  /* I am not sure if I should do a deep copy */
  src_meta = (GstCheeseMultifaceMeta *) meta;
  dst_meta = gst_buffer_add_cheese_multiface_meta (dst_buf);
  dst_meta->faces = gst_cheese_multiface_info_copy (src_meta->faces);
  g_array_append_vals (dst_meta->removed_faces, src_meta->removed_faces->data,
      sizeof (guint) * src_meta->removed_faces->len);

  return TRUE;
}

const GstMetaInfo *
gst_cheese_multiface_meta_get_info (void)
{
  static const GstMetaInfo *cheese_multiface_meta_info = NULL;

  if (g_once_init_enter (&cheese_multiface_meta_info)) {
    const GstMetaInfo *meta = gst_meta_register (
        GST_CHEESE_MULTIFACE_META_API_TYPE,
        "GstCheeseMultifaceMeta", sizeof (GstCheeseMultifaceMeta),
        gst_cheese_multiface_meta_init, gst_cheese_multiface_meta_free,
        gst_cheese_multiface_meta_transform);
    g_once_init_leave (&cheese_multiface_meta_info, meta);
  }
  return cheese_multiface_meta_info;
}

void
gst_cheese_multiface_meta_add_removed_face_id (GstCheeseMultifaceMeta * self,
    guint id)
{
  g_array_append_val (self->removed_faces, id);
}

/**
 * gst_buffer_add_cheese_multiface_meta:
 * @buffer: (transfer none): #GstBuffer holding subtitle text, to which
 * subtitle metadata should be added.
 * @regions: (transfer full): A #GPtrArray of #GstSubtitleRegions.
 *
 * Attaches subtitle metadata to a #GstBuffer.
 *
 * Returns: A pointer to the added #GstCheeseMultifaceMeta if successful; %NULL if
 * unsuccessful.
 */
GstCheeseMultifaceMeta *
gst_buffer_add_cheese_multiface_meta (GstBuffer * buffer)
{
  GstCheeseMultifaceMeta *meta;

  g_return_val_if_fail (GST_IS_BUFFER (buffer), NULL);

  meta = (GstCheeseMultifaceMeta *) gst_buffer_add_meta (buffer,
      GST_CHEESE_MULTIFACE_META_INFO, NULL);

  return meta;
}
