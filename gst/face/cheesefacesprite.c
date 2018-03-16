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

#include "cheesefacesprite.h"

struct _CheeseFaceSprite {
  GObject parent_instance;
  GPtrArray *keypoints;
};

static void cheese_face_sprite_class_init (CheeseFaceSpriteClass * klass);
static void cheese_face_sprite_init (CheeseFaceSprite * self);
static void cheese_face_sprite_finalize (GObject * object);

CheeseFaceSpriteKeypoint * cheese_face_sprite_get_sprite_keypoint (
    CheeseFaceSprite *self, const guint i);
guint cheese_face_sprite_count_sprite_keypoint (CheeseFaceSprite *self);
void cheese_face_sprite_add_sprite_keypoint (CheeseFaceSprite *self,
    CheeseFaceSpriteKeypoint * sprite_keypoint);
CheeseFaceSprite * cheese_face_sprite_new ();


G_DEFINE_TYPE (CheeseFaceSprite, cheese_face_sprite, G_TYPE_OBJECT)
static void
cheese_face_sprite_class_init (CheeseFaceSpriteClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_face_sprite_finalize;
}

static void
cheese_face_sprite_init (CheeseFaceSprite * self)
{
  self->keypoints = g_ptr_array_new_with_free_func (
      (GDestroyNotify) g_object_unref);
}

static void
cheese_face_sprite_finalize (GObject * object)
{
  CheeseFaceSprite *self = (CheeseFaceSprite *) object;
  g_ptr_array_free (self->keypoints, TRUE);
  G_OBJECT_CLASS (cheese_face_sprite_parent_class)->finalize (object);
}

CheeseFaceSpriteKeypoint *
cheese_face_sprite_get_sprite_keypoint (CheeseFaceSprite *self, const guint i)
{
  if (i >= self->keypoints->len)
    return NULL;
  return (CheeseFaceSpriteKeypoint *) g_ptr_array_index (self->keypoints, i);
}

guint
cheese_face_sprite_count_sprite_keypoint (CheeseFaceSprite *self)
{
  return self->keypoints->len;
}

void
cheese_face_sprite_add_sprite_keypoint (CheeseFaceSprite *self,
    CheeseFaceSpriteKeypoint * sprite_keypoint)
{
  g_ptr_array_add (self->keypoints, (gpointer) sprite_keypoint);
}

CheeseFaceSprite *
cheese_face_sprite_new ()
{
  return g_object_new (CHEESE_TYPE_FACE_SPRITE, NULL);
}
