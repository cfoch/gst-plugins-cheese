/*
 * GStreamer Plugins Cheese
 * Copyright (C) 2011 Jon Nordby <jononor@gmail.com>
 * Copyright (C) 2011 Laura Lucas Alday <lauralucas@gmail.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-faceoverlay
 *
 * Overlays a SVG image over a detected face in a video stream.
 * x, y, w, and h properties are optional, and change the image position and
 * size relative to the detected face position and size.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 autovideosrc ! videoconvert ! faceoverlay location=sprite.json ! videoconvert ! autovideosink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <string.h>

#include <gst/cheese/face/cheesemultifacemeta.h>
#include <gst/cheese/face/cheesemultifaceinfo.h>
#include <gst/cheese/face/cheesefaceinfo.h>
#include "gstcheesefaceoverlay.h"

static void gst_cheese_face_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_cheese_face_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_cheese_face_overlay_message_handler (GstBin * bin,
    GstMessage * message);
static void gst_cheese_face_overlay_draw_overlay (GstElement * overlay,
    cairo_t * cr, guint64 timestamp, guint64 duration, gpointer user_data);
static GstStateChangeReturn gst_cheese_face_overlay_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_cheese_face_overlay_create_children (GstCheeseFaceOverlay * filter);
static void gst_cheese_face_overlay_dispose (GObject * object);
static void gst_cheese_face_overlay_set_sprite_from_location (GstCheeseFaceOverlay * filter);
static void gst_cheese_face_overlay_set_sprite_from_data (GstCheeseFaceOverlay * filter);
static GstPadProbeReturn probe_overlay_buffer (GstPad * pad,
    GstPadProbeInfo * info, gpointer user_data);

GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_overlay_debug);
#define GST_CAT_DEFAULT gst_cheese_face_overlay_debug

typedef struct _FaceOverlayData FaceOverlayData;
typedef struct _FaceOverlayData {
  GstCheeseFaceInfo *face_info;
  CheeseFaceSprite *sprite;
  guint64 frame_counter;
};

static FaceOverlayData *
face_overlay_data_new (GstCheeseFaceInfo * face_info, CheeseFaceSprite * sprite,
    guint64 initial_frame)
{
  FaceOverlayData *ret;
  ret = g_new (FaceOverlayData, 1);
  ret->face_info = face_info;
  ret->sprite = sprite;
  ret->frame_counter = initial_frame;
  return ret;
}

static void
face_overlay_data_set_face_info (FaceOverlayData * self,
    GstCheeseFaceInfo * face_info)
{
  self->face_info = face_info;
}

static void
face_overlay_data_free (FaceOverlayData * self)
{
  g_free (self);
}

static void
face_overlay_data_get_image_position (FaceOverlayData * self,
    CheeseFaceKeypoint keypoint_type, guint face_id,
    graphene_point_t * keypoint, GdkPixbuf * pixbuf, graphene_point_t * img_pos)
{
  int img_width, img_height;
  img_width = gdk_pixbuf_get_width (pixbuf);
  img_height = gdk_pixbuf_get_height (pixbuf);

  switch (keypoint_type) {
    case CHEESE_FACE_KEYPOINT_PHILTRUM:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_MOUTH:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_LEFT_EYE:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_RIGHT_EYE:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_NOSE:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_LEFT_EAR:
      img_pos->x = keypoint->x - img_width;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_RIGHT_EAR:
      img_pos->x = keypoint->x;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_FACE:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height / 2;
      break;
    case CHEESE_FACE_KEYPOINT_HEAD:
      img_pos->x = keypoint->x - img_width / 2;
      img_pos->y = keypoint->y - img_height;
      break;
    default:
      GST_WARNING ("Face %d: No valid CheeseFaceKeypoint type found.", face_id);
  }
}

static gboolean
face_overlay_data_get_keypoint_pixinfo (FaceOverlayData * self,
    CheeseFaceKeypoint keypoint_type, guint face_id, graphene_point_t * pt)
{
  /* This is the relative position the image will be put at */
  /* For example, the relative position for the head is somewhere above the */
  /* front and the image should be rotated against this point */
  gboolean ret = TRUE;
  GArray *landmark;
  graphene_point_t *pt_ptr;
  *pt = GRAPHENE_POINT_INIT (0.0, 0.0);

  /* TODO Do not hard-code this */
  if (cheese_face_info_get_landmark_type (self->face_info) !=
      CHEESE_FACE_LANDMARK_TYPE_68) {
    GST_WARNING ("Face %d: a %d keypoint shape model info was not found.",
        face_id, CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_68));
    return FALSE;
  }

  /* Rotation is not supported yet */
  landmark = cheese_face_info_get_landmark_keypoints (self->face_info);

  switch (keypoint_type) {
    case CHEESE_FACE_KEYPOINT_PHILTRUM:
      /* Below tip of the nose */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 33);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Upper lip */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 51);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Average between part below tip of the nose and upper lip */
      pt->x /= 2;
      pt->y /= 2;
      break;
    case CHEESE_FACE_KEYPOINT_MOUTH:
      /* Upper inner lip */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 62);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Lower inner lip */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 66);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Average between upper and lower inner lip */
      pt->x /= 2;
      pt->y /= 2;
      break;
    case CHEESE_FACE_KEYPOINT_LEFT_EYE:
      /* Left part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 36);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Right part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 39);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Average */
      pt->x /= 2;
      pt->y /= 2;
      break;
    case CHEESE_FACE_KEYPOINT_RIGHT_EYE:
      /* Left part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 42);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Right part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 45);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Average */
      pt->x /= 2;
      pt->y /= 2;
      break;
    case CHEESE_FACE_KEYPOINT_NOSE:
      pt_ptr = &g_array_index (landmark, graphene_point_t, 30);
      *pt = *pt_ptr;
      break;
    case CHEESE_FACE_KEYPOINT_LEFT_EAR:
      /* How to calculate this? */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 1);
      *pt = *pt_ptr;
      break;
    case CHEESE_FACE_KEYPOINT_RIGHT_EAR:
      /* How to calculate this? */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 16);
      *pt = *pt_ptr;
      break;
    case CHEESE_FACE_KEYPOINT_FACE:
      /* How to calculate this? */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 30);
      *pt = *pt_ptr;
      break;
    case CHEESE_FACE_KEYPOINT_HEAD:
      /* Left part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 19);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Right part of the eye */
      pt_ptr = &g_array_index (landmark, graphene_point_t, 24);
      pt->x += pt_ptr->x;
      pt->y += pt_ptr->y;
      /* Average */
      pt->x /= 2;
      pt->y /= 2;
      break;
    default:
      GST_WARNING ("Face %d: No valid CheeseFaceKeypoint type found.", face_id);
      ret = FALSE;
  }
  return ret;
}

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_DATA
};

/* RGB16 is native-endianness in GStreamer */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define TEMPLATE_CAPS GST_VIDEO_CAPS_MAKE("{ BGRx, BGRA, RGB16 }")
#else
#define TEMPLATE_CAPS GST_VIDEO_CAPS_MAKE("{ xRGB, ARGB, RGB16 }")
#endif

static GstStaticPadTemplate src_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (TEMPLATE_CAPS)
    );

static GstStaticPadTemplate sink_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (TEMPLATE_CAPS)
    );

#define gst_cheese_face_overlay_parent_class parent_class
G_DEFINE_TYPE (GstCheeseFaceOverlay, gst_cheese_face_overlay, GST_TYPE_BIN);

static gboolean
gst_cheese_face_overlay_create_children (GstCheeseFaceOverlay * filter)
{
  GstElement *overlay;
  GstPad *sinkpad, *srcpad;

  overlay = gst_element_factory_make ("cairooverlay", NULL);

  g_signal_connect (overlay, "draw",
      G_CALLBACK (gst_cheese_face_overlay_draw_overlay), filter);

  gst_bin_add (GST_BIN (filter), overlay);
  filter->overlay = overlay;

  sinkpad = gst_element_get_static_pad (overlay, "sink");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (filter->sinkpad), sinkpad))
    GST_ERROR_OBJECT (filter->sinkpad, "couldn't set sinkpad target");

  srcpad = gst_element_get_static_pad (overlay, "src");
  if (!gst_ghost_pad_set_target (GST_GHOST_PAD (filter->srcpad), srcpad))
    GST_ERROR_OBJECT (filter->srcpad, "couldn't set srcpad target");
  gst_object_unref (srcpad);

  if (sinkpad) {
    filter->buffer_probe_id = gst_pad_add_probe (sinkpad,
        GST_PAD_PROBE_TYPE_BUFFER, probe_overlay_buffer, filter, NULL);
  }
  g_object_unref (sinkpad);

  return TRUE;

element_not_found:
  {
    if (overlay == NULL)
      GST_ERROR_OBJECT (filter, "cairooverlay element not found");
    else
      gst_object_unref (overlay);

    return FALSE;
  }
}

static GstStateChangeReturn
gst_cheese_face_overlay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      if (filter->overlay == NULL) {
        GST_ELEMENT_ERROR (filter, CORE, MISSING_PLUGIN, (NULL),
            ("Some required plugins are missing, probably either "
                "cheesefacedetect element or cairooverlay"));
        return GST_STATE_CHANGE_FAILURE;
      }
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    default:
      break;
  }

  return ret;
}

static CheeseFaceSprite *
gst_cheese_face_overlay_get_sprite_by_face_id (GstCheeseFaceOverlay * filter,
    guint face_id)
{
  CheeseFaceSprite *sprite;
  guint i, n_sprites;

  if (!filter->multiface_sprite)
    return NULL;

  n_sprites =
      cheese_multiface_sprite_count_face_sprite (filter->multiface_sprite);
  GST_DEBUG ("Finding sprite for face %d. There are %d sprites", face_id, n_sprites);
  i = (face_id - 1) % n_sprites;
  GST_DEBUG ("Face %d: got face sprite at index %d", face_id, i);
  sprite =
      cheese_multiface_sprite_get_face_sprite (filter->multiface_sprite, i);
  return sprite;
}

static GstPadProbeReturn
probe_overlay_buffer (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (user_data);
  /* Is there a better way to count frames? */
  filter->frame_counter++;

  if (GST_PAD_PROBE_INFO_TYPE (info) & GST_PAD_PROBE_TYPE_BUFFER) {
    GstBuffer *buf = gst_pad_probe_info_get_buffer (info);
    GstCheeseMultifaceMeta *meta = (GstCheeseMultifaceMeta *)
        gst_buffer_get_meta (buf, GST_CHEESE_MULTIFACE_META_API_TYPE);
    if (meta) {
      GstCheeseMultifaceInfoIter itr;
      GstCheeseFaceInfo *face_info;
      guint face_id;
      guint i;

      /* Remove faces */
      for (i = 0; i < meta->removed_faces->len; i++) {
        face_id =
            GPOINTER_TO_INT (g_array_index (meta->removed_faces, guint, i));
        GST_LOG ("Face %d: preparing to remove face.", face_id);
        if (!g_hash_table_lookup (filter->faces, GINT_TO_POINTER (face_id))) {
          GST_WARNING ("Face %d was not registered. There's nothing to remove."
              "There may be an error", face_id);
        }
        g_hash_table_remove (filter->faces, GINT_TO_POINTER (face_id));
        GST_LOG ("Face %d: face has been removed.", face_id);
      }

      /* Update faces info */
      GST_DEBUG ("There are %d faces to process.",
          gst_cheese_multiface_info_size (meta->faces));
      gst_cheese_multiface_info_iter_init (&itr, meta->faces);
      while (gst_cheese_multiface_info_iter_next (&itr, &face_id, &face_info)) {
        FaceOverlayData *face_overlay_data;
        GstCheeseFaceInfo *new_face_info;

        face_overlay_data = g_hash_table_lookup (filter->faces,
            GINT_TO_POINTER (face_id));
        if (!face_overlay_data) {
          CheeseFaceSprite *sprite;
          GST_LOG ("Face %d: inserting a new face.", face_id);
          /* Calculate which sprite to use */
          sprite = gst_cheese_face_overlay_get_sprite_by_face_id (filter,
              face_id);
          if (!sprite) {
            GST_WARNING ("Face %d: not possible to assign a sprite for this"
                "face.", face_id);
            continue;
          }
          /* Creating the face overlay data which holds an sprite */
          face_overlay_data = face_overlay_data_new (face_info, sprite,
              filter->frame_counter);
          g_hash_table_insert (filter->faces, GINT_TO_POINTER (face_id),
              face_overlay_data);
        }
        GST_LOG ("Face %d: updating face info.", face_id);
        /* It is easier to just replace the face than update its values */
        new_face_info = (GstCheeseFaceInfo *)
            gst_mini_object_copy (GST_MINI_OBJECT (face_info));
        face_overlay_data_set_face_info (face_overlay_data, new_face_info);
      }
    } else
      GST_LOG ("Not possible to obtain metadata of faces.");
  }
  return GST_PAD_PROBE_OK;
}

static void
gst_cheese_face_overlay_draw_overlay_from_pixbuf (GstElement * overlay,
    cairo_t * cr, GdkPixbuf * pixbuf, graphene_point_t image_pos,
    graphene_point_t keypoint_pos, gdouble rot_angle)
{
  if (rot_angle == 0.0) {
    gdk_cairo_set_source_pixbuf (cr, pixbuf, image_pos.x, image_pos.y);
    cairo_paint (cr);
  } else {
    int img_width, img_height;

    img_width = gdk_pixbuf_get_width (pixbuf);
    img_height = gdk_pixbuf_get_height (pixbuf);

    cairo_save (cr);
    cairo_translate (cr, image_pos.x + img_width / 2,
        image_pos.y + img_height / 2);
    cairo_rotate (cr, -rot_angle);
    cairo_translate (cr, -img_width / 2, -img_height / 2);
    gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
    cairo_paint (cr);
    cairo_restore (cr);
  }
}

static void
gst_cheese_face_overlay_draw_overlay (GstElement * overlay, cairo_t * cr,
    guint64 timestamp, guint64 duration, gpointer user_data)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (user_data);
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, filter->faces);
  /* For each face */
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    guint i, n_keypoints;
    const guint face_id = GPOINTER_TO_INT (key);
    FaceOverlayData *face_data = (FaceOverlayData *) value;
    CheeseFaceSprite *face_sprite = face_data->sprite;
    graphene_rect_t bounding_box;

    GST_DEBUG ("Draw face %d", face_id);

    if (!face_data->sprite) {
      GST_DEBUG ("Face %d: this face has not an assigned sprite."
          "Nothing to draw.", face_id);
      continue;
    }

    if (!cheese_face_info_get_display (face_data->face_info)) {
      GST_DEBUG ("Face %d: this face should not be displayed in this frame.",
          face_id);
      continue;
    }

    bounding_box = cheese_face_info_get_bounding_box (face_data->face_info);
    n_keypoints = cheese_face_sprite_count_sprite_keypoint (face_data->sprite);

    /* For each keypoint */
    for (i = 0; i < n_keypoints; i++) {
      guint j;
      CheeseFaceSpriteKeypoint *keypoint_sprite;
      CheeseFaceSpriteFrame *frame_sprite;
      CheeseFaceKeypoint keypoint_type;
      GdkPixbuf *pixbuf;
      gboolean draw;
      graphene_point_t keypoint_position, img_position;
      gdouble scale_factor;
      int img_real_width, img_real_height;
      GdkPixbuf *scaled_pixbuf;
      gboolean rotate;
      gdouble rot_angle = 0.0;

      /* Get the frame sprite to this current frame for this face. */
      keypoint_sprite = cheese_face_sprite_get_sprite_keypoint (face_sprite, i);
      frame_sprite = cheese_face_sprite_keypoint_get_frame_at (keypoint_sprite,
          filter->frame_counter);
      g_object_get (G_OBJECT (keypoint_sprite), "keypoint", &keypoint_type,
          "rotate", &rotate, NULL);

      /* Obtain its pixbuf */
      g_object_get (G_OBJECT (frame_sprite), "pixbuf", &pixbuf,
          "base-scale-factor", &scale_factor, NULL);
      img_real_width = gdk_pixbuf_get_width (pixbuf);
      img_real_height = gdk_pixbuf_get_height (pixbuf);

      scale_factor *= graphene_rect_get_width (&bounding_box) / img_real_width;
      draw = face_overlay_data_get_keypoint_pixinfo (face_data,
          keypoint_type, face_id, &keypoint_position);
      if (!draw)
        continue;

      scaled_pixbuf = gdk_pixbuf_scale_simple (pixbuf,
          img_real_width * scale_factor, img_real_height * scale_factor,
          GDK_INTERP_NEAREST);

      face_overlay_data_get_image_position (face_data, keypoint_type, face_id,
          &keypoint_position, scaled_pixbuf, &img_position);
      if (rotate)
        cheese_face_info_get_eye_rotation (face_data->face_info, &rot_angle);
      gst_cheese_face_overlay_draw_overlay_from_pixbuf (overlay, cr,
          scaled_pixbuf, img_position, keypoint_position, rot_angle);
      g_object_unref (scaled_pixbuf);
    }
    face_data->frame_counter++;
  }
}

static void
gst_cheese_face_overlay_class_init (GstCheeseFaceOverlayClass * klass)
{
  GObjectClass *gobject_class;
  GstBinClass *gstbin_class;
  GstElementClass *gstelement_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbin_class = GST_BIN_CLASS (klass);
  gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_cheese_face_overlay_debug,
      "gstcheesefaceoverlay", 0, "Cheese Face Overlay");


  gobject_class->set_property = gst_cheese_face_overlay_set_property;
  gobject_class->get_property = gst_cheese_face_overlay_get_property;
  gobject_class->dispose = gst_cheese_face_overlay_dispose;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location of SVG file to use for face overlay",
          "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_DATA,
      g_param_spec_string ("data", "Data",
          "Raw JSON data representing a CheeseMultifaceSprite",
          "", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gst_element_class_set_static_metadata (gstelement_class,
      "cheesefaceoverlay",
      "Filter/Editor/Video",
      "Overlays graphics over a detected face in a video stream",
      "Fabian Orccon <cfoch.fabian@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));

  gstelement_class->change_state =
      GST_DEBUG_FUNCPTR (gst_cheese_face_overlay_change_state);
}

static void
gst_cheese_face_overlay_init (GstCheeseFaceOverlay * filter)
{
  GstPadTemplate *tmpl;

  filter->frame_counter = 0;

  filter->buffer_probe_id = 0;
  filter->overlay = NULL;
  filter->location = NULL;
  filter->data = NULL;

  filter->multiface_sprite = NULL;

  filter->faces = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL,
      (GDestroyNotify) face_overlay_data_free);

  tmpl = gst_static_pad_template_get (&sink_factory);
  filter->sinkpad = gst_ghost_pad_new_no_target_from_template ("sink", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  tmpl = gst_static_pad_template_get (&src_factory);
  filter->srcpad = gst_ghost_pad_new_no_target_from_template ("src", tmpl);
  gst_object_unref (tmpl);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  gst_cheese_face_overlay_create_children (filter);
}

static void
gst_cheese_face_overlay_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (filter);
      if (filter->location)
        g_free (filter->location);
      filter->location = g_value_dup_string (value);
      gst_cheese_face_overlay_set_sprite_from_location (filter);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_DATA:
      GST_OBJECT_LOCK (filter);
      if (filter->data)
        g_free (filter->data);
      filter->data = g_value_dup_string (value);
      gst_cheese_face_overlay_set_sprite_from_data (filter);
      GST_OBJECT_UNLOCK (filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_overlay_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);

  switch (prop_id) {
    case PROP_LOCATION:
      GST_OBJECT_LOCK (filter);
      g_value_set_string (value, filter->location);
      GST_OBJECT_UNLOCK (filter);
      break;
    case PROP_DATA:
      GST_OBJECT_LOCK (filter);
      g_value_set_string (value, filter->data);
      GST_OBJECT_UNLOCK (filter);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_overlay_dispose (GObject * object)
{
  GstCheeseFaceOverlay *filter = GST_CHEESEFACEOVERLAY (object);


  if (filter->location)
    g_free (filter->location);
  if (filter->multiface_sprite)
    g_object_unref (filter->multiface_sprite);
  if (filter->buffer_probe_id != 0) {
    GstPad *sinkpad;
    sinkpad = gst_element_get_static_pad (filter->overlay, "sink");
    if (sinkpad)
      gst_pad_remove_probe (sinkpad, filter->buffer_probe_id);
    g_object_unref (sinkpad);
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gst_cheese_face_overlay_set_sprite_from_location (GstCheeseFaceOverlay * filter)
{
  GError *error = NULL;

  if (filter->multiface_sprite) {
    g_object_unref (filter->multiface_sprite);
    filter->multiface_sprite = NULL;
  }

  if (!filter->location) {
    GST_WARNING_OBJECT (filter, "Location is not set. Not possible to create"
        "sprite.");
    return;
  }

  filter->multiface_sprite =
      cheese_multiface_sprite_new_from_location (filter->location, &error);
  if (!filter->multiface_sprite)
    GST_ERROR (error->message);
}

static void
gst_cheese_face_overlay_set_sprite_from_data (GstCheeseFaceOverlay * filter)
{
  GError *error = NULL;

  if (filter->multiface_sprite) {
    g_object_unref (filter->multiface_sprite);
    filter->multiface_sprite = NULL;
  }

  if (!filter->data) {
    GST_WARNING_OBJECT (filter, "Data property is not set. Not possible to "
        "create sprite.");
    return;
  }

  filter->multiface_sprite =
      cheese_multiface_sprite_new_from_string (filter->data, &error);
  if (!filter->multiface_sprite)
    GST_ERROR (error->message);
}
