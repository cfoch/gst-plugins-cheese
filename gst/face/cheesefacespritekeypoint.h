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
#ifndef __CHEESE_FACE_SPRITE_KEYPOINT_H__
#define __CHEESE_FACE_SPRITE_KEYPOINT_H__

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "cheesefacespriteframe.h"

G_BEGIN_DECLS

typedef enum
{
  CHEESE_FACE_KEYPOINT_PHILTRUM,
  CHEESE_FACE_KEYPOINT_MOUTH,
  CHEESE_FACE_KEYPOINT_EYE,
  CHEESE_FACE_KEYPOINT_NOSE,
  CHEESE_FACE_KEYPOINT_EAR,
  CHEESE_FACE_KEYPOINT_FACE,
  CHEESE_FACE_KEYPOINT_HEAD
} CheeseFaceKeypoint;

/*
 * Type declaration.
 */
#define CHEESE_TYPE_FACE_KEYPOINT (cheese_face_keypoint_get_type ())

#define CHEESE_TYPE_FACE_SPRITE_KEYPOINT cheese_face_sprite_keypoint_get_type ()
G_DECLARE_FINAL_TYPE (CheeseFaceSpriteKeypoint, cheese_face_sprite_keypoint,
    CHEESE, FACE_SPRITE_KEYPOINT, GObject);

/*
 * Method definitions.
 */
CheeseFaceSpriteKeypoint * cheese_face_sprite_keypoint_new (
    CheeseFaceKeypoint type);
CheeseFaceSpriteFrame * cheese_face_sprite_keypoint_get_frame (
    CheeseFaceSpriteKeypoint * self, const guint i);
guint cheese_face_sprite_keypoint_count_frames (
    CheeseFaceSpriteKeypoint * self);
void cheese_face_sprite_keypoint_add_frame (CheeseFaceSpriteKeypoint * self,
    CheeseFaceSpriteFrame * frame);

G_END_DECLS

#endif /* __CHEESE_FACE_SPRITE_KEYPOINT_H__ */
