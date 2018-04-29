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
#ifndef __GSTCHEESEFACETRACK_INFO_H__
#define __GSTCHEESEFACETRACK_INFO_H__

#include <glib.h>
#include <dlib/opencv.h>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

G_BEGIN_DECLS

typedef enum {
  GST_CHEESEFACETRACK_TRACKER_BOOSTING,
  GST_CHEESEFACETRACK_TRACKER_GOTURN,
  GST_CHEESEFACETRACK_TRACKER_KCF,
  GST_CHEESEFACETRACK_TRACKER_MEDIANFLOW,
  GST_CHEESEFACETRACK_TRACKER_MIL,
  GST_CHEESEFACETRACK_TRACKER_TLD
} GstCheeseFaceTrackTrackerType;

typedef enum {
  CHEESE_FACE_INFO_STATE_TRACKER_UNSET,
  CHEESE_FACE_INFO_STATE_TRACKER_UNINITIALIZED,
  CHEESE_FACE_INFO_STATE_TRACKER_INITIALIZED,
  CHEESE_FACE_INFO_STATE_TRACKER_WAITING,
  CHEESE_FACE_INFO_STATE_TRACKER_TRACKS,
} CheeseFaceInfoState;

typedef void (* CheeseFaceFreeFunc) (gpointer);

struct CheeseFace {
  private:
    cv::Rect2d _bounding_box;
    cv::Rect2d _previous_bounding_box;
    guint _last_detected_frame;
    gboolean _previous_bounding_box_exists;
    CheeseFaceInfoState _state;
    std::vector<cv::Point> _landmark;

  public:
    /* TODO */
    /* Make these attributes private! */
    cv::Ptr<cv::Tracker> _tracker;
    guint tracking_duration;

    gpointer user_data;
    CheeseFaceFreeFunc free_user_data_func;

    CheeseFace ();
    ~CheeseFace ();
    GstCheeseFaceInfo * to_face_info_at_scale (gdouble scale_factor = 1.0);
    gboolean update_tracker (cv::Mat & frame);
    cv::Point bounding_box_centroid ();
    cv::Point previous_bounding_box_centroid ();
    cv::Rect2d bounding_box ();
    guint last_detected_frame ();
    CheeseFaceInfoState state ();
    gboolean get_previous_bounding_box (cv::Rect2d & ret);
    void set_last_detected_frame (guint frame_number);
    void set_bounding_box (dlib::rectangle & rect);
    void set_landmark (std::vector<cv::Point> & landmark);
    void create_tracker (GstCheeseFaceTrackTrackerType tracker_type);
    void init_tracker (cv::Mat & img);
    void release_tracker ();
};

G_END_DECLS

#endif /* __GSTCHEESEFACETRACK_INFO_H__ */

