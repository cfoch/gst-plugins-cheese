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
#include <gst/cheese/face/cheesefacesprite.h>
#include <gst/cheese/face/cheesefacespritekeypoint.h>

static void
test_insert_get_sprite_keypoint ()
{
  CheeseFaceSprite *sprite;
  CheeseFaceSpriteKeypoint *eye_sprite, *head_sprite, *mouth_sprite;
  CheeseFaceSpriteKeypoint *out_keypoint_sprite;

  eye_sprite = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_RIGHT_EYE);
  head_sprite = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_HEAD);
  mouth_sprite = cheese_face_sprite_keypoint_new (CHEESE_FACE_KEYPOINT_MOUTH);

  sprite = cheese_face_sprite_new ();
  cheese_face_sprite_add_sprite_keypoint (sprite, head_sprite);
  g_assert_cmpint (cheese_face_sprite_count_sprite_keypoint (sprite), ==, 1);
  cheese_face_sprite_add_sprite_keypoint (sprite, eye_sprite);
  g_assert_cmpint (cheese_face_sprite_count_sprite_keypoint (sprite), ==, 2);
  cheese_face_sprite_add_sprite_keypoint (sprite, mouth_sprite);
  g_assert_cmpint (cheese_face_sprite_count_sprite_keypoint (sprite), ==, 3);

  out_keypoint_sprite = cheese_face_sprite_get_sprite_keypoint (sprite, 3);
  g_assert_null (out_keypoint_sprite);
  out_keypoint_sprite = cheese_face_sprite_get_sprite_keypoint (sprite, 0);
  g_assert (out_keypoint_sprite = head_sprite);
  out_keypoint_sprite = cheese_face_sprite_get_sprite_keypoint (sprite, 2);
  g_assert (out_keypoint_sprite = mouth_sprite);
  out_keypoint_sprite = cheese_face_sprite_get_sprite_keypoint (sprite, 1);
  g_assert (out_keypoint_sprite = eye_sprite);

  g_object_unref (sprite);
}

static void
test_new ()
{
  CheeseFaceSprite *sprite;
  sprite = cheese_face_sprite_new ();
  g_assert_true (CHEESE_IS_FACE_SPRITE (sprite));
  g_object_unref (sprite);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/cheese/facesprite/test_new", test_new);
  g_test_add_func ("/cheese/facesprite/test_insert_get_sprite_keypoint",
      test_insert_get_sprite_keypoint);
  return g_test_run ();
}
