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

#include "gstcheesefacetrack.h"

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
static void dlib_rectangle_to_cv_rect(dlib::rectangle & dlib_rect,
    cv::Rect2d & cv_rect);

typedef void (* CheeseFaceFreeFunc) (gpointer);

static void
dlib_rectangle_to_cv_rect (dlib::rectangle & dlib_rect, cv::Rect2d & cv_rect)
{
  cv_rect = cv::Rect2d (cv::Point (dlib_rect.left (), dlib_rect.top ()),
      cv::Point (dlib_rect.right () + 1, dlib_rect.bottom () + 1));
}

/*
static void
cv_rect_to_dlib_rectangle (cv::Rect2d & cv_rect, dlib::rectangle & dlib_rect)
{
  long tl_x, tl_y, br_x, br_y;

  tl_x = (long) cv_rect.tl ().x;
  tl_y = (long) cv_rect.tl ().y;
  br_x = (long) cv_rect.tl ().x - 1;
  br_y = (long) cv_rect.tl ().y - 1;

  dlib_rect = dlib::rectangle (tl_x, tl_y, br_x, br_y);
}
*/


typedef enum {
  CHEESE_FACE_INFO_STATE_TRACKER_UNSET,
  CHEESE_FACE_INFO_STATE_TRACKER_UNINITIALIZED,
  CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED,
  CHEESE_FACE_INFO_STATE_TRACKER_WAITING,
  CHEESE_FACE_INFO_STATE_TRACKER_TRACKS,
} CheeseFaceInfoState;

struct CheeseFace {
  private:
    cv::Ptr<cv::Tracker> _tracker;
    cv::Rect2d _bounding_box;
    cv::Rect2d _previous_bounding_box;
    guint _last_detected_frame;
    gboolean _previous_bounding_box_exists;
    CheeseFaceInfoState _state;

    void
    release_tracker ()
    {
      _tracker.release ();
      _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
    }

  public:
    /* TODO */
    /* Make these attributes private! */
    guint tracking_duration;

    std::vector<cv::Point> landmark;

    gpointer user_data;
    CheeseFaceFreeFunc free_user_data_func;

    CheeseFace ()
    {
      // tracking_duration = 0;
      user_data = NULL;
      free_user_data_func = NULL;
      _previous_bounding_box_exists = FALSE;
      _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
    }

    ~CheeseFace ()
    {
      cout << "FACE ABOUT BEING FREED" << endl;
      if (user_data && free_user_data_func)
        free_user_data_func (user_data);

      cout << "FACE FREED" << endl;
    }

    gboolean
    update_tracker (cv::Mat & frame)
    {
      gboolean target_found;
      cv::Rect2d tmp;

      if (_state == CHEESE_FACE_INFO_STATE_TRACKER_UNSET)
        return FALSE;
      /* Update tracker and swap previous and current bounding box if found. */
      tmp = _bounding_box;
      target_found = _tracker->update(frame, _bounding_box);
      if (target_found)
        _previous_bounding_box = tmp;
      else
        release_tracker ();
      return target_found;
    }

    cv::Point
    bounding_box_centroid ()
    {
      return (_bounding_box.br () + _bounding_box.tl ()) * 0.5;
    }

    cv::Point
    previous_bounding_box_centroid ()
    {
      return
          (_previous_bounding_box.br () + _previous_bounding_box.tl ()) * 0.5;
    }

    cv::Rect2d
    bounding_box ()
    {
      return _bounding_box;
    }

    guint
    last_detected_frame ()
    {
      return _last_detected_frame;
    }

    CheeseFaceInfoState
    state ()
    {
      return _state;
    }

    gboolean
    get_previous_bounding_box (cv::Rect2d & ret)
    {
      if (!_previous_bounding_box_exists)
        return FALSE;
      ret = _bounding_box;
      return TRUE;
    }

    void
    set_last_detected_frame (guint frame_number)
    {
      _last_detected_frame = frame_number;
    }

    void
    set_bounding_box (dlib::rectangle & rect)
    {
      _previous_bounding_box = _bounding_box;
      dlib_rectangle_to_cv_rect (rect, _bounding_box);
      _previous_bounding_box_exists = TRUE;
    }

    void
    create_tracker (GstCheeseFaceTrackTrackerType tracker_type)
    {
      _state = CHEESE_FACE_INFO_STATE_TRACKER_UNINITIALIZED;
      switch (tracker_type) {
        case GST_CHEESEFACETRACK_TRACKER_BOOSTING:
          _tracker = cv::TrackerBoosting::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_GOTURN:
          _tracker = cv::TrackerGOTURN::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_KCF:
          _tracker = cv::TrackerKCF::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_MEDIANFLOW:
          _tracker = cv::TrackerMedianFlow::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_MIL:
          _tracker = cv::TrackerMIL::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_TLD:
          _tracker = cv::TrackerTLD::create ();
          break;
        case GST_CHEESEFACETRACK_TRACKER_NONE:
          break;
        default:
          _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
          g_assert_not_reached ();
      }
    }

    void
    init_tracker (cv::Mat & img)
    {
      /* FIXME: Check if the tracker is set and init before doing this */
      if (_tracker->init (img, _bounding_box))
        _state = CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED;
    }
};

struct _GstCheeseFaceTrack
{
  GstOpencvVideoFilter element;

  /* props */
  gboolean display_bounding_box;
  gboolean display_id;
  gboolean display_landmark;
  gboolean display_pose_estimation;
  gchar *landmark;
  GstCheeseFaceTrackTrackerType tracker_type;
  guint tracker_duration;
  gboolean use_hungarian;
  gboolean use_pose_estimation;
  guint hungarian_delete_threshold;
  gfloat scale_factor;
  gdouble distance_factor;
  guint detection_gap_duration;

  /* private props */
  dlib::frontal_face_detector *face_detector;
  dlib::shape_predictor *shape_predictor;
  std::vector<cv::Point3d> *pose_model_points;

  guint last_face_id;
  guint frame_number;
  std::map<guint, CheeseFace> *faces;
  GHashTable *face_table;

  cv::Mat *camera_matrix;
  cv::Mat *dist_coeffs;
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
      { GST_CHEESEFACETRACK_TRACKER_NONE,  "No Tracker", "none" },
      { 0, NULL, NULL }
    };

    tracker_type = g_enum_register_static ("GstCheeseFaceTrackTrackerType",
        tracker_types);
  }
  return tracker_type;
}

#define DEFAULT_HUNGARIAN_DELETE_THRESHOLD                72
#define DEFAULT_SCALE_FACTOR                              1.0
#define DEFAULT_TRACKER                                   GST_CHEESEFACETRACK_TRACKER_NONE
#define DEFAULT_DETECTION_GAP_DURATION                    10
#define DEFAULT_DISTANCE_FACTOR                           0.18
#define DEFAULT_BOUNDING_BOX_DETECT_COLOR                 cv::Scalar (255, 255, 0)
#define DEFAULT_BOUNDING_BOX_TRACK_COLOR                  cv::Scalar (0, 255, 0)
#define DEFAULT_ID_COLOR                                  cv::Scalar (255, 0, 0)

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
  PROP_DISPLAY_POSE_ESTIMATION,
  PROP_LANDMARK,
  PROP_TRACKER,
  PROP_TRACKER_DURATION,
  PROP_USE_HUNGARIAN,
  PROP_HUNGARIAN_DELETE_THRESHOLD,
  PROP_USE_POSE_ESTIMATION,
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
  g_object_class_install_property (gobject_class, PROP_DISPLAY_POSE_ESTIMATION,
      g_param_spec_boolean ("display-pose-estimation", "Display the landmark",
          "Sets whether display the axis of the pose estimation for each face",
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
  /*
  g_object_class_install_property (gobject_class, PROP_TRACKER_DURATION,
      g_param_spec_uint ("tracker-duration", "Tracker duration",
          "Sets the maximum number of frames that the tracker will live "
          "before it is destroyed.",
          0, G_MAXUINT, DEFAULT_TRACKER_DURATION,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  */
  g_object_class_install_property (gobject_class, PROP_USE_HUNGARIAN,
      g_param_spec_boolean ("use-hungarian", "Display",
          "Sets whether to use the Hungarian algorithm to matching faces in "
          "the next frames.",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_HUNGARIAN_DELETE_THRESHOLD,
      g_param_spec_uint ("hungarian-delete-threshold",
          "Hungarian delete threshold",
          "Sets the number of frames that need to pass to delete a face if it "
          "has not been detected in that period",
          0, G_MAXUINT,
          DEFAULT_HUNGARIAN_DELETE_THRESHOLD,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_USE_POSE_ESTIMATION,
      g_param_spec_boolean ("use-pose-estimation", "Pose estimation",
          "Sets whether to use estimate the pose of each face.",
          TRUE, (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_SCALE_FACTOR,
      g_param_spec_float ("scale-factor", "Scale Factor",
          "Sets the scale factor the frame which be scaled with before face "
          "detection",
          0, G_MAXFLOAT,
          DEFAULT_SCALE_FACTOR,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class,
      PROP_DISTANCE_FACTOR,
      g_param_spec_float ("max-distance-factor", "Max distance factor",
          "Sets the maximum distance factor to calculate the maximum distance "
          "that will be applied between centroids of the tracker and detector. "
          "This factor is in relation to the bounding box width. The value of "
          "this factor should usually be very low.",
          0.0, 1.0, DEFAULT_DISTANCE_FACTOR,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
  g_object_class_install_property (gobject_class, PROP_DETECTION_GAP_DURATION,
      g_param_spec_uint ("detection-gap-duration", "Detection gap duration",
          "Sets the maximum number of frames between each detection phase.",
          1, G_MAXUINT, DEFAULT_DETECTION_GAP_DURATION,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));


  gst_element_class_set_details_simple(gstelement_class,
    "CheeseFaceTracker",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Fabian Orccon <<user@hostname.org>>");

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

  filter->use_hungarian = TRUE;
  filter->use_pose_estimation = TRUE;
  filter->hungarian_delete_threshold = DEFAULT_HUNGARIAN_DELETE_THRESHOLD;
  filter->display_bounding_box = TRUE;
  filter->display_id = TRUE;
  filter->display_pose_estimation = TRUE;
  filter->landmark = NULL;
  filter->tracker_type = DEFAULT_TRACKER;
  filter->detection_gap_duration = DEFAULT_DETECTION_GAP_DURATION;
  filter->face_detector =
      new frontal_face_detector (get_frontal_face_detector());
  filter->shape_predictor = NULL;
  filter->scale_factor = DEFAULT_SCALE_FACTOR;
  filter->distance_factor = DEFAULT_DISTANCE_FACTOR;

  filter->faces = new std::map<guint, CheeseFace>;

  filter->pose_model_points = new std::vector<cv::Point3d>;

  filter->camera_matrix = NULL;
  filter->dist_coeffs = NULL;
  /* Pose estimation 3D model */
  /* Nose tip */
  filter->pose_model_points->push_back (cv::Point3d (0.f, 0.f, 0.f));
  /* Chin */
  filter->pose_model_points->push_back (cv::Point3d (0.f, -330.f, -65.f));
  /* Left eye left corner */
  filter->pose_model_points->push_back (cv::Point3d (-225.f, 170.f, -135.f));
  /* Right eye right corner */
  filter->pose_model_points->push_back (cv::Point3d (225.f, 170.f, -135.f));
  /* Left Mouth corner */
  filter->pose_model_points->push_back (cv::Point3d (-150.f, -150.f, -125.f));
  /* Right mouth corner */
  filter->pose_model_points->push_back (cv::Point3d (150.f, -150.f, -125.f));

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
    case PROP_DISPLAY_POSE_ESTIMATION:
      filter->display_pose_estimation = g_value_get_boolean (value);
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
    case PROP_USE_HUNGARIAN:
      filter->use_hungarian = g_value_get_boolean (value);
      break;
    case PROP_HUNGARIAN_DELETE_THRESHOLD:
      filter->hungarian_delete_threshold = g_value_get_uint (value);
      break;
    case PROP_USE_POSE_ESTIMATION:
      filter->use_pose_estimation = g_value_get_boolean (value);
      break;
    case PROP_SCALE_FACTOR:
      filter->scale_factor = g_value_get_float (value);
      break;
    case PROP_DISTANCE_FACTOR:
      filter->distance_factor = g_value_get_float (value);
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
    case PROP_LANDMARK:
      g_value_set_string (value, filter->landmark);
      break;
    case PROP_TRACKER:
      g_value_set_enum (value, filter->tracker_type);
      break;
    case PROP_TRACKER_DURATION:
      g_value_set_uint (value, filter->tracker_duration);
      break;
    case PROP_DISPLAY_POSE_ESTIMATION:
      g_value_set_boolean (value, filter->display_pose_estimation);
      break;
    case PROP_USE_HUNGARIAN:
      g_value_set_boolean (value, filter->use_hungarian);
      break;
    case PROP_HUNGARIAN_DELETE_THRESHOLD:
      g_value_set_uint (value, filter->hungarian_delete_threshold);
      break;
    case PROP_USE_POSE_ESTIMATION:
      g_value_set_boolean (value, filter->use_pose_estimation);
      break;
    case PROP_SCALE_FACTOR:
      g_value_set_float (value, filter->scale_factor);
      break;
    case PROP_DISTANCE_FACTOR:
      g_value_set_float (value, filter->distance_factor);
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
  guint i;
  dets = (*filter->face_detector) (dlib_img);
  /*
  for (i = 0; filter->scale_factor != 1.0 && i < dets.size (); i++) {
    dets[i].set_left (dets[i].left () / filter->scale_factor);
    dets[i].set_top (dets[i].top () / filter->scale_factor);
    dets[i].set_right (dets[i].right () / filter->scale_factor);
    dets[i].set_bottom (dets[i].bottom () / filter->scale_factor);
  }
  */
}

std::vector<guint>
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
    (*filter->faces)[filter->last_face_id].create_tracker (filter->tracker_type);
    cout << "TRACKER CREATED" << endl;
    (*filter->faces)[filter->last_face_id].init_tracker (img);
    cout << "TRACKER INITIALIZED" << endl;

    created_faces_ids.push_back (filter->last_face_id);

    GST_LOG ("Face %d: this face has just been created.", filter->last_face_id);
  };
  return created_faces_ids;
}

std::vector<cv::Point>
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

static GstFlowReturn
gst_cheese_face_track_transform_ip (GstOpencvVideoFilter * base,
    GstBuffer * buf, IplImage * img)
{
  GstCheeseFaceTrack *filter = GST_CHEESEFACETRACK (base);

  /* Frame storage */
  cv::Mat cv_img (cv::cvarrToMat (img));
  cv::Mat cv_resized_img (cv::cvarrToMat (img));
  dlib::cv_image<bgr_pixel> dlib_resized_img;
  // cv::UMat cv_uimg;
  // cv_uimg = cv_img.getUMat (cv::ACCESS_WRITE);
  //std::vector<CheeseFace &> faces_with_lost_target;
  std::vector<guint> faces_ids_with_lost_target;

  gst_cheese_face_track_try_scale_image (filter, cv_img, cv_resized_img);
  dlib_resized_img = cv_image<bgr_pixel> (cv_resized_img);

  std::vector<guint> non_created_faces_ids;

  GST_DEBUG ("Frame number: %d.", filter->frame_number);
  
  //if (!filter->use_hungarian) {
    /* If we are not remapping faces by using the Hungarian Algorithm
     * so reset all the info to default values. */
    //filter->last_face_id = 0;
    //filter->faces->clear ();
  //}

  if (filter->use_hungarian) {
    guint i;
    std::vector<guint> to_remove;
    for (auto &kv : *filter->faces) {
      guint delta_since_detected;
      guint id = kv.first;
      CheeseFace &face = kv.second;

      delta_since_detected = filter->frame_number - face.last_detected_frame ();
      GST_LOG ("Face %d: number of frames passed since last detection of "
          "face is delta=%d.", id, delta_since_detected);

      if (delta_since_detected > filter->hungarian_delete_threshold) {
        GST_LOG ("Face %d will be deleted: "
            "delta=%d > hungarian-delete-threshold=%d.", id,
            delta_since_detected, filter->hungarian_delete_threshold);
        to_remove.push_back (id);
      }
    }

    for (i = 0; i < to_remove.size (); i++) {
      GST_LOG ("Face %d: about-removing face.", to_remove[i]);
      filter->faces->erase (to_remove[i]);
      GST_LOG ("Face %d: this face has just been deleted.", i);
    }
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
    std::vector<dlib::rectangle> resized_dets;

    if (gst_cheese_face_track_is_detection_phase (filter))
      GST_LOG ("Detection phase.");
    if (faces_ids_with_lost_target.size () > 0)
      GST_LOG ("Detection phase was forced because a tracker lost its target.");

    gst_cheese_face_track_detect_faces (filter, dlib_resized_img, resized_dets);

    /* Init faces, and thus create trackers */
    if (filter->faces->empty ())
      gst_cheese_face_track_create_faces (filter, cv_resized_img, resized_dets);

    for (int i = 0; i < resized_dets.size (); i++) {
      cv::Rect2d cvrect, ocvrect;
      cv::Point ocentroid;
      dlib_rectangle_to_cv_rect (resized_dets[i], cvrect);
      ocvrect = cv::Rect (
          cvrect.x / filter->scale_factor,
          cvrect.y / filter->scale_factor,
          cvrect.width / filter->scale_factor,
          cvrect.height / filter->scale_factor);

      ocentroid = (ocvrect.tl () + ocvrect.br ()) * 0.5;
      cv::putText (cv_img, std::to_string (i), ocentroid,
          cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar (0, 0, 255));
      cv::rectangle (cv_img, ocvrect, cv::Scalar (0, 0, 255), 5);
    }

    if (!non_created_faces_ids.empty () && resized_dets.size () > 0) {
      guint i, r, c;
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
        double max_dist;
        max_dist = filter->distance_factor * resized_dets[r].width ();
        for (c = 0; c < non_created_faces_ids.size (); c++) {
          const guint id = non_created_faces_ids[c];
          CheeseFace &face = (*filter->faces)[id];
          double dist;

          dist = cv::norm (cv::Mat (detection_centroids[r]),
              cv::Mat (face.bounding_box_centroid ()));

          cout << "Distance from face " << id << " to centroid " << r << ": " << dist << endl;

          /* FIXME */
          /* This should be deleted. We just should iterate after solving */
          /* the hungarian problem and update indices to -1 if distance > MAX */
          /*
          if (dist >= max_dist)
            dist = G_MAXDOUBLE;
          */

          row.push_back (dist);
        }
        cost_matrix.push_back (row);
      }

      /* Solve the Hungarian problem */
      GST_LOG ("Hungarian method: solve the Hungarian problem.");
      HungAlgo.Solve(cost_matrix, assignment);

      cout << "Assignments: " << endl;
      for (i = 0; i < cost_matrix.size (); i++) {
        if (assignment[i] != -1) {
          const gint id = non_created_faces_ids[assignment[i]];
          cv::Point cent = (*filter->faces)[id].bounding_box_centroid () / filter->scale_factor;
          double dist = cv::norm (cv::Mat (detection_centroids[i]),
              cv::Mat ((*filter->faces)[id].bounding_box_centroid ()));
          cout << i << " -> " << non_created_faces_ids[assignment[i]] <<
              ". Distance: " << dist << endl;
        } else {
          cout << i << " -> " << assignment[i] << endl;
        }
      }

      for (i = 0; i < cost_matrix.size (); i++) {
        if (assignment[i] == -1) {
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

  GST_LOG ("Prepare for drawing.");
  for (auto &kv : *filter->faces) {
    guint id = kv.first;
    CheeseFace &face = kv.second;
    if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED ||
        face.state () == CHEESE_FACE_INFO_STATE_TRACKER_WAITING) {
      cv::Rect2d resized_bounding_box, bounding_box;
      cv::Point centroid;

      resized_bounding_box = face.bounding_box ();
      bounding_box = cv::Rect (
          resized_bounding_box.x / filter->scale_factor,
          resized_bounding_box.y / filter->scale_factor,
          resized_bounding_box.width / filter->scale_factor,
          resized_bounding_box.height / filter->scale_factor);
      centroid = (bounding_box.tl () + bounding_box.br ()) * 0.5;

      if (gst_cheese_face_track_display_face (filter, face)) {
        GST_LOG ("Face %d: drawing bounding box. Position: (%d, %d)."
            "Size (w x h): %d x %d.", id,
            (int) bounding_box.x, (int) bounding_box.y,
            (int) bounding_box.width, (int) bounding_box.height);
        if (filter->display_bounding_box) {
          if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED)
            cv::rectangle (cv_img, bounding_box, DEFAULT_BOUNDING_BOX_DETECT_COLOR);
          else if (face.state () == CHEESE_FACE_INFO_STATE_TRACKER_WAITING)
            cv::rectangle (cv_img, bounding_box, DEFAULT_BOUNDING_BOX_TRACK_COLOR);
        }
        if (filter->display_id) {
          GST_LOG ("Face %d: drawing id number. Position: (%d, %d).", id,
              (int) centroid.x, (int) centroid.y);
          cv::putText (cv_img, std::to_string (id), centroid,
              cv::FONT_HERSHEY_SIMPLEX, 1.0, DEFAULT_ID_COLOR);
        }
      }
      /*
      GST_LOG ("Face %d: drawing bounding box. Position: (%d, %d)."
          "Size (w x h): %d x %d.", id,
          (int) bounding_box.x, (int) bounding_box.y,
          (int) bounding_box.width, (int) bounding_box.height);
      */
    }
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
  if (filter->camera_matrix)
    delete filter->camera_matrix;
  if (filter->dist_coeffs)
    delete filter->dist_coeffs;

  G_OBJECT_CLASS (gst_cheese_face_track_parent_class)->finalize (obj);
}