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
#include <gst/cheese/face/cheesefacespritekeypoint.h>
#include <gst/cheese/face/cheesefacespriteframe.h>

static void
test_get_frame_at ()
{
  CheeseFaceSpriteKeypoint *keypoint;
  CheeseFaceSpriteFrame *frame0, *frame1, *frame2;
  GdkPixbuf *pixbuf;
  const guchar pixdata[] = {255, 100, 220};
  pixbuf = gdk_pixbuf_new_from_data (pixdata, GDK_COLORSPACE_RGB, FALSE, 8,
      1, 1, 1 * 3, NULL, NULL);

  keypoint = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_PHILTRUM);
  frame0 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);
  frame1 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);
  frame2 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);

  cheese_face_sprite_keypoint_add_frame (keypoint, frame0);
  cheese_face_sprite_keypoint_add_frame (keypoint, frame1);
  cheese_face_sprite_keypoint_add_frame (keypoint, frame2);

  g_object_set (G_OBJECT (frame0), "duration", 30, NULL);
  g_object_set (G_OBJECT (frame1), "duration", 50, NULL);
  g_object_set (G_OBJECT (frame2), "duration", 20, NULL);

  g_assert_cmpint (cheese_face_sprite_keypoint_get_duration (keypoint), ==,
      100);

  g_object_set (G_OBJECT (keypoint), "loop", FALSE, NULL);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 1) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 15) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 30) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 31) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 55) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 80) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 81) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 90) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 100) == frame2);
  /* Out of bounds frames */
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 101) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 500) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 800) == frame2);

  g_object_set (G_OBJECT (keypoint), "loop", TRUE, NULL);
  /* Out of bound frames */
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 101) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 115) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 130) == frame0);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 131) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 155) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 180) == frame1);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 181) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 190) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 200) == frame2);
  g_assert (cheese_face_sprite_keypoint_get_frame_at (keypoint, 201) == frame0);

  g_object_unref (keypoint);
}

static void
test_frames ()
{
  CheeseFaceSpriteKeypoint *keypoint;
  CheeseFaceSpriteFrame *frame0, *frame1, *frame2, *frame_out;
  GdkPixbuf *pixbuf;
  const guchar pixdata[] = {255, 100, 220};

  /* Prepare pixbuf */
  pixbuf = gdk_pixbuf_new_from_data (pixdata, GDK_COLORSPACE_RGB, FALSE, 8,
      1, 1, 1 * 3, NULL, NULL);

  keypoint = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_PHILTRUM);
  frame0 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);
  frame1 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);
  frame2 = cheese_face_sprite_frame_new_from_pixbuf (pixbuf);

  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (keypoint), ==, 0);
  cheese_face_sprite_keypoint_add_frame (keypoint, frame0);
  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (keypoint), ==, 1);
  cheese_face_sprite_keypoint_add_frame (keypoint, frame1);
  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (keypoint), ==, 2);
  cheese_face_sprite_keypoint_add_frame (keypoint, frame2);
  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (keypoint), ==, 3);

  frame_out = cheese_face_sprite_keypoint_get_frame (keypoint, 2);
  g_assert (frame_out == frame2);
  frame_out = cheese_face_sprite_keypoint_get_frame (keypoint, 0);
  g_assert (frame_out == frame0);
  frame_out = cheese_face_sprite_keypoint_get_frame (keypoint, 1);
  g_assert (frame_out == frame1);
  frame_out = cheese_face_sprite_keypoint_get_frame (keypoint, 99);
  g_assert_null (frame_out);

  g_object_unref (pixbuf);
  g_object_unref (keypoint);
}

static void
test_properties ()
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
  keypoint = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_LEFT_EYE);
  g_assert_true (CHEESE_IS_FACE_SPRITE_KEYPOINT (keypoint));
  g_object_unref (keypoint);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/cheese/facespritekeypoint/test_new", test_new);
  g_test_add_func ("/cheese/facespritekeypoint/test_properties",
      test_properties);
  g_test_add_func ("/cheese/facespritekeypoint/test_frames", test_frames);
  g_test_add_func ("/cheese/facespritekeypoint/test_get_frame_at",
      test_get_frame_at);
  return g_test_run ();
}
