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

#include <math.h>
#include "cheesefaceinfo.h"

struct _GstCheeseFaceInfo {
  GstMiniObject mini_object;

  /* < private > */
  graphene_rect_t bounding_box;
  gboolean display;
  GArray *landmark_keypoints;
  gpointer _gst_reserved[GST_PADDING];
};

static void
_gst_cheese_face_info_free (GstCheeseFaceInfo * self)
{
  g_return_if_fail (self != NULL);
  g_array_unref (self->landmark_keypoints);
  g_slice_free (GstCheeseFaceInfo, self);
}

GST_DEFINE_MINI_OBJECT_TYPE (GstCheeseFaceInfo, gst_cheese_face_info);

/**
 * gst_cheese_face_info_new:
 *
 * Allocates a new #GstCheeseFaceInfo.
 *
 * Returns: (transfer full): A newly-allocated #GstCheeseFaceInfo. Unref
 * with gst_cheese_face_info_unref () when no longer needed.
 */
GstCheeseFaceInfo *
gst_cheese_face_info_new ()
{
  GstCheeseFaceInfo *face_info;
  face_info = g_slice_new0 (GstCheeseFaceInfo);
  gst_mini_object_init (GST_MINI_OBJECT_CAST (face_info),
      GST_MINI_OBJECT_FLAG_LOCKABLE, gst_cheese_face_info_get_type (),
      (GstMiniObjectCopyFunction) gst_cheese_face_info_copy,
      (GstMiniObjectDisposeFunction) NULL,
      (GstMiniObjectFreeFunction) _gst_cheese_face_info_free);
  face_info->landmark_keypoints = g_array_sized_new (FALSE, FALSE,
      sizeof (graphene_point_t),
      sizeof (graphene_point_t) *
      CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_68));
  return face_info;
}

GstCheeseFaceInfo *
gst_cheese_face_info_copy (const GstCheeseFaceInfo * self)
{
  GstCheeseFaceInfo *ret;
  ret = gst_cheese_face_info_new ();
  ret->bounding_box = self->bounding_box;
  ret->display = self->display;

  g_array_append_vals (ret->landmark_keypoints, self->landmark_keypoints->data,
      self->landmark_keypoints->len);

  return ret;
}

void
cheese_face_info_set_bounding_box (GstCheeseFaceInfo * self,
    graphene_rect_t bounding_box)
{
  self->bounding_box = bounding_box;
}

void
cheese_face_info_set_display (GstCheeseFaceInfo * self, gboolean display)
{
  self->display = display;
}

gboolean
cheese_face_info_get_display (GstCheeseFaceInfo * self)
{
  return self->display;
}

void
cheese_face_info_set_landmark_keypoints (GstCheeseFaceInfo * self,
    const graphene_point_t * landmark_keypoints, guint n_landmark_keypoints)
{
  if (n_landmark_keypoints !=
      CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_68)) {
    g_warning ("Landmarks of %d facial keypoints are not allowed.",
        n_landmark_keypoints);
    return;
  }
  g_array_append_vals (self->landmark_keypoints, landmark_keypoints,
      n_landmark_keypoints);
}

graphene_rect_t
cheese_face_info_get_bounding_box (GstCheeseFaceInfo * self)
{
  return self->bounding_box;
}

CheeseFaceLandmarkType
cheese_face_info_get_landmark_type (GstCheeseFaceInfo * self)
{
  if (self->landmark_keypoints->len ==
        CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_5))
    return CHEESE_FACE_LANDMARK_TYPE_5;
  if (self->landmark_keypoints->len ==
        CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_68))
    return CHEESE_FACE_LANDMARK_TYPE_68;
  return CHEESE_FACE_LANDMARK_TYPE_UNKNOWN;
}

gboolean
cheese_face_info_get_eye_rotation (GstCheeseFaceInfo * self, gdouble * rot_rad)
{
  gboolean ret;
  graphene_point_t *left_eye_ptr, *right_eye_ptr;
  gfloat x, y;
  CheeseFaceLandmarkType landmark_type;

  landmark_type = cheese_face_info_get_landmark_type (self);
  switch (landmark_type) {
    case CHEESE_FACE_LANDMARK_TYPE_5:
      g_warning ("Landmark of %d keypoints not supported.",
          CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_5));
      ret = FALSE;
      break;
    case CHEESE_FACE_LANDMARK_TYPE_68:
      left_eye_ptr = &g_array_index (self->landmark_keypoints, graphene_point_t,
          39);
      right_eye_ptr = &g_array_index (self->landmark_keypoints,
          graphene_point_t, 42);
      x = right_eye_ptr->x - left_eye_ptr->x;
      y = left_eye_ptr->y - right_eye_ptr->y;
      *rot_rad = atan2 (y, x);
      ret = TRUE;
      break;
    default:
      ret = FALSE;
  }
  return ret;
}

GArray *
cheese_face_info_get_landmark_keypoints (GstCheeseFaceInfo * self)
{
  return self->landmark_keypoints;
}
