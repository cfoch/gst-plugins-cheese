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
 * SECTION:element-cheesefacetrack
 *
 * FIXME:Describe cheesefacetrack here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! cheesefacetrack ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <gst/cheese/face/cheesemultifacemeta.h>
#include <gst/cheese/face/cheesefaceinfo.h>

#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <math.h>

#include "gstcheesefacetrack.h"
#include "facetrack.h"
#include "utils.h"

using namespace std;
using namespace dlib;

#define INIT_FRAME_COUNTER()                              1

static void gst_cheese_face_track_finalize (GObject * obj);
static void gst_cheese_face_track_set_property (GObject * object,
    guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_cheese_face_track_get_property (GObject * object,
    guint prop_id, GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_cheese_face_track_transform_ip (
    GstOpencvVideoFilter * filter, GstBuffer * buf, IplImage * img);

struct _GstCheeseFaceTrack
{
  GstOpencvVideoFilter element;

  /* props */
  gboolean display_bounding_box;
  gboolean display_id;
  gboolean display_landmark;
  gboolean display_detection_phase;
  gchar *landmark;
  GstCheeseFaceTrackTrackerType tracker_type;
  guint tracker_duration;
  guint delete_threshold;
  gfloat scale_factor;
  gdouble distance_factor;
  guint detection_gap_duration;

  /* private props */
  dlib::frontal_face_detector *face_detector;
  dlib::shape_predictor *shape_predictor;

  guint last_face_id;
  guint frame_number;
  std::map<guint, CheeseFace> *faces;
  GHashTable *face_table;
};

struct _GstCheeseFaceTrackClass
{
  GstOpencvVideoFilterClass parent_class;

  /* funcs */
  CheeseFaceFreeFunc cheese_face_free_user_data_func;
};


#define GST_TYPE_CHEESEFACETRACKER_TRACKER (gst_cheese_face_track_tracker_get_type ())
static GType
gst_cheese_face_track_tracker_get_type (void)
{
  static GType tracker_type = 0;

  if (!tracker_type) {
    static GEnumValue tracker_types[] = {
      { GST_CHEESEFACETRACK_TRACKER_BOOSTING, "Boosting", "boosting" },
      { GST_CHEESEFACETRACK_TRACKER_GOTURN,  "GOTURN", "goturn"  },
      { GST_CHEESEFACETRACK_TRACKER_KCF, "Kernelized Correlation Filters",
          "kcf" },
      { GST_CHEESEFACETRACK_TRACKER_MEDIANFLOW, "Median Flow", "median-flow" },
      { GST_CHEESEFACETRACK_TRACKER_MIL, "Multiple Instance Learning", "mil" },
      { GST_CHEESEFACETRACK_TRACKER_TLD, "Tracking Learning Detection",
          "tld" },
      { 0, NULL, NULL }
    };

    tracker_type = g_enum_register_static ("GstCheeseFaceTrackTrackerType",
        tracker_types);
  }
  return tracker_type;
}

#define DEFAULT_DELETE_THRESHOLD                          72
#define DEFAULT_SCALE_FACTOR                              1.0
#define DEFAULT_TRACKER                                   GST_CHEESEFACETRACK_TRACKER_MEDIANFLOW
#define DEFAULT_DETECTION_GAP_DURATION                    10
#define DEFAULT_DISTANCE_FACTOR                           10.0
#define DEFAULT_BOUNDING_BOX_DETECT_COLOR                 cv::Scalar (255, 255, 0)
#define DEFAULT_BOUNDING_BOX_TRACK_COLOR                  cv::Scalar (0, 255, 0)
#define DEFAULT_LANDMARK_COLOR                            cv::Scalar (255, 140, 0)
#define DEFAULT_ID_COLOR                                  cv::Scalar (255, 0, 0)
#define DEFAULT_DETECTION_PHASE_COLOR                     cv::Scalar (0, 0, 255)


GST_DEBUG_CATEGORY_STATIC (gst_cheese_face_track_debug);
#define GST_CAT_DEFAULT gst_cheese_face_track_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_DISPLAY_BOUNDING_BOX,
  PROP_DISPLAY_ID,
  PROP_DISPLAY_LANDMARK,
  PROP_DISPLAY_DETECTION_PHASE,
  PROP_LANDMARK,
  PROP_TRACKER,
  PROP_TRACKER_DURATION,
  PROP_DELETE_THRESHOLD,
  PROP_SCALE_FACTOR,
  PROP_DISTANCE_FACTOR,
  PROP_DETECTION_GAP_DURATION
};

// static dlib::frontal_face_detector mydetector = get_frontal_face_detector();

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
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

#define gst_cheese_face_track_parent_class parent_class
G_DEFINE_TYPE (GstCheeseFaceTrack, gst_cheese_face_track,
               GST_TYPE_OPENCV_VIDEO_FILTER);

/* GObject vmethod implementations */

/* initialize the cheesefacetrack's class */
static void
gst_cheese_face_track_class_init (GstCheeseFaceTrackClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstOpencvVideoFilterClass *gstopencvbasefilter_class;

  GST_DEBUG_CATEGORY_INIT (gst_cheese_face_track_debug, "gstcheesefacetrack",
      0, "Cheese Face Detect");

  klass->cheese_face_free_user_data_func = NULL;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstopencvbasefilter_class = (GstOpencvVideoFilterClass *) klass;

  gstopencvbasefilter_class->cv_trans_ip_func =
      gst_cheese_face_track_transform_ip;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_cheese_face_track_finalize);
  gobject_class->set_property = gst_cheese_face_track_set_property;
  gobject_class->get_property = gst_cheese_face_track_get_property;

  /* TODO */
  /* Remove unused properties. */
  g_object_class_install_property (gobject_class, PROP_DISPLAY_BOUNDING_BOX,
      g_param_spec_boolean ("display-bounding-box", "Display",
          "Sets whether the detected faces should be highlighted in the output",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_ID,
      g_param_spec_boolean ("display-id", "Display the ID of each face",
          "Sets whether to display the ID of each face",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_LANDMARK,
      g_param_spec_boolean ("display-landmark", "Display the landmark",
          "Sets whether display the landmark for each face",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DISPLAY_DETECTION_PHASE,
      g_param_spec_boolean ("display-detection-phase",
          "Display detection phase",
          "Sets whether to display or not the bounding boxes of faces detected "
          "during the detection phase.",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_LANDMARK,
      g_param_spec_string ("landmark", "Landmark shape model",
          "Location of the shape model. You can get one from "
          "http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2",
          NULL, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_TRACKER,
      g_param_spec_enum ("tracker", "Tracker algorithm",
          "Type of the tracker algorithm to use",
          GST_TYPE_CHEESEFACETRACKER_TRACKER, DEFAULT_TRACKER,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_DELETE_THRESHOLD,
      g_param_spec_uint ("delete-threshold",
          "Delete threshold",
          "Sets the number of frames that need to pass to delete a face if it "
          "has not been detected in that period",
          0, G_MAXUINT,
          DEFAULT_DELETE_THRESHOLD,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_SCALE_FACTOR,
      g_param_spec_float ("scale-factor", "Scale Factor",
          "Sets the scale factor the frame which be scaled with before face "
          "detection",
          0, G_MAXFLOAT,
          DEFAULT_SCALE_FACTOR,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_DISTANCE_FACTOR,
      g_param_spec_double ("max-distance-factor", "Max distance factor",
          "Sets the maximum distance factor to calculate the maximum distance "
          "that will be applied between centroids of the tracker and detector. "
          "This factor is in relation to the bounding box width. The value of "
          "this factor should usually be very low.",
          0.0, G_MAXDOUBLE, DEFAULT_DISTANCE_FACTOR,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DETECTION_GAP_DURATION,
      g_param_spec_uint ("detection-gap-duration", "Detection gap duration",
          "Sets the maximum number of frames between each detection phase.",
          1, G_MAXUINT, DEFAULT_DETECTION_GAP_DURATION,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));


  gst_element_class_set_details_simple (gstelement_class,
    "cheesefacetrack",
    "Filter/Effect/Video",
    "Performs multiple face tracking and detects facial landmarks. It works by "
    "a cycle of tracking bursts followed by face detection in order to "
    "robustly track faces. This filter preserves identity of faces for a long "
    "time.",
    "Fabian Orccon <cfoch.fabian@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_cheese_face_track_init (GstCheeseFaceTrack * filter)
{
  filter->last_face_id = 0;
  filter->frame_number = INIT_FRAME_COUNTER ();

  filter->delete_threshold = DEFAULT_DELETE_THRESHOLD;
  filter->display_bounding_box = TRUE;
  filter->display_id = TRUE;
  filter->landmark = NULL;
  filter->tracker_type = DEFAULT_TRACKER;
  filter->detection_gap_duration = DEFAULT_DETECTION_GAP_DURATION;
  filter->face_detector =
      new frontal_face_detector (get_frontal_face_detector());
  filter->shape_predictor = NULL;
  filter->scale_factor = DEFAULT_SCALE_FACTOR;
  filter->distance_factor = DEFAULT_DISTANCE_FACTOR;

  filter->faces = new std::map<guint, CheeseFace>;

  gst_opencv_video_filter_set_in_place (GST_OPENCV_VIDEO_FILTER_CAST (filter),
      TRUE);
}

static void
gst_cheese_face_track_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceTrack *filter = GST_CHEESEFACETRACK (object);

  switch (prop_id) {
    case PROP_DISPLAY_BOUNDING_BOX:
      filter->display_bounding_box = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_ID:
      filter->display_id = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_LANDMARK:
      filter->display_landmark = g_value_get_boolean (value);
      break;
    case PROP_DISPLAY_DETECTION_PHASE:
      filter->display_detection_phase = g_value_get_boolean (value);
      break;
    case PROP_LANDMARK:
      filter->landmark = g_value_dup_string (value);
      filter->shape_predictor = new dlib::shape_predictor;
      try {
        dlib::deserialize (filter->landmark) >> *filter->shape_predictor;
      } catch (dlib::serialization_error &e) {
        GST_ERROR ("Error when deserializing landmark predictor model: %s",
            e.info.c_str());
        delete filter->shape_predictor;
        filter->shape_predictor = NULL;
      }
      break;
    case PROP_TRACKER:
      filter->tracker_type =
          (GstCheeseFaceTrackTrackerType) g_value_get_enum (value);
      break;
    case PROP_TRACKER_DURATION:
      filter->tracker_duration = g_value_get_uint (value);
      break;
    case PROP_DELETE_THRESHOLD:
      filter->delete_threshold = g_value_get_uint (value);
      break;
    case PROP_SCALE_FACTOR:
      filter->scale_factor = g_value_get_float (value);
      break;
    case PROP_DISTANCE_FACTOR:
      filter->distance_factor = g_value_get_double (value);
      break;
    case PROP_DETECTION_GAP_DURATION:
      filter->detection_gap_duration = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_cheese_face_track_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstCheeseFaceTrack *filter = GST_CHEESEFACETRACK (object);

  switch (prop_id) {
    case PROP_DISPLAY_BOUNDING_BOX:
      g_value_set_boolean (value, filter->display_bounding_box);
      break;
    case PROP_DISPLAY_ID:
      g_value_set_boolean (value, filter->display_id);
      break;
    case PROP_DISPLAY_LANDMARK:
      g_value_set_boolean (value, filter->display_landmark);
      break;
    case PROP_DISPLAY_DETECTION_PHASE:
      g_value_set_boolean (value, filter->display_detection_phase);
      break;
    case PROP_LANDMARK:
      g_value_set_string (value, filter->landmark);
      break;
    case PROP_TRACKER:
      g_value_set_enum (value, filter->tracker_type);
      break;
    case PROP_TRACKER_DURATION:
      g_value_set_uint (value, filter->tracker_duration);
      break;
    case PROP_DELETE_THRESHOLD:
      g_value_set_uint (value, filter->delete_threshold);
      break;
    case PROP_SCALE_FACTOR:
      g_value_set_float (value, filter->scale_factor);
      break;
    case PROP_DISTANCE_FACTOR:
      g_value_set_double (value, filter->distance_factor);
      break;
    case PROP_DETECTION_GAP_DURATION:
      g_value_set_uint (value, filter->detection_gap_duration);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_cheese_face_track_is_detection_phase (GstCheeseFaceTrack * filter)
{
  return filter->frame_number % filter->detection_gap_duration == 1;
}

static void
gst_cheese_face_track_try_scale_image (GstCheeseFaceTrack * filter,
    cv::Mat & img, cv::Mat & resized_img)
{

  if (filter->scale_factor != 1.0) {
    cv::resize(img, resized_img, cv::Size (img.cols * filter->scale_factor,
        img.rows * filter->scale_factor));
    GST_LOG ("Image scaled by the factor %.2f. New image processing size: "
        "%d (height) x %d (width).", filter->scale_factor,
        resized_img.rows, resized_img.cols);
  }
}

static void
gst_cheese_face_track_detect_faces (GstCheeseFaceTrack * filter,
    dlib::cv_image<bgr_pixel> & dlib_img, std::vector<dlib::rectangle> & dets)
{
  dets = (*filter->face_detector) (dlib_img);
}

static std::vector<guint>
gst_cheese_face_track_create_faces (GstCheeseFaceTrack * filter,
    cv::Mat & img, std::vector<dlib::rectangle> & dets)
{
  GstCheeseFaceTrackClass *klass = GST_CHEESEFACETRACK_GET_CLASS (filter);
  std::vector<guint> created_faces_ids;
  guint i;
  for (i = 0; i < dets.size(); i++) {
    /**
     * FIXME (Ineficient code)
     * When we declare a new CheeseFace in this way we actually create a
     * copy and this copy is obviously destructed after the end of this
     * block/scope. A solution may be to use a constructor but I am not
     * sure yet if CheeseFace should be a struct, a C++ class, a GObject...
     **/
    CheeseFace face_info;
    face_info.set_last_detected_frame (filter->frame_number);
    face_info.set_bounding_box (dets[i]);
    face_info.free_user_data_func = klass->cheese_face_free_user_data_func;
    /* Store face info in filter's dictionary. */
    filter->last_face_id++;
    (*filter->faces)[filter->last_face_id] = face_info;
    /* Init tracker */
    (*filter->faces)[filter->last_face_id].create_tracker (
        filter->tracker_type);
    (*filter->faces)[filter->last_face_id].init_tracker (img);

    created_faces_ids.push_back (filter->last_face_id);

    GST_LOG ("Face %d: this face has just been created.", filter->last_face_id);
  };
  return created_faces_ids;
}

static std::vector<cv::Point>
get_centroids (std::vector<dlib::rectangle> & dets)
{
  guint i;
  std::vector<cv::Point> centroids;

  for (i = 0; i < dets.size (); i++) {
    cv::Rect2d rect;
    cv::Point centroid;
    dlib_rectangle_to_cv_rect (dets[i], rect);
    centroid = (rect.tl () + rect.br ()) * 0.5;
    centroids.push_back(centroid);
  }
  return centroids;
}

gboolean
gst_cheese_face_track_display_face (GstCheeseFaceTrack * filter,
    CheeseFace & face)
{
  return face.last_detected_frame () == filter->frame_number;
}

static std::vector<guint>
gst_cheese_face_track_try_to_remove_faces (GstCheeseFaceTrack * filter)
{
  std::vector<guint> to_remove;
  for (auto &kv : *filter->faces) {
    guint delta_since_detected;
    guint id = kv.first;
    CheeseFace &face = kv.second;

    delta_since_detected = filter->frame_number - face.last_detected_frame ();
    GST_LOG ("Face %d: number of frames passed since last detection of "
        "face is delta=%d.", id, delta_since_detected);

    if (delta_since_detected > filter->delete_threshold) {
      GST_LOG ("Face %d will be deleted: "
          "delta=%d > delete-threshold=%d.", id,
          delta_since_detected, filter->delete_threshold);
      to_remove.push_back (id);
    }
  }

  return to_remove;
}

static GstFlowReturn
gst_cheese_face_track_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  GstCheeseFaceTrack *filter = GST_CHEESEFACETRACK (base);
  GstCheeseMultifaceMeta *multiface_meta;
  /* Frame storage */
  cv::Mat cv_img (cv::cvarrToMat (img));
  cv::Mat cv_resized_img (cv::cvarrToMat (img));
  std::vector<dlib::rectangle> resized_dets;
  dlib::cv_image<bgr_pixel> dlib_resized_img;
  std::vector<guint> faces_ids_with_lost_target;
  std::vector<guint> faces_ids_to_remove;
  guint i;

  gst_cheese_face_track_try_scale_image (filter, cv_img, cv_resized_img);
  dlib_resized_img = cv_image<bgr_pixel> (cv_resized_img);

  std::vector<guint> non_created_faces_ids;

  GST_DEBUG ("Frame number: %d.", filter->frame_number);

  multiface_meta = gst_buffer_add_cheese_multiface_meta (buf);

  /* If there are faces to remove add them to the metadata. */
  faces_ids_to_remove = gst_cheese_face_track_try_to_remove_faces (filter);
  for (i = 0; i < faces_ids_to_remove.size (); i++) {
    filter->faces->erase (faces_ids_to_remove[i]);
    gst_cheese_multiface_meta_add_removed_face_id (multiface_meta,
        faces_ids_to_remove[i]);
    GST_LOG ("Face %d: this face has just been deleted.",
        faces_ids_to_remove[i]);
  }

  for (auto &kv : *filter->faces) {
    guint id = kv.first;
    CheeseFace &face = kv.second;
    if (face.update_tracker (cv_resized_img)) {
      GST_LOG ("Face %d: tracker updated.", id);
      face.set_last_detected_frame (filter->frame_number);
    } else {
      GST_LOG ("Face %d: tracker lost its target.", id);
      faces_ids_with_lost_target.push_back (id);
    }
    non_created_faces_ids.push_back (id);
  }

  /* There is a detection cycle in the case new faces enter to the scene. */
  if (gst_cheese_face_track_is_detection_phase (filter) ||
      faces_ids_with_lost_target.size () > 0) {
    if (gst_cheese_face_track_is_detection_phase (filter))
      GST_LOG ("Detection phase.");
    if (faces_ids_with_lost_target.size () > 0)
      GST_LOG ("Detection phase was forced because a tracker lost its target.");

    gst_cheese_face_track_detect_faces (filter, dlib_resized_img, resized_dets);

    /* Init faces, and thus create trackers */
    if (filter->faces->empty ())
      gst_cheese_face_track_create_faces (filter, cv_resized_img, resized_dets);

    if (!non_created_faces_ids.empty () && resized_dets.size () > 0) {
      guint r, c;
      HungarianAlgorithm HungAlgo;
      std::vector<int> assignment;
      std::vector<std::vector<double>> cost_matrix;
      std::vector<cv::Point> detection_centroids;

      detection_centroids = get_centroids (resized_dets);

      GST_LOG ("Hungarian method: initialize cost matrix of "
          "detected faces x filter's faces excluding just created: "
          "%d rows x %d cols",
          (gint) detection_centroids.size (),
          (gint) non_created_faces_ids.size ());

      /* Initialize cost matrix. */
      for (r = 0; r < detection_centroids.size (); r++) {
        std::vector<double> row;
        for (c = 0; c < non_created_faces_ids.size (); c++) {
          const guint id = non_created_faces_ids[c];
          CheeseFace &face = (*filter->faces)[id];
          double dist;
          dist = cv::norm (cv::Mat (detection_centroids[r]),
              cv::Mat (face.bounding_box_centroid ()));
          row.push_back (dist);
        }
        cost_matrix.push_back (row);
      }

      /* Solve the Hungarian problem */
      GST_LOG ("Hungarian method: solve the Hungarian problem.");
      HungAlgo.Solve(cost_matrix, assignment);

      for (i = 0; i < cost_matrix.size (); i++) {
        const gboolean asigned = assignment[i] != -1;
        gboolean create_face;

        /* Discard faces if they are too far. */
        if (asigned) {
          const gint id = non_created_faces_ids[assignment[i]];
          CheeseFace &face = (*filter->faces)[id];
          double max_dist;
          double dist;

          create_face = FALSE;

          dist = cv::norm (cv::Mat (detection_centroids[i]),
              cv::Mat ((*filter->faces)[id].bounding_box_centroid ()));
          max_dist = filter->distance_factor * resized_dets[i].width ();

          if (dist >= max_dist) {
            GST_LOG ("Face %d: this face is too far from detector", id);
            face.release_tracker ();
          }
        } else {
          create_face = TRUE;
        }

        if (create_face) {
          std::vector<dlib::rectangle> det_wrap;
          det_wrap.push_back (resized_dets[i]);
          /* Assume a new face was found. Create a new face. */
          GST_LOG ("Face detector at index %d could not be assigned.", i);
          gst_cheese_face_track_create_faces (filter, cv_resized_img, det_wrap);
        } else {
          const gint id = non_created_faces_ids[assignment[i]];
          CheeseFace &face = (*filter->faces)[id];
          /* Create a new tracker if the target was lost. */
          if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_UNSET) {
            GST_LOG ("Face %d: creating a new tracker because target was lost.",
                id);
            face.set_bounding_box (resized_dets[i]);
            face.create_tracker (filter->tracker_type);
            face.init_tracker (cv_resized_img);
            face.set_last_detected_frame (filter->frame_number);
          }
        }
      }
    }
  }

  if (filter->display_detection_phase && resized_dets.size () > 0) {
    guint i;
    GST_LOG ("Displaying detection phase.");
    for (i = 0; i < resized_dets.size (); i++) {
      cv::Rect2d cv_resized_rect, cv_rect;
      cv::Point centroid;
      dlib_rectangle_to_cv_rect (resized_dets[i], cv_resized_rect);
      cv_rect = cv::Rect (
          cv_resized_rect.x / filter->scale_factor,
          cv_resized_rect.y / filter->scale_factor,
          cv_resized_rect.width / filter->scale_factor,
          cv_resized_rect.height / filter->scale_factor);

      centroid = (cv_rect.tl () + cv_rect.br ()) * 0.5;
      cv::putText (cv_img, std::to_string (i), centroid,
          cv::FONT_HERSHEY_SIMPLEX, 1.0, DEFAULT_DETECTION_PHASE_COLOR);
      cv::rectangle (cv_img, cv_rect, DEFAULT_DETECTION_PHASE_COLOR, 5);
    }
  }

  for (auto &kv : *filter->faces) {
    guint id = kv.first;
    CheeseFace &face = kv.second;
    GstCheeseFaceInfo *info;
    gboolean display;

    display = gst_cheese_face_track_display_face (filter, face);

    if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED ||
        face.state () == CHEESE_FACE_INFO_STATE_TRACKER_WAITING) {
      cv::Rect2d resized_bounding_box, bounding_box;
      cv::Point centroid;

      /* Scale to original size. */
      resized_bounding_box = face.bounding_box ();
      bounding_box = cv::Rect (
          resized_bounding_box.x / filter->scale_factor,
          resized_bounding_box.y / filter->scale_factor,
          resized_bounding_box.width / filter->scale_factor,
          resized_bounding_box.height / filter->scale_factor);
      centroid = (bounding_box.tl () + bounding_box.br ()) * 0.5;

      /* Set landmark. */
      if (filter->shape_predictor) {
        std::vector<cv::Point> landmark;
        dlib::rectangle dlib_resized_bounding_box;
        dlib::full_object_detection shape;

        cv_rect_to_dlib_rectangle (resized_bounding_box,
            dlib_resized_bounding_box);
        shape = (*filter->shape_predictor)
            (dlib_resized_img, dlib_resized_bounding_box);

        if (filter->display_landmark)
          GST_LOG ("Face %d: drawing landmark.", id);

        for (i = 0; i < shape.num_parts (); i++) {
          cv::Point resized_keypoint (shape.part (i).x (), shape.part (i).y ());
          landmark.push_back (resized_keypoint);
          /* Draw */
          if (display && filter->display_landmark) {
            cv::circle(cv_img, resized_keypoint / filter->scale_factor, 1,
                DEFAULT_LANDMARK_COLOR, CV_FILLED);
          }
        }
        face.set_landmark (landmark);
      }

      /* Draw */
      if (display) {
        GST_LOG ("Face %d: drawing bounding box. Position: (%d, %d)."
            "Size (w x h): %d x %d.", id,
            (int) bounding_box.x, (int) bounding_box.y,
            (int) bounding_box.width, (int) bounding_box.height);
        if (filter->display_bounding_box) {
          if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED) {
            cv::rectangle (cv_img, bounding_box,
                DEFAULT_BOUNDING_BOX_DETECT_COLOR);
          } else if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_WAITING) {
            cv::rectangle (cv_img, bounding_box,
                DEFAULT_BOUNDING_BOX_TRACK_COLOR);
          }
        }
        if (filter->display_id) {
          GST_LOG ("Face %d: drawing id number. Position: (%d, %d).", id,
              (int) centroid.x, (int) centroid.y);
          cv::putText (cv_img, std::to_string (id), centroid,
              cv::FONT_HERSHEY_SIMPLEX, 1.0, DEFAULT_ID_COLOR);
        }
      }
    }
    /* Set metadata */
    info = face.to_face_info_at_scale (1.0 / filter->scale_factor);
    cheese_face_info_set_display (info, display);
    gst_cheese_multiface_info_insert (multiface_meta->faces, id, info);
  }

  filter->frame_number++;
  return GST_FLOW_OK;
}

static void
gst_cheese_face_track_finalize (GObject * obj)
{
  GstCheeseFaceTrack *filter = GST_CHEESEFACETRACK (obj);

  if (filter->face_detector)
    delete filter->face_detector;
  if (filter->shape_predictor)
    delete filter->shape_predictor;

  G_OBJECT_CLASS (gst_cheese_face_track_parent_class)->finalize (obj);
}
