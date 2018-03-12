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
#include <stdio.h>
#include <gst/cheese/face/cheesefacespriteframe.h>

static void
test_new_from_location ()
{
  CheeseFaceSpriteFrame *frame;
  GdkPixbuf *pixbuf;
  const guchar inpixdata[] = {255, 100, 220};

  /* Test creating from a NULL location */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
      "*assertion*location*failed");
  frame = cheese_face_sprite_frame_new_from_location (NULL, NULL);
  g_test_assert_expected_messages ();
  g_assert_null (frame);

  /* Test creating from an empty location */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
      "*assertion*location*failed");
  frame = cheese_face_sprite_frame_new_from_location ("", NULL);
  g_test_assert_expected_messages ();
  g_assert_null (frame);


  /* Test creating from a location that no exist */
  g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
      "*assertion*pixbuf*failed");
  frame = cheese_face_sprite_frame_new_from_location ("/foo/bar", NULL);
  g_assert_null (frame);

  /* Prepare PNG file */
  pixbuf = gdk_pixbuf_new_from_data (inpixdata, GDK_COLORSPACE_RGB, FALSE, 8,
      1, 1, 1 * 3, NULL, NULL);
  g_assert_true (gdk_pixbuf_save (pixbuf, "/tmp/test-cheesefacespriteframe.png",
      "png", NULL, NULL));

  /* Test creating from valid location */
  frame = cheese_face_sprite_frame_new_from_location (
      "/tmp/test-cheesefacespriteframe.png", NULL);
  g_assert_true (CHEESE_IS_FACE_SPRITE_FRAME (frame));

  g_object_unref (pixbuf);
  g_object_unref (frame);
}

static void
test_properties ()
{
  CheeseFaceSpriteFrame *frame;
  GdkPixbuf *inpixbuf, *outpixbuf = NULL;
  gchar *outlocation;
  guint duration;
  gdouble base_scale_factor;
  const guchar inpixdata[] = {255, 100, 220};

  /* Prepare pixbuf */
  inpixbuf = gdk_pixbuf_new_from_data (inpixdata, GDK_COLORSPACE_RGB, FALSE, 8,
      1, 1, 1 * 3, NULL, NULL);
  g_assert_nonnull (inpixbuf);
  frame = cheese_face_sprite_frame_new_from_pixbuf (inpixbuf);
  g_assert_true (CHEESE_IS_FACE_SPRITE_FRAME (frame));

  /* Test base scale factor */
  g_object_set (G_OBJECT (frame), "base-scale-factor", 7.0, NULL);
  g_object_get (G_OBJECT (frame), "base-scale-factor", &base_scale_factor,
      NULL);
  g_assert_cmpint (base_scale_factor, ==, 7.0);

  /* Test set duration */
  g_object_set (G_OBJECT (frame), "duration", 6, NULL);
  g_object_get (G_OBJECT (frame), "duration", &duration, NULL);
  g_assert_cmpint (duration, ==, 6);

  /* Test get pixbuf */
  g_object_get (G_OBJECT (frame), "pixbuf", &outpixbuf, NULL);
  g_assert (inpixbuf == outpixbuf);

  /* Test get location */
  g_object_get (G_OBJECT (frame), "location", &outlocation, NULL);
  g_assert_cmpstr (outlocation, ==, NULL);

  g_object_unref (inpixbuf);
  g_object_unref (frame);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/cheese/facespriteframe/test_new_from_location",
      test_new_from_location);
  g_test_add_func ("/cheese/facespriteframe/test_properties", test_properties);
  return g_test_run ();
}
