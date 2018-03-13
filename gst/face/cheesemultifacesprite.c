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

#include "cheesemultifacesprite.h"
#include "cheesefacesprite.h"

struct _CheeseMultifaceSprite {
  GObject parent_instance;
  GPtrArray *faces;
};

static void cheese_multiface_sprite_class_init (CheeseMultifaceSpriteClass *
    klass);
static void cheese_multiface_sprite_init (CheeseMultifaceSprite * self);
static void cheese_multiface_sprite_finalize (GObject * object);
cheese_multiface_sprite_add_face_sprite (CheeseMultifaceSprite * self,
    CheeseFaceSprite * face_sprite);
guint cheese_multiface_sprite_count_face_sprite (
    CheeseMultifaceSprite * self)
CheeseFaceSprite * cheese_multiface_sprite_get_face_sprite (
    CheeseMultifaceSprite * self, const guint i)
CheeseMultifaceSprite * cheese_multiface_sprite_new ();


G_DEFINE_TYPE (CheeseMultifaceSprite, cheese_multiface_sprite, G_TYPE_OBJECT)
static void
cheese_multiface_sprite_class_init (CheeseMultifaceSpriteClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = cheese_multiface_sprite_finalize;
}

static void
cheese_multiface_sprite_init (CheeseMultifaceSprite * self)
{
  self->faces =
      g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);
}

static void
cheese_multiface_sprite_finalize (GObject * object)
{
  CheeseMultifaceSprite *self = (CheeseMultifaceSprite *) object;
  g_ptr_array_free (self->faces, TRUE);
  G_OBJECT_CLASS (cheese_multiface_sprite_parent_class)->finalize (object);
}

void
cheese_multiface_sprite_add_face_sprite (CheeseMultifaceSprite * self,
    CheeseFaceSprite * face_sprite)
{
  g_ptr_array_add (self->faces, (gpointer) face_sprite);
}

guint
cheese_multiface_sprite_count_face_sprite (CheeseMultifaceSprite * self)
{
  return self->faces->len;
}

CheeseFaceSprite *
cheese_multiface_sprite_get_face_sprite (CheeseMultifaceSprite * self,
    const guint i)
{
  if (i >= self->faces->len)
    return NULL;
  return g_ptr_array_index (self->faces, i);
}

CheeseMultifaceSprite *
cheese_multiface_sprite_new ()
{
  return g_object_new (CHEESE_TYPE_MULTIFACE_SPRITE, NULL);
}
