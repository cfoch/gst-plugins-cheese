/*
 * GStreamer Plugins Cheese
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2008 Michael Sheldon <mike@mikeasoft.com>
 * Copyright (C) 2011 Stefan Sauer <ensonic@users.sf.net>
 * Copyright (C) 2014 Robert Jobbagy <jobbagy.robert@gmail.com>
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

/**
 * SECTION:element-cheesefaceomelette
 *
 * FIXME:Describe cheesefaceomelette here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 v4l2src ! videoconvert ! \
 *   cheesefaceomelette scale-factor=0.5 display-landmark=false \
 *   display-pose-estimation=false display-bounding-box=false display-id=false \
 *   landmark=../shape_predictor_68_face_landmarks.dat display-landmark=false \
 *   hungarian-delete-threshold=75 ! \
 *   videoconvert ! xvimagesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <cstdio>
#include <iostream>
#include <vector>

#include "gstcheesefaceomelette.h"

using namespace std;
using namespace cv;

enum OmeletteAnimationTurn {
  OMELETTE_ANIMATION_TURN_OMELETTE,
  OMELETTE_ANIMATION_TURN_OMELETTE_OREGANO,
  OMELETTE_ANIMATION_TURN_TOMATO,
  OMELETTE_ANIMATION_TURN_CHEESE
};

typedef struct _OmeletteData OmeletteData;
typedef struct _OmeletteData {
  guint face_id;
  OmeletteAnimationTurn turn;
  /* Counts frames when animation runs. If face not detected, skip. */
  guint64 animation_frame_counter;
  /* Counts the number of cheese added to the omelette. */
  guint cheese_counter;
  /* Counts the number of tomatoes added to the omelette. */
  guint tomato_counter;
  /* Just in case */
  GstCheeseFaceOmelette *filter;
  
};

static cv::Scalar WHITE(255, 255, 255);
static cv::Scalar BLACK(0, 0, 0);

static guint FACE_NOSE_POINT = 34;
static guint FACE_LIPS_INTERNAL_BORDER[] =
    {49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60};
static guint FACE_LIPS_EXTERNAL_BORDER[] = {61, 62, 63, 64, 65, 66, 67, 68};
static guint FACE_LEFT_EYE_BORDER[] = {37, 38, 39, 40, 41, 42};
static guint FACE_RIGHT_EYE_BORDER[] = {43, 44, 45, 46, 47, 48};

#define OMELETTE_ASSETS_DIR       GST_PLUGINS_CHEESE_DATADIR "/pixmaps/gst/cheese/face/omelette/"
#define DEFAULT_FIT_FACTOR                                   1.5
/* FIXME: If intervals are 1 there is a segmentation fault. */
#define DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL            120
#define DEFAULT_ANIMATION_OMELETTE_OREGANO_FRAME_INTERVAL    30
#define DEFAULT_ANIMATION_TOMATO_FRAME_INTERVAL              2
#define DEFAULT_ANIMATION_CHEESE_FRAME_INTERVAL              2

GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_omelette_debug);
#define GST_CAT_DEFAULT gst_cheese_face_omelette_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB"))
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("RGB"))
    );

#define gst_cheese_face_omelette_parent_class parent_class
G_DEFINE_TYPE (GstCheeseFaceOmelette, gst_cheese_face_omelette,
               GST_TYPE_CHEESEFACEDETECT);

static void gst_cheese_face_omelette_finalize (GObject * obj);
static void gst_cheese_face_omelette_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_cheese_face_omelette_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_cheese_face_omelette_transform_ip (
    GstOpencvVideoFilter * filter, GstBuffer * buf, IplImage * img);

/* Omelette per face data */
static OmeletteData * omelette_data_new (GstCheeseFaceOmelette * filter);
static void omelette_data_free (gpointer user_data);

static OmeletteData *
omelette_data_new (guint face_id, GstCheeseFaceOmelette * filter)
{
  OmeletteData * data = new OmeletteData;
  GST_DEBUG ("Face %d: Create new omelette data.", face_id);
  data->face_id = face_id;
  data->turn = OMELETTE_ANIMATION_TURN_OMELETTE;
  data->animation_frame_counter = 0;
  data->cheese_counter = 0;
  data->tomato_counter = 0;
  data->filter = filter;
  return data;
}

static void
omelette_data_free (gpointer user_data)
{
  OmeletteData *data = (OmeletteData *) user_data;
  delete data;
}

static void
omelette_data_update_turn (OmeletteData * data)
{
  const guint64 frame_counter = data->animation_frame_counter;
  const guint max_tomatoes = G_N_ELEMENTS (data->filter->tomatoes);
  const guint max_cheeses = G_N_ELEMENTS (data->filter->cheeses);

  if (frame_counter == 1 + DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL) {
    data->turn = OMELETTE_ANIMATION_TURN_OMELETTE_OREGANO;
    GST_LOG ("Face %d: Turn changed to %s", data->face_id,
        G_STRINGIFY (OMELETTE_ANIMATION_TURN_OMELETTE_OREGANO));
  } else if (frame_counter == 1 + DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL +
      DEFAULT_ANIMATION_OMELETTE_OREGANO_FRAME_INTERVAL) {
    data->turn = OMELETTE_ANIMATION_TURN_TOMATO;
    GST_LOG ("Face %d: Turn changed to %s", data->face_id,
        G_STRINGIFY (OMELETTE_ANIMATION_TURN_TOMATO));
  } else if (frame_counter == 1 + DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL +
      DEFAULT_ANIMATION_OMELETTE_OREGANO_FRAME_INTERVAL +
      DEFAULT_ANIMATION_TOMATO_FRAME_INTERVAL * max_tomatoes) {
    data->turn = OMELETTE_ANIMATION_TURN_CHEESE;
    GST_LOG ("Face %d: Turn changed to %s", data->face_id,
        G_STRINGIFY (OMELETTE_ANIMATION_TURN_CHEESE));
  }

  if (data->turn == OMELETTE_ANIMATION_TURN_TOMATO) {
    const guint64 delta = frame_counter -
        DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL -
        DEFAULT_ANIMATION_OMELETTE_OREGANO_FRAME_INTERVAL -
        DEFAULT_ANIMATION_TOMATO_FRAME_INTERVAL * data->tomato_counter;

    if (delta % DEFAULT_ANIMATION_TOMATO_FRAME_INTERVAL == 1 &&
        data->tomato_counter < max_tomatoes) {
      data->tomato_counter++;
      GST_LOG ("Face %d: Tomato counter set to %d", data->face_id,
          data->tomato_counter);
    }
  } else if (data->turn == OMELETTE_ANIMATION_TURN_CHEESE) {
    const guint64 delta = frame_counter -
        DEFAULT_ANIMATION_OMELETTE_FRAME_INTERVAL -
        DEFAULT_ANIMATION_OMELETTE_OREGANO_FRAME_INTERVAL -
        DEFAULT_ANIMATION_TOMATO_FRAME_INTERVAL * data->tomato_counter -
        DEFAULT_ANIMATION_CHEESE_FRAME_INTERVAL * data->cheese_counter;

    if (delta % DEFAULT_ANIMATION_CHEESE_FRAME_INTERVAL == 1 &&
        data->cheese_counter < max_cheeses) {
      data->cheese_counter++;
      GST_LOG ("Face %d: Cheese counter set to %d", data->face_id,
          data->cheese_counter);
    }
  }
}

/* initialize the cheesefaceomelette's class */
static void
gst_cheese_face_omelette_class_init (GstCheeseFaceOmeletteClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstOpencvVideoFilterClass *gstopencvbasefilter_class;
  GstCheeseFaceDetectClass *gstcheesefacedetect_class;

  GST_DEBUG_CATEGORY_INIT (gst_cheese_face_omelette_debug,
      "gstcheesefaceomelette", 0, "Cheese Face Detect");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstopencvbasefilter_class = (GstOpencvVideoFilterClass *) klass;
  gstcheesefacedetect_class = (GstCheeseFaceDetectClass *) klass;

  gstopencvbasefilter_class->cv_trans_ip_func =
      gst_cheese_face_omelette_transform_ip;
  gstcheesefacedetect_class->cheese_face_free_user_data_func =
      omelette_data_free;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_cheese_face_omelette_finalize);
  gobject_class->set_property = gst_cheese_face_omelette_set_property;
  gobject_class->get_property = gst_cheese_face_omelette_get_property;

  gst_element_class_set_details_simple(gstelement_class,
    "CheeseFaceOmelette",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabian Orccon <<cfoch.fabian@gmail.com>>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_cheese_face_omelette_init (GstCheeseFaceOmelette * filter)
{
  cv::Mat img;
  guint i;

  filter->omelette = NULL;
  filter->omelette_oregano = NULL;
  /* TODO: Use memcpy? */
  for (i = 0; i < G_N_ELEMENTS (filter->cheeses); i++)
    filter->cheeses[i] = NULL;
  for (i = 0; i < G_N_ELEMENTS (filter->tomatoes); i++)
    filter->tomatoes[i] = NULL;

  /* Load overlay images */
  GST_LOG ("Loading image file '%s'.", OMELETTE_ASSETS_DIR "omelette.png");
  filter->omelette = new cv::UMat;
  img = cv::imread (OMELETTE_ASSETS_DIR "omelette.png",
      CV_LOAD_IMAGE_UNCHANGED);
  if (!img.data)
    goto load_resources_error;
  *filter->omelette = img.getUMat(cv::ACCESS_READ);

  GST_LOG ("Loading image file '%s'.",
      OMELETTE_ASSETS_DIR "omelette-oregano.png");
  img = cv::imread (OMELETTE_ASSETS_DIR "omelette-oregano.png",
      CV_LOAD_IMAGE_UNCHANGED);
  filter->omelette_oregano = new cv::UMat;
  if (!img.data)
    goto load_resources_error;
  *filter->omelette_oregano = img.getUMat(cv::ACCESS_READ);

  for (i = 0; i < G_N_ELEMENTS (filter->cheeses); i++) {
    gchar *filename;
    filter->cheeses[i] = new cv::UMat;
    filename = g_strdup_printf (OMELETTE_ASSETS_DIR "cheese-%d.png", i + 1);
    GST_LOG ("Loading image file '%s'.", filename);
    img = cv::imread (filename, CV_LOAD_IMAGE_UNCHANGED);
    g_free (filename);
    if (!img.data)
      goto load_resources_error;
    *filter->cheeses[i] = img.getUMat(cv::ACCESS_READ);
  }

  for (i = 0; i < G_N_ELEMENTS (filter->tomatoes); i++) {
    gchar *filename;
    filter->tomatoes[i] = new cv::UMat;
    filename = g_strdup_printf (OMELETTE_ASSETS_DIR "tomato-%d.png", i + 1);
    GST_LOG ("Loading image file '%s'.", filename);
    img = cv::imread (filename, CV_LOAD_IMAGE_UNCHANGED);
    g_free (filename);
    if (!img.data)
      goto load_resources_error;
    *filter->tomatoes[i] = img.getUMat(cv::ACCESS_READ);
  }
  filter->resources_loaded = TRUE;
  return;

load_resources_error:
  GST_ERROR ("Unable to load the image file.");
  filter->resources_loaded = FALSE;
  return;
}

static void
gst_cheese_face_omelette_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (object);
  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_omelette_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_omelette_select_image (GstCheeseFaceOmelette * filter,
    OmeletteData * omelette_data, cv::UMat & image, cv::UMat & image_mask,
    cv::Size & image_size, float scale_factor, float fit_factor)
{
  scale_factor *= fit_factor;
  /* Decide what image to show */
  switch (omelette_data->turn) {
    case OMELETTE_ANIMATION_TURN_OMELETTE:
      cv::resize (*filter->omelette, image, cv::Size (), scale_factor,
          scale_factor);
      break;
    case OMELETTE_ANIMATION_TURN_OMELETTE_OREGANO:
      cv::resize (*filter->omelette_oregano, image, cv::Size (),
          scale_factor, scale_factor);
      break;
    case OMELETTE_ANIMATION_TURN_TOMATO:
      cv::resize (*filter->tomatoes[omelette_data->tomato_counter - 1],
          image, cv::Size (), scale_factor, scale_factor);
      break;
    case OMELETTE_ANIMATION_TURN_CHEESE:
      cv::resize (*filter->cheeses[omelette_data->cheese_counter - 1],
          image, cv::Size (), scale_factor, scale_factor);
      break;
  }
  image_size = image.size ();
  /* TODO */
  /* It is obvious that images have alpha channel but validate anyway */
  cv::extractChannel (image, image_mask, 3);
  cv::cvtColor(image_mask, image_mask, CV_GRAY2RGB);
  cv::cvtColor(image, image, CV_BGR2RGB);
}

static void
_cheese_face_create_face_polygons (CheeseFace & face,
    std::vector<cv::Point> & face_lips_internal_pts,
    std::vector<cv::Point> & face_left_eye_internal_pts,
    std::vector<cv::Point> & face_right_eye_internal_pts)
{
  guint i;
  for (i = 0; i < G_N_ELEMENTS (FACE_LIPS_INTERNAL_BORDER); i++)
    face_lips_internal_pts.push_back (
        face.landmark[FACE_LIPS_INTERNAL_BORDER[i] - 1]);
  for (i = 0; i < G_N_ELEMENTS (FACE_LEFT_EYE_BORDER); i++)
    face_left_eye_internal_pts.push_back (
        face.landmark[FACE_LEFT_EYE_BORDER[i] - 1]);
  for (i = 0; i < G_N_ELEMENTS (FACE_RIGHT_EYE_BORDER); i++)
    face_right_eye_internal_pts.push_back (
        face.landmark[FACE_RIGHT_EYE_BORDER[i] - 1]);
}

static void
_draw_polygons_to_mask (cv::UMat & umask,
    std::vector<cv::Point> & face_lips_internal_pts,
    std::vector<cv::Point> & face_left_eye_internal_pts,
    std::vector<cv::Point> & face_right_eye_internal_pts)
{
  /* Do I really have to convert it to a cv::Mat ? */
  cv::Mat mask = umask.getMat (cv::ACCESS_WRITE);
  cv::fillConvexPoly (mask, &face_lips_internal_pts[0],
      face_lips_internal_pts.size (), BLACK);
  cv::fillConvexPoly (mask, &face_left_eye_internal_pts[0],
      face_left_eye_internal_pts.size (), BLACK);
  cv::fillConvexPoly (mask, &face_right_eye_internal_pts[0],
      face_right_eye_internal_pts.size (), BLACK);
  umask = mask.getUMat (cv::ACCESS_WRITE);
}

static GstFlowReturn
gst_cheese_face_omelette_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  guint i, j;
  const gboolean debug = gst_debug_is_active ();
  GstFlowReturn ret = GST_FLOW_ERROR;
  GstOpencvVideoFilterClass *bclass =
    GST_OPENCV_VIDEO_FILTER_CLASS (gst_cheese_face_omelette_parent_class);
  GstCheeseFaceDetect *parent_filter = GST_CHEESEFACEDETECT (base);
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (base);

  if (filter->resources_loaded && parent_filter->shape_predictor)
    ret = bclass->cv_trans_ip_func (base, buf, img);

  if (ret == GST_FLOW_OK) {
    cv::Mat cvImg (cv::cvarrToMat (img));
    cv::Size sz = cvImg.size ();
    cv::Rect rect (cv::Point (0, 0), sz);
    /* Dumbness */
    gint64 time_start;
    gint64 time_animation_counter = 0, time_copy_frame = 0, time_polygons = 0;
    gint64 time_select_image = 0, time_draw_polygons = 0;
    gint64 time_create_black_frame = 0;
    gint64 time_per_face_start = 0, time_per_face = 0;

    if (debug)
      time_start = cv::getTickCount ();
    cv::UMat mask(sz.height, sz.width, CV_8UC3, BLACK);
    /* The image that will be masked */
    cv::UMat mask_victim (sz.height, sz.width, CV_8UC3, BLACK);
    /* The mask_victim with mask applied? */
    cv::UMat masked (sz.height, sz.width, CV_8UC3, BLACK);
    //uImg = cvImg.getUMat (cv::ACCESS_WRITE);
    if (debug)
      time_create_black_frame = cv::getTickCount () - time_start;


    for (auto &kv : *parent_filter->faces) {
      guint id = kv.first;
      CheeseFace &face = kv.second;
      const gboolean animate =
          face.last_detected_frame == parent_filter->frame_number - 1;
      OmeletteData *omelette_data;

      if (face.landmark.size () != 68) {
        GST_DEBUG ("Face %d: Landmark is not a 68-facial keypoints model.", id);
        continue;
      }

      /**
       * FIXME
       * If two or more detected faces are very close then the bounding box
       * of one of the images clouds the other image. What should be done
       * is to blend all the overlay images, or at least that is what I think.
       * This isn't critical but it gives a bad effect. Also, if one image is
       * should be overlayed out of the bounds of the frames it won't be drawn.
       */
      if (debug)
        time_per_face_start = cv::getTickCount ();;
      if (animate) {
        if (debug)
          time_start = cv::getTickCount ();
        if (!face.user_data)
          face.user_data = (gpointer) omelette_data_new (id, filter);
        omelette_data = (OmeletteData *) (face.user_data);
        omelette_data->animation_frame_counter += 1;
        GST_DEBUG ("Face %d: Animation frame counter is %d", id,
            omelette_data->animation_frame_counter);
        omelette_data_update_turn (omelette_data);
        if (debug)
          time_animation_counter += cv::getTickCount () - time_start;
      }

      /* The parent already incremented the frame counter */
      if (animate) {
        const float scale_factor =
            face.bounding_box.width () / (gfloat) filter->omelette->cols;
        /* Images */
        cv::UMat image, image_mask;
        cv::Size image_size;
        /* Points to build polygons */
        std::vector<cv::Point> face_lips_internal_pts;
        std::vector<cv::Point> face_left_eye_internal_pts;
        std::vector<cv::Point> face_right_eye_internal_pts;
        /* ROIs */
        cv::Rect mask_victim_rect_ROI;
        cv::UMat mask_victim_ROI;
        cv::UMat mask_ROI;
        cv::Point nose_point = face.landmark[FACE_NOSE_POINT - 1];
        cv::Point image_offset;

        if (debug)
          time_start = cv::getTickCount ();
        _cheese_face_create_face_polygons (face, face_lips_internal_pts,
            face_left_eye_internal_pts, face_right_eye_internal_pts);
        if (debug)
          time_polygons += cv::getTickCount () - time_start;

        if (debug)
          time_start = cv::getTickCount ();
        gst_cheese_face_omelette_select_image (filter, omelette_data, image,
            image_mask, image_size, scale_factor, DEFAULT_FIT_FACTOR);
        if (debug)
          time_select_image += cv::getTickCount () - time_start;

        image_offset = cv::Point (
            nose_point.x - image_size.width / 2,
            nose_point.y - image_size.height / 2
        );
        mask_victim_rect_ROI = cv::Rect (image_offset, image_size) & rect;
        mask_victim_ROI = mask_victim (mask_victim_rect_ROI);
        mask_ROI = mask (mask_victim_rect_ROI);

        image.copyTo (mask_victim_ROI);
        image_mask.copyTo (mask_ROI);

        if (debug)
          time_start = cv::getTickCount ();
        _draw_polygons_to_mask (mask, face_lips_internal_pts,
            face_left_eye_internal_pts, face_right_eye_internal_pts);
        if (debug)
          time_draw_polygons += cv::getTickCount () - time_start;
      }

      if (debug)
        time_per_face = cv::getTickCount () - time_per_face_start;
      GST_DEBUG ("Face %d: time to prepare images: %.2f ms.",
          ((double) time_per_face * 1000) / cv::getTickFrequency ());
    }

    if (parent_filter->faces->size () > 0) {
      /* Dumbness */
      if (debug)
        time_start = cv::getTickCount ();

      cv::UMat result;
      mask_victim.convertTo (mask_victim, CV_32FC3);
      /* Set mask pixel channels values between 0.0 and 1.0 */
      mask.convertTo(mask, CV_32FC3, 1.0 / 255);
      cvImg.convertTo (result, CV_32FC3);

      /* Weight foreground */
      cv::multiply (mask, mask_victim, mask_victim);
      /* Invert the mask */
      cv::subtract (cv::Scalar::all(1.0), mask, mask);
      /* Weight background */
      cv::multiply (mask, result, result);
      /* Blend foreground and background */
      cv::add(mask_victim, result, result);

      result.convertTo (result, CV_8UC3);
      result.copyTo (cvImg);
      if (debug)
        time_copy_frame += cv::getTickCount () - time_start;

      GST_DEBUG ("Time to create black frame: %.2f ms.",
          ((double) time_create_black_frame * 1000) / cv::getTickFrequency ());
      GST_DEBUG ("Time to update animation counter: %.2f ms.",
          ((double) time_animation_counter * 1000) / cv::getTickFrequency ());
      GST_DEBUG ("Time to select image: %.2f ms.",
          ((double) time_select_image * 1000) / cv::getTickFrequency ());
      GST_DEBUG ("Time to create polygons: %.2f ms.",
          ((double) time_polygons * 1000) / cv::getTickFrequency ());
      GST_DEBUG ("Time to draw polygons: %.2f ms.",
          ((double) time_draw_polygons * 1000) / cv::getTickFrequency ());
      GST_DEBUG ("Time to copy frame: %.2f ms.",
          ((double) time_copy_frame * 1000) / cv::getTickFrequency ());
    }
  }
  return ret;
}

static void
gst_cheese_face_omelette_finalize (GObject * obj)
{
  guint i;
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (obj);
  if (filter->omelette)
    delete filter->omelette;
  if (filter->omelette_oregano);
    delete filter->omelette_oregano;
  for (i = 0; i < G_N_ELEMENTS (filter->cheeses); i++)
    if (filter->cheeses[i])
      delete filter->cheeses[i];
  for (i = 0; i < G_N_ELEMENTS (filter->tomatoes); i++)
    if (filter->tomatoes[i])
      delete filter->tomatoes[i];
  G_OBJECT_CLASS (gst_cheese_face_omelette_parent_class)->finalize (obj);
}
