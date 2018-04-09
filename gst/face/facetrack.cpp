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
#include <gst/cheese/face/cheesefaceinfo.h>
#include <gst/cheese/face/cheesemultifacemeta.h>
#include "utils.h"
#include "facetrack.h"

CheeseFace::CheeseFace ()
{
  user_data = NULL;
  free_user_data_func = NULL;
  _previous_bounding_box_exists = FALSE;
  _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
}

CheeseFace::~CheeseFace ()
{
  if (user_data && free_user_data_func)
    free_user_data_func (user_data);
}

GstCheeseFaceInfo *
CheeseFace::to_face_info_at_scale (gdouble scale_factor)
{
  GstCheeseFaceInfo *info;
  guint n_keypoints = CHEESE_FACE_LANDMARK_N (CHEESE_FACE_LANDMARK_TYPE_68);
  guint i;
  gfloat tl_x, tl_y, width, height;

  info = gst_cheese_face_info_new ();

  tl_x = _bounding_box.tl ().x * scale_factor;
  tl_y = _bounding_box.tl ().y * scale_factor;
  width = _bounding_box.width * scale_factor;
  height = _bounding_box.height * scale_factor;

  cheese_face_info_set_bounding_box (info,
      GRAPHENE_RECT_INIT (tl_x, tl_y, width, height));

  /* Only 68 landmarks supported now */
  if (_landmark.size () == n_keypoints) {
    graphene_point_t landmark_keypoints[n_keypoints];

    for (i = 0; i < _landmark.size (); i++) {
      float x, y;
      x = (float) _landmark[i].x * scale_factor;
      y = (float) _landmark[i].y * scale_factor;
      landmark_keypoints[i] = GRAPHENE_POINT_INIT (x, y);
    }
    cheese_face_info_set_landmark_keypoints (info, landmark_keypoints,
        n_keypoints);
  }
  return info;
}

gboolean
CheeseFace::update_tracker (cv::Mat & frame)
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
CheeseFace::bounding_box_centroid ()
{
  return (_bounding_box.br () + _bounding_box.tl ()) * 0.5;
}

cv::Point
CheeseFace::previous_bounding_box_centroid ()
{
  return
      (_previous_bounding_box.br () + _previous_bounding_box.tl ()) * 0.5;
}

cv::Rect2d
CheeseFace::bounding_box ()
{
  return _bounding_box;
}

guint
CheeseFace::last_detected_frame ()
{
  return _last_detected_frame;
}

CheeseFaceInfoState
CheeseFace::state ()
{
  return _state;
}

gboolean
CheeseFace::get_previous_bounding_box (cv::Rect2d & ret)
{
  if (!_previous_bounding_box_exists)
    return FALSE;
  ret = _bounding_box;
  return TRUE;
}

void
CheeseFace::set_last_detected_frame (guint frame_number)
{
  _last_detected_frame = frame_number;
}

void
CheeseFace::set_bounding_box (dlib::rectangle & rect)
{
  _previous_bounding_box = _bounding_box;
  dlib_rectangle_to_cv_rect (rect, _bounding_box);
  _previous_bounding_box_exists = TRUE;
}

void
CheeseFace::set_landmark (std::vector<cv::Point> & landmark)
{
  _landmark = landmark;
}

void
CheeseFace::create_tracker (GstCheeseFaceTrackTrackerType tracker_type)
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
    default:
      _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
      g_assert_not_reached ();
  }
}

void
CheeseFace::init_tracker (cv::Mat & img)
{
  /* FIXME: Check if the tracker is set and init before doing this */
  if (_tracker->init (img, _bounding_box))
    _state = CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED;
}

void
CheeseFace::release_tracker ()
{
  _tracker.release ();
  _state = CHEESE_FACE_INFO_STATE_TRACKER_UNSET;
}
