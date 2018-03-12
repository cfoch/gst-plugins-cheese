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

#include <glib.h>
#include "../../gst/face/cheesefacespritekeypoint.h"

static void
test_set_properties ()
{
  CheeseFaceKeypoint face_keypoint;
  CheeseFaceSpriteKeypoint *keypoint;
  gboolean rotate, loop;
  gdouble base_scale_factor;

  keypoint = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_PHILTRUM);

  /* Test keypoint construct property is set */
  g_object_get (G_OBJECT (keypoint), "keypoint", &face_keypoint, NULL);
  g_assert_true (face_keypoint == CHEESE_FACE_KEYPOINT_PHILTRUM);

  /* Test set properties */
  g_object_set (G_OBJECT (keypoint), "keypoint", CHEESE_FACE_KEYPOINT_MOUTH,
      "rotate", TRUE, "loop", FALSE, "base-scale-factor", 0.85, NULL);
  g_object_get (G_OBJECT (keypoint), "keypoint", &face_keypoint,
      "rotate", &rotate, "loop", &loop, "base-scale-factor", &base_scale_factor,
      NULL);
  g_assert (face_keypoint == CHEESE_FACE_KEYPOINT_MOUTH);
  g_assert_true (rotate);
  g_assert_false (loop);
  g_assert_cmpfloat (base_scale_factor, == , 0.85);

  g_object_unref (keypoint);
}

static void
test_new ()
{
  CheeseFaceSpriteKeypoint *keypoint;
  keypoint = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_EYE);
  g_assert_true (CHEESE_IS_FACE_SPRITE_KEYPOINT (keypoint));
  g_object_unref (keypoint);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/cheese/facespritekeypoint/test_new", test_new);
  return g_test_run ();
}
