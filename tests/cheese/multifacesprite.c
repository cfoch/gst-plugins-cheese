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
#include "../../gst/face/cheesemultifacesprite.h"

static const gchar *INVALID_JSON_NO_FRAMES_MEMBER = "[{\"philtrum\": {}}]";
static const gchar *INVALID_JSON_FRAMES_EMPTY =
    "[{\"ear\": {\"frames\": []}}]";
static const gchar *INVALID_JSON_FRAME_WITHOUT_LOCATION =
    "[{\"ear\": {\"frames\": [{\"duration\": 10}]}}]";
static const gchar *INVALID_JSON_FRAME_WITH_INVALID_LOCATION =
    "[{\"ear\": {\"frames\": [{\"location\": \"foomambar\"}]}}]";
static const gchar *INVALID_JSON_FRAME_WITH_VALID_LOCATION =
    "[{\"ear\": {\"frames\": [{\"location\": \"foomambar\"}]}}]";
static const gchar *VALID_JSON_TEMPLATE =
    "["
    "    {"
    "        \"ear\": {"
    "            \"rotate\": true,"
    "            \"loop\": false,"
    "            \"base-scale-factor\": 1.43,"
    "            \"frames\": ["
    "                {"
    "                    \"base-scale-factor\": 1.5,"
    "                    \"duration\": 2,"
    "                    \"location\": \"/tmp/test-cheesemultifacesprite.png\""
    "                },"
    "                {"
    "                    \"base-scale-factor\": 1.2,"
    "                    \"duration\": 10,"
    "                    \"location\": \"/tmp/test-cheesemultifacesprite.png\""
    "                }"
    "            ]"
    "        }"
    "    },"
    "    {"
    "        \"philtrum\": {"
    "            \"frames\": ["
    "                {"
    "                    \"location\": \"/tmp/test-cheesemultifacesprite.png\""
    "                }"
    "            ]"
    "        },"
    "        \"mouth\": {"
    "            \"frames\": ["
    "                {"
    "                    \"location\": \"/tmp/test-cheesemultifacesprite.png\""
    "                }"
    "            ]"
    "        }"
    "    }"
    "]";

static void
test_parse_from_valid_string ()
{
  GError *error = NULL;
  CheeseMultifaceSprite *sprite;
  CheeseFaceSprite *sprite_face;
  CheeseFaceSpriteKeypoint *sprite_keypoint;
  CheeseFaceSpriteFrame *sprite_frame;
  CheeseFaceKeypoint keypoint_type;
  GdkPixbuf *pixbuf, *pixbuf_internal;
  gboolean loop, rotate;
  gdouble base_scale_factor;
  guint duration;
  const guchar pixdata[] = {255, 0, 0};
  guchar *internal_pixdata;

  /* Prepare PNG file */
  pixbuf = gdk_pixbuf_new_from_data (pixdata, GDK_COLORSPACE_RGB, FALSE, 8,
      1, 1, 1 * 3, NULL, NULL);
  g_assert_true (gdk_pixbuf_save (pixbuf, "/tmp/test-cheesemultifacesprite.png",
      "png", NULL, NULL));

  sprite = cheese_multiface_sprite_new_from_string (VALID_JSON_TEMPLATE,
      &error);
  g_assert_true (CHEESE_IS_MULTIFACE_SPRITE (sprite));
  g_assert_null (error);

  g_assert_cmpint (cheese_multiface_sprite_count_face_sprite (sprite), ==, 2);

  /* First face */
  sprite_face = cheese_multiface_sprite_get_face_sprite (sprite, 0);
  g_assert_cmpint (cheese_face_sprite_count_sprite_keypoint (sprite_face), ==,
      1);

  /* First face, first keypoint */
  sprite_keypoint =
      cheese_face_sprite_get_sprite_keypoint (sprite_face, 0);
  g_object_get (G_OBJECT (sprite_keypoint), "keypoint", &keypoint_type,
      "rotate", &rotate, "loop", &loop, "base-scale-factor", &base_scale_factor,
      NULL);
  g_assert (keypoint_type == CHEESE_FACE_KEYPOINT_EAR);
  g_assert_true (rotate);
  g_assert_false (loop);
  g_assert_cmpfloat (base_scale_factor, ==, 1.43);

  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (sprite_keypoint),
      ==, 2);

  /* First face, first keypoint, first frame */
  sprite_frame = cheese_face_sprite_keypoint_get_frame (sprite_keypoint, 0);
  g_object_get (G_OBJECT (sprite_frame), "duration", &duration, "pixbuf",
      &pixbuf_internal, "base-scale-factor", &base_scale_factor, NULL);
  g_assert_cmpint (duration, ==, 2);
  g_assert_cmpfloat (base_scale_factor, ==, 1.5);
  g_assert_true (GDK_IS_PIXBUF (pixbuf_internal));
  internal_pixdata = gdk_pixbuf_get_pixels (pixbuf_internal);
  g_assert_cmpmem (internal_pixdata,
      gdk_pixbuf_get_byte_length (pixbuf_internal), pixdata, sizeof (pixdata));

  /* First face, first keypoint, second frame */
  sprite_frame = cheese_face_sprite_keypoint_get_frame (sprite_keypoint, 1);
  g_object_get (G_OBJECT (sprite_frame), "duration", &duration, "pixbuf",
      &pixbuf_internal, "base-scale-factor", &base_scale_factor, NULL);
  g_assert_cmpint (duration, ==, 10);
  g_assert_cmpfloat (base_scale_factor, ==, 1.2);
  g_assert_true (GDK_IS_PIXBUF (pixbuf_internal));
  internal_pixdata = gdk_pixbuf_get_pixels (pixbuf_internal);
  g_assert_cmpmem (internal_pixdata,
      gdk_pixbuf_get_byte_length (pixbuf_internal), pixdata, sizeof (pixdata));


  /* Second face */
  sprite_face = cheese_multiface_sprite_get_face_sprite (sprite, 1);
  g_assert_cmpint (cheese_face_sprite_count_sprite_keypoint (sprite_face), ==,
      2);

  /* Second face, first keypoint */
  sprite_keypoint =
      cheese_face_sprite_get_sprite_keypoint (sprite_face, 0);
  g_object_get (G_OBJECT (sprite_keypoint), "keypoint", &keypoint_type,
      "rotate", &rotate, "loop", &loop, "base-scale-factor", &base_scale_factor,
      NULL);
  g_assert (keypoint_type == CHEESE_FACE_KEYPOINT_PHILTRUM);
  g_assert_false (rotate);
  g_assert_false (loop);
  g_assert_cmpfloat (base_scale_factor, ==, 1.0);

  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (sprite_keypoint),
      ==, 1);

  /* Second face, first keypoint, first frame */
  sprite_frame = cheese_face_sprite_keypoint_get_frame (sprite_keypoint, 0);
  g_object_get (G_OBJECT (sprite_frame), "duration", &duration, "pixbuf",
      &pixbuf_internal, "base-scale-factor", &base_scale_factor, NULL);
  g_assert_cmpint (duration, ==, 1);
  g_assert_cmpfloat (base_scale_factor, ==, 1.0);
  g_assert_true (GDK_IS_PIXBUF (pixbuf_internal));
  internal_pixdata = gdk_pixbuf_get_pixels (pixbuf_internal);
  g_assert_cmpmem (internal_pixdata,
      gdk_pixbuf_get_byte_length (pixbuf_internal), pixdata, sizeof (pixdata));

  /* Second face, second keypoint */
  sprite_keypoint =
      cheese_face_sprite_get_sprite_keypoint (sprite_face, 1);
  g_object_get (G_OBJECT (sprite_keypoint), "keypoint", &keypoint_type,
      "rotate", &rotate, "loop", &loop, "base-scale-factor", &base_scale_factor,
      NULL);
  g_assert (keypoint_type == CHEESE_FACE_KEYPOINT_MOUTH);
  g_assert_false (rotate);
  g_assert_false (loop);
  g_assert_cmpfloat (base_scale_factor, ==, 1.0);

  g_assert_cmpint (cheese_face_sprite_keypoint_count_frames (sprite_keypoint),
      ==, 1);

  /* Second face, second keypoint, first frame */
  sprite_frame = cheese_face_sprite_keypoint_get_frame (sprite_keypoint, 0);
  g_object_get (G_OBJECT (sprite_frame), "duration", &duration, "pixbuf",
      &pixbuf_internal, "base-scale-factor", &base_scale_factor, NULL);
  g_assert_cmpint (duration, ==, 1);
  g_assert_cmpfloat (base_scale_factor, ==, 1.0);
  g_assert_true (GDK_IS_PIXBUF (pixbuf_internal));
  internal_pixdata = gdk_pixbuf_get_pixels (pixbuf_internal);
  g_assert_cmpmem (internal_pixdata,
      gdk_pixbuf_get_byte_length (pixbuf_internal), pixdata, sizeof (pixdata));

  g_object_unref (pixbuf);
  g_object_unref (sprite);
}

static void
test_parse_from_invalid_string ()
{
  GError *error = NULL;
  CheeseMultifaceSprite *sprite;

  #define free_error(x)     { g_error_free (error); error = NULL; }
  sprite = cheese_multiface_sprite_new_from_string ("invalid", &error);
  g_assert_null (sprite);
  g_assert_nonnull (error);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string ("{}", &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string ("[]", &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string ("[{}]", &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string (
      INVALID_JSON_NO_FRAMES_MEMBER, &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string (INVALID_JSON_FRAMES_EMPTY,
      &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string (
      INVALID_JSON_FRAME_WITHOUT_LOCATION, &error);
  g_assert_null (sprite);
  g_assert_error (error, CHEESE_MULTIFACE_SPRITE_ERROR,
      CHEESE_MULTIFACE_SPRITE_ERROR_DESERIALIZE);
  free_error (error);

  sprite = cheese_multiface_sprite_new_from_string (
      INVALID_JSON_FRAME_WITH_INVALID_LOCATION, &error);
  g_assert_null (sprite);
  free_error (error);
  #undef test_free_error
}

static void
test_add_get_face_sprite ()
{
  CheeseMultifaceSprite *sprite;
  CheeseFaceSprite *face_sprite1, *face_sprite2, *face_sprite3;
  sprite = cheese_multiface_sprite_new ();

  face_sprite1 = cheese_face_sprite_new ();
  face_sprite2 = cheese_face_sprite_new ();
  face_sprite3 = cheese_face_sprite_new ();

  cheese_multiface_sprite_add_face_sprite (sprite, face_sprite1);
  g_assert_cmpint (cheese_multiface_sprite_count_face_sprite (sprite), ==, 1);
  cheese_multiface_sprite_add_face_sprite (sprite, face_sprite2);
  g_assert_cmpint (cheese_multiface_sprite_count_face_sprite (sprite), ==, 2);
  g_assert (cheese_multiface_sprite_get_face_sprite (sprite, 1) ==
      face_sprite2);
  g_assert (cheese_multiface_sprite_get_face_sprite (sprite, 0) ==
      face_sprite1);
  cheese_multiface_sprite_add_face_sprite (sprite, face_sprite3);
  g_assert_cmpint (cheese_multiface_sprite_count_face_sprite (sprite), ==, 3);
  g_assert (cheese_multiface_sprite_get_face_sprite (sprite, 2) ==
      face_sprite3);
  g_object_unref (sprite);
}

static void
test_new ()
{
  CheeseMultifaceSprite *sprite;
  sprite = cheese_multiface_sprite_new ();
  g_assert_true (CHEESE_IS_MULTIFACE_SPRITE (sprite));
  g_object_unref (sprite);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/cheese/multifacesprite/test_new", test_new);
  g_test_add_func ("/cheese/multifacesprite/test_add_get_face_sprite",
      test_add_get_face_sprite);
  g_test_add_func ("/cheese/multifacesprite/test_parse_from_invalid_string",
      test_parse_from_invalid_string);
  g_test_add_func ("/cheese/multifacesprite/test_parse_from_valid_string",
      test_parse_from_valid_string);
  return g_test_run ();
}
