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

/* inclusion guard */
#ifndef __CHEESE_FACE_INFO_H__
#define __CHEESE_FACE_INFO_H__

#include <glib.h>
#include <gst/gst.h>
#include <gst/gstminiobject.h>
#include <graphene.h>

G_BEGIN_DECLS

typedef struct _GstCheeseFaceInfo GstCheeseFaceInfo;

typedef enum {
  CHEESE_FACE_LANDMARK_TYPE_5,
  CHEESE_FACE_LANDMARK_TYPE_68,
  CHEESE_FACE_LANDMARK_TYPE_UNKNOWN
} CheeseFaceLandmarkType;

GST_EXPORT
inline gint CHEESE_FACE_LANDMARK_N (CheeseFaceLandmarkType type);
GstCheeseFaceInfo * gst_cheese_face_info_new ();
GstCheeseFaceInfo * gst_cheese_face_info_copy (const GstCheeseFaceInfo * self);
void cheese_face_info_set_bounding_box (GstCheeseFaceInfo * self,
    graphene_rect_t bounding_box);
void cheese_face_info_set_display (GstCheeseFaceInfo * self, gboolean display);
gboolean cheese_face_info_get_display (GstCheeseFaceInfo * self);
void cheese_face_info_set_landmark_keypoints (GstCheeseFaceInfo * self,
    const graphene_point_t * landmark_keypoints, guint n_landmark_keypoints);
graphene_rect_t cheese_face_info_get_bounding_box (GstCheeseFaceInfo * self);
gboolean cheese_face_info_get_eye_rotation (GstCheeseFaceInfo * self,
    gdouble * rot_rad);
GArray * cheese_face_info_get_landmark_keypoints (GstCheeseFaceInfo * self);

G_END_DECLS

#endif /* __CHEESE_FACE_INFO_H__ */
