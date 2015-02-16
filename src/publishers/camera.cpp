/*
 * Copyright 2015 Aldebaran
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <alvision/alvisiondefinitions.h> // for kTop...
#include <alvision/alimage.h>
#include <alvalue/alvalue.h>
#include <alvision/alimage_opencv.h>

#include "camera.hpp"
#include "camera_info_definitions.hpp"

#include <ros/serialization.h>

namespace alros
{
namespace publisher
{

namespace camera_info_definitions
{

const sensor_msgs::CameraInfo& getCameraInfo( int camera_source, int resolution )
{
  /** RETURN VALUE OPTIMIZATION (RVO)
  * since there is no C++11 initializer list nor lamda functions
  */
  if ( camera_source == AL::kTopCamera)
  {
    if ( resolution == AL::kVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoTOPVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoTOPQVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoTOPQQVGA();
      return cam_info_msg;
    }
    else{
      std::cout << "no camera information found for camera_source " << camera_source << " and res: " << resolution << std::endl;
    }
  }
  else if ( camera_source == AL::kBottomCamera )
  {
    if ( resolution == AL::kVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoBOTTOMVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoBOTTOMQVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoBOTTOMQQVGA();
      return cam_info_msg;
    }
    else{
      std::cout << "no camera information found for camera_source " << camera_source << " and res: " << resolution << std::endl;
    }
  }
  else if ( camera_source == AL::kDepthCamera )
  {
    if ( resolution == AL::kVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoDEPTHVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoDEPTHQVGA();
      return cam_info_msg;
    }
    else if( resolution == AL::kQQVGA )
    {
      static const sensor_msgs::CameraInfo cam_info_msg = createCameraInfoDEPTHQQVGA();
      return cam_info_msg;
    }
    else{
      std::cout << "no camera information found for camera_source " << camera_source << " and res: " << resolution << std::endl;
    }
  }
}

} // camera_info_definitions

CameraPublisher::CameraPublisher( const std::string& name, const std::string& topic, float frequency, const qi::AnyObject& p_video, int camera_source, int resolution )
  : BasePublisher( name, topic, frequency ),
    p_video_( p_video ),
    camera_source_(camera_source),
    resolution_(resolution),
    // change in case of depth camera
    colorspace_( (camera_source_!=2)?AL::kBGRColorSpace:AL::kDepthColorSpace ),
    msg_colorspace_( (camera_source_!=2)?"bgr8":"mono16" ),
    cv_mat_type_( (camera_source_!=2)?CV_8UC3:CV_16U ),
    camera_info_( camera_info_definitions::getCameraInfo(camera_source, resolution) )
{
  if ( camera_source == AL::kTopCamera )
  {
    msg_frameid_ = "CameraTop_frame";
  }
  else if (camera_source == AL::kBottomCamera )
  {
    msg_frameid_ = "CameraBottom_frame";
  }
  else if (camera_source_ == AL::kDepthCamera )
  {
    msg_frameid_ = "CameraDepth_frame";
  }
}

CameraPublisher::~CameraPublisher()
{
  if (!handle_.empty())
  {
    p_video_.call<AL::ALValue>("unsubscribe", handle_);
    handle_.clear();
  }
}

void CameraPublisher::publish()
{
  // THIS WILL CRASH IN THE FUTURE
  AL::ALValue value = p_video_.call<AL::ALValue>("getImageRemote", handle_);
  if (!value.isArray())
  {
    std::cout << "Cannot retrieve image" << std::endl;
    return;
  }

  // Create a cv::Mat of the right dimensions
  cv::Mat img(value[1], value[0], cv_mat_type_, const_cast<void*>(value[6].GetBinary()));
  std_msgs::Header header;
  header.frame_id = msg_frameid_;
  header.stamp = ros::Time::now();
  cv_bridge::CvImage cv_img(header, msg_colorspace_, img);

  // Image transport would be cleaner here but would create another memory copy
  pub_image_.publish( cv_img );
  pub_camera_.publish( camera_info_ );
}

void CameraPublisher::reset( ros::NodeHandle& nh )
{
  if (!handle_.empty())
  {
    p_video_.call<AL::ALValue>("unsubscribe", handle_);
    handle_.clear();
  }

  handle_ = p_video_.call<std::string>(
                         "subscribeCamera",
                          name_,
                          camera_source_,
                          resolution_,
                          colorspace_,
                          frequency_
                          );

  pub_image_ = nh.advertise<sensor_msgs::Image>( topic_, 1 );
  pub_camera_ = nh.advertise<sensor_msgs::CameraInfo>( topic_ + "/camera_info", 1 );

  is_initialized_ = true;
}

} // publisher
} //alros
