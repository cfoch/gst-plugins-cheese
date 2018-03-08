/*
 * GStreamer
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
 * gst-launch -v -m fakesrc ! cheesefaceomelette ! fakesink silent=TRUE
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


static cv::Scalar WHITE(255, 255, 255);
static cv::Scalar BLACK(0, 0, 0);
static guint FACE_LIPS_INTERNAL_BORDER[] =
    {49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60};
static guint FACE_LIPS_EXTERNAL_BORDER[] = {61, 62, 63, 64, 65, 66, 67, 68};
static guint FACE_LEFT_EYE_BORDER[] = {37, 38, 39, 40, 41, 42};
static guint FACE_RIGHT_EYE_BORDER[] = {43, 44, 45, 46, 47, 48};

/*
#define DEFAULT_HUNGARIAN_DELETE_THRESHOLD                72
#define DEFAULT_SCALE_FACTOR                              1.0
*/

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

/* GObject vmethod implementations */

/* initialize the cheesefaceomelette's class */
static void
gst_cheese_face_omelette_class_init (GstCheeseFaceOmeletteClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstOpencvVideoFilterClass *gstopencvbasefilter_class;

  GST_DEBUG_CATEGORY_INIT (gst_cheese_face_omelette_debug,
      "gstcheesefaceomelette", 0, "Cheese Face Detect");

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstopencvbasefilter_class = (GstOpencvVideoFilterClass *) klass;

  gstopencvbasefilter_class->cv_trans_ip_func =
     gst_cheese_face_omelette_transform_ip;

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

#define OMELETTE_ASSETS_DIR  "/home/cfoch/Documents/git/gst-plugins-cheese/png/omelette/"

static void
gst_cheese_face_omelette_init (GstCheeseFaceOmelette * filter)
{
  guint i;

  /* Load overlay images */
  /* FIXME: This should be done in the GNOME way: gresources*/

  /*
  filter->background = new cv::Mat;
  *filter->background = cv::imread (OMELETTE_ASSETS_DIR "background.png",
      CV_LOAD_IMAGE_UNCHANGED);
  */

  filter->omelette = new cv::Mat;
  *filter->omelette = cv::imread (OMELETTE_ASSETS_DIR "omelette.png",
      CV_LOAD_IMAGE_UNCHANGED);

  filter->omelette_oregano = new cv::Mat;
  *filter->omelette_oregano = cv::imread (
      OMELETTE_ASSETS_DIR "omelette_oregano.png", CV_LOAD_IMAGE_UNCHANGED);

  for (i = 0; i < G_N_ELEMENTS (filter->cheeses); i++) {
    gchar *filename;
    filter->cheeses[i] = new cv::Mat;
    filename = g_strdup_printf (OMELETTE_ASSETS_DIR "cheese-%d.png", i + 1);
    *filter->cheeses[i] = cv::imread (filename, CV_LOAD_IMAGE_UNCHANGED);
    g_free (filename);
  }
  for (i = 0; i < G_N_ELEMENTS (filter->tomatoes); i++) {
    gchar *filename;
    filter->tomatoes[i] = new cv::Mat;
    filename = g_strdup_printf (OMELETTE_ASSETS_DIR "tomato-%d.png", i + 1);
    *filter->tomatoes[i] = cv::imread (filename, CV_LOAD_IMAGE_UNCHANGED);
    g_free (filename);
  }
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

static GstFlowReturn
gst_cheese_face_omelette_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  guint i, j;
  GstFlowReturn ret;
  GstOpencvVideoFilterClass *bclass =
    GST_OPENCV_VIDEO_FILTER_CLASS (gst_cheese_face_omelette_parent_class);
  GstCheeseFaceDetect *parent_filter = GST_CHEESEFACEDETECT (base);
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (base);
  cv::Mat cvImg (cv::cvarrToMat (img));
  cv::Size sz = cvImg.size ();


  if ((ret = bclass->cv_trans_ip_func (base, buf, img)) != GST_FLOW_OK)
    return ret;

  /*cv::Mat background_mask;
  cv::Mat background;*/
  cv::Mat landmark_mask(sz.height, sz.width, CV_8UC3, BLACK);
  cv::Mat mask_victim (sz.height, sz.width, CV_8UC3, BLACK);
  cv::Mat masked (sz.height, sz.width, CV_8UC3, BLACK);
  cv::Mat overlay(sz.height, sz.width, CV_8UC3, BLACK);

  /* Scale background to fit input frame */
  /* TODO do this only in the first frame! */

  /*
  cv::resize (*filter->background, *filter->background, sz, CV_INTER_CUBIC);
  cv::extractChannel (*filter->background, background_mask, 3);
  cv::cvtColor(background_mask, background_mask, CV_GRAY2RGB);
  cvtColor (*filter->background, background, CV_BGR2RGB);

  for (i = 0; i < sz.width; i++) {
    for (j = 0; j < sz.height; j++) {
      cv::Vec3b &mask_px = background_mask.at<cv::Vec3b>(j, i);
      float alpha = float (mask_px[0]) / 255.0;
      float inv_alpha = 1 - alpha;
      cv::Vec3b &in_px = cvImg.at<cv::Vec3b> (j, i);
      cv::Vec3b &mask_bg_px = background.at<cv::Vec3b>(j, i);
      cv::Vec3b px;
      px = cv::Vec3b (
          mask_bg_px[0] * alpha + in_px[0] * inv_alpha,
          mask_bg_px[1] * alpha + in_px[1] * inv_alpha,
          mask_bg_px[2] * alpha + in_px[2] * inv_alpha);
      cvImg.at<cv::Vec3b> (j, i) = px;
    }
  }
  */

  for (auto &kv : *parent_filter->faces) {
    guint id = kv.first;
    CheeseFace &face = kv.second;

    /* The parent already incremented the frame counter */
    if (face.last_detected_frame == parent_filter->frame_number - 1) {
      std::vector<cv::Point> face_lips_internal_pts;
      std::vector<cv::Point> face_left_eye_internal_pts;
      std::vector<cv::Point> face_right_eye_internal_pts;
      /* images */
      cv::Mat omelette, omelette_mask;
      cv::Size omelette_sz;
      gfloat scale_factor;

      scale_factor =
          face.bounding_box.width () / (gfloat) filter->omelette->cols;

      /*
      cout << "cvImg.cols: " << cvImg.cols << endl;
      cout << "cvImg.rows: " << cvImg.rows << endl;
      cout << "bounding_box.width: " << face.bounding_box.width () << endl;
      cout << "bounding_box.height: " << face.bounding_box.height () << endl;
      cout << "filter->omelette->cols: " << filter->omelette->cols << endl;
      cout << "filter->omelette->rows: " << filter->omelette->rows << endl;
      */

      // omelette_sz = cv::Size (face.bounding_box.width (),
      //   filter->omelette->rows * scale_factor);
      cv::resize (*filter->omelette, omelette, cv::Size (), scale_factor, 
          scale_factor);

      /* Build mask */
      cv::extractChannel (omelette, omelette_mask, 3);
      cv::cvtColor(omelette_mask, omelette_mask, CV_GRAY2RGB);


      for (i = 0; i < omelette_sz.width; i++) {
        for (j = 0; j < omelette_sz.height; j++) {
          const guint mask_i = i + face.bounding_box.left ();
          const guint mask_j = j + face.bounding_box.top ();
          landmark_mask.at<cv::Vec3b> (mask_j, mask_i) =
              omelette_mask.at<cv::Vec3b>(j, i);
        }
      }


      /* Draw mask for face internal lips */
      for (i = 0; i < G_N_ELEMENTS (FACE_LIPS_INTERNAL_BORDER); i++)
        face_lips_internal_pts.push_back (
            face.landmark[FACE_LIPS_INTERNAL_BORDER[i] - 1]);
      for (i = 0; i < G_N_ELEMENTS (FACE_LEFT_EYE_BORDER); i++)
        face_left_eye_internal_pts.push_back (
            face.landmark[FACE_LEFT_EYE_BORDER[i] - 1]);
      for (i = 0; i < G_N_ELEMENTS (FACE_RIGHT_EYE_BORDER); i++)
        face_right_eye_internal_pts.push_back (
            face.landmark[FACE_RIGHT_EYE_BORDER[i] - 1]);


      cv::fillConvexPoly(landmark_mask, &face_lips_internal_pts[0],
          face_lips_internal_pts.size (), BLACK);
      cv::fillConvexPoly(landmark_mask, &face_left_eye_internal_pts[0],
          face_left_eye_internal_pts.size (), BLACK);
      cv::fillConvexPoly(landmark_mask, &face_right_eye_internal_pts[0],
          face_right_eye_internal_pts.size (), BLACK);


      /*
      cvImg.copyTo (cvImg, landmark_mask);

      /* Dumbness */

      /* After masking */
      cvtColor (omelette, omelette, CV_BGR2RGB);
      for (i = 0; i < omelette_sz.width; i++) {
        for (j = 0; j < omelette_sz.height; j++) {
          const guint in_i = face.bounding_box.left ();
          const guint in_j = face.bounding_box.top ();
          mask_victim.at<cv::Vec3b> (in_j + j, in_i + i) =
              omelette.at<cv::Vec3b> (j, i);
        }
      }
      mask_victim.copyTo (masked, landmark_mask);


      /* TODO */
      /* This is hardcoded and I also think that I am copying pixels */
      /* unnecesarily */
      for (i = 0; i < sz.width; i++) {
        for (j = 0; j < sz.height; j++) {
          cv::Vec3b &mask_px = landmark_mask.at<cv::Vec3b>(j, i);
          float alpha = mask_px[0] / 255.0;
          float inv_alpha = 1 - alpha;
          cv::Vec3b &in_px = cvImg.at<cv::Vec3b> (j, i);
          cv::Vec3b &mask_victim_px = mask_victim.at<cv::Vec3b>(j, i);
          cvImg.at<cv::Vec3b> (j, i) = cv::Vec3b (
              mask_victim_px[0] * alpha + in_px[0] * inv_alpha,
              mask_victim_px[1] * alpha + in_px[1] * inv_alpha,
              mask_victim_px[2] * alpha + in_px[2] * inv_alpha
          );

        }
      }
    }
  }

  return ret;
}

static void
gst_cheese_face_omelette_finalize (GObject * obj)
{
  guint i;
  GstCheeseFaceOmelette *filter = GST_CHEESEFACEOMELETTE (obj);
  /*
  if (filter->background)
    delete filter->background;
  */
  if (filter->omelette)
    delete filter->omelette;
  if (filter->omelette_oregano);
    delete filter->omelette_oregano;
  for (i = 0; i < G_N_ELEMENTS (filter->cheeses); i++)
    delete filter->cheeses[i];
  for (i = 0; i < G_N_ELEMENTS (filter->tomatoes); i++)
    delete filter->tomatoes[i];

  G_OBJECT_CLASS (gst_cheese_face_omelette_parent_class)->finalize (obj);
}
