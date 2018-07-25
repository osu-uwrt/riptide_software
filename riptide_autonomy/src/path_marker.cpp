#include "riptide_autonomy/path_marker.h"

#define ALIGN_CENTER 0
#define ALIGN_BBOX_WIDTH 1
#define ALIGN_OFFSET 2

PathMarker::PathMarker(BeAutonomous* master) {
  this->master = master;
  od = new ObjectDescriber(master);
  PathMarker::Initialize();
}

void PathMarker::Initialize() {
  detections = 0;
  attempts = 0;
  num_markers_dropped = 0;
  align_id = ALIGN_CENTER;
  
  detection_duration = 0;
  error_duration = 0;
  drop_duration = 0;
  drop_duration_thresh = 0;
  clock_is_ticking = false;
  drop_clock_is_ticking = false;

  for(int i=0; i< sizeof(active_subs)/sizeof(active_subs[0]); i++)
    active_subs[i]->shutdown();
}

void PathMarker::Start() {
  align_cmd.surge_active = false;
  align_cmd.sway_active = false;
  align_cmd.heave_active = false;
  align_cmd.object_name = master->object_names.at(0); // PathMarker
  align_cmd.alignment_plane = master->alignment_plane;
  align_cmd.bbox_dim = (int)(master->frame_height*0.7);
  align_cmd.bbox_control = rc::CONTROL_BBOX_HEIGHT;
  align_cmd.target_pos.x = 0;
  align_cmd.target_pos.y = 0;
  align_cmd.target_pos.z = 0;
  master->alignment_pub.publish(align_cmd);
  ROS_INFO("PathMarker: alignment command published (but disabled)");

  task_bbox_sub = master->nh.subscribe<darknet_ros_msgs::BoundingBoxes>("/task/bboxes", 1, &PathMarker::IDPathMarker, this);
}

// ID the PathMarker task
void PathMarker::IDPathMarker(const darknet_ros_msgs::BoundingBoxes::ConstPtr& bbox_msg) {
  // Get number of objects and make sure you have 'x' many within 't' seconds
  // Simply entering this callback signifies the object was detected (unless it was a false-positive)
  /*detections++;
  if(detections == 1) {
    detect_start = ros::Time::now();
    attempts++;
  }
  else {
    detection_duration = ros::Time::now().toSec() - detect_start.toSec();
  }

  if(detection_duration > master->detection_duration_thresh) {
    if(detections >= master->detections_req) {
      task_bbox_sub.shutdown();
      active_subs.erase(active_subs.end());
      master->tslam->Abort(true);
      clock_is_ticking = false;
      detection_duration = 0;

      // Send alignment command to put in center of frame (activate controllers)
      // Set points already specified in initial alignment command
      align_cmd.surge_active = true;
      align_cmd.sway_active = true;
      align_cmd.heave_active = false;
      master->alignment_pub.publish(align_cmd);
      alignment_status_sub = master->nh.subscribe<riptide_msgs::ControlStatusLinear>("/status/controls/linear", 1, &PathMarker::AlignmentStatusCB, this);
      active_subs.push_back(alignment_status_sub);
      ROS_INFO("PathMarker: Identified PathMarker. Now aligning to center");
    }
    else {
      ROS_INFO("PathMarker: Attempt %i to ID PathMarker", attempts);
      ROS_INFO("PathMarker: %i detections in %f sec", detections, detection_duration);
      ROS_INFO("PathMarker: Beginning attempt %i", attempts+1);
      detections = 0;
      detection_duration = 0;
    }/*
    else {
      ROS_INFO("PathMarker: More than 3 attempts used to ID PathMarker");
      ROS_INFO("PathMarker: Aborting");
      PathMarker::Abort();
    }*/

    od->GetPathHeading(&PathMarker::GotHeading, this);
  
}

// A. Make sure the vehicle is aligned to the center of the PathMarker wheel
// B. Make sure the vehicle is aligned to the ofset position so it can drop two markers
void PathMarker::AlignmentStatusCB(const riptide_msgs::ControlStatusLinear::ConstPtr& status_msg) {
  if(align_id == ALIGN_CENTER) { // Perform (A)
    if(abs(status_msg->x.error) < master->align_thresh && abs(status_msg->y.error) < master->align_thresh)
  	{
      if(!clock_is_ticking) {
        acceptable_begin = ros::Time::now();
        clock_is_ticking = true;
      }
      else
        error_duration = ros::Time::now().toSec() - acceptable_begin.toSec();

      if(error_duration >= master->error_duration_thresh) { // Roulete should be in the camera center
        alignment_status_sub.shutdown();
        error_duration = 0;
        clock_is_ticking = false;

    		// Calculate heading for PathMarker wheel
        //od->GetPathHeading(&PathMarker::SetMarkerDropHeading, this);
      }
  	}
    else {
      error_duration = 0;
      clock_is_ticking = false;
    }
  }
  else if(align_id == ALIGN_BBOX_WIDTH) { // Align with bbox
    if(abs(status_msg->z.error) < master->bbox_thresh)
  	{
      if(!clock_is_ticking) {
        acceptable_begin = ros::Time::now();
        clock_is_ticking = true;
      }
      else
        error_duration = ros::Time::now().toSec() - acceptable_begin.toSec();

      if(error_duration >= master->bbox_heave_duration_thresh) { // Roulete should be in the camera center
        alignment_status_sub.shutdown();
        error_duration = 0;
        clock_is_ticking = false;

    		// Calculate heading for PathMarker wheel
        //od->GetPathHeading(&PathMarker::SetMarkerDropHeading, this);
      }
  	}
    else {
      error_duration = 0;
      clock_is_ticking = false;
    }
  }
  else if(align_id == ALIGN_OFFSET) { // Perform (B)
    if(abs(status_msg->x.error) < master->align_thresh && abs(status_msg->y.error) < master->align_thresh)
  	{
      if(!clock_is_ticking) {
        acceptable_begin = ros::Time::now();
        clock_is_ticking = true;
      }
      else
        error_duration = ros::Time::now().toSec() - acceptable_begin.toSec();

      if(error_duration >= master->error_duration_thresh) { // Roulete should be off-center
        if(num_markers_dropped < 2) {
          pneumatics_cmd.header.stamp = ros::Time::now();
          pneumatics_cmd.torpedo_stbd = false;
          pneumatics_cmd.torpedo_port = false;
          pneumatics_cmd.markerdropper = true;
          pneumatics_cmd.manipulator = false;
          pneumatics_cmd.duration = 300; // [ms]

          if(!drop_clock_is_ticking || drop_duration > drop_duration_thresh) {
            drop_time = ros::Time::now();
            drop_clock_is_ticking = true;
            master->pneumatics_pub.publish(pneumatics_cmd);
            num_markers_dropped++;
          }
          else {
            drop_duration = ros::Time::now().toSec() - drop_time.toSec();
          }
        }
        else {
          pneumatics_cmd.markerdropper = false;;
          master->pneumatics_pub.publish(pneumatics_cmd);
          ROS_INFO("PathMarker is DONE!!!");
          PathMarker::Abort();
          master->StartTask();
        }
      }
  	}
    else {
      error_duration = 0;
      clock_is_ticking = false;
      drop_clock_is_ticking = false;
    }
  }
}

// When the green_heading has been found, set the robot to a heading normal to the green section
void PathMarker::GotHeading(double heading) {
  ROS_INFO("PathMarker angle: %f", heading);
  /*double offset = heading - 90;
  green_heading = master->euler_rpy.z + offset; // Center about current heading
  green_heading = master->tslam->KeepHeadingInRange(green_heading);

  // Calculate heading to drop markers - align to angle normal to green heading
  if(offset <= 90)
    marker_drop_heading = master->euler_rpy.z + 90;
  if(offset > 90)
    marker_drop_heading = master->euler_rpy.z - 90;

  marker_drop_heading = master->tslam->KeepHeadingInRange(marker_drop_heading);

  ROS_INFO("PathMarker: Green heading is %f", green_heading);
  ROS_INFO("PathMarker: Marker Drop Heading is %f", marker_drop_heading);

  // Publish attitude command
  attitude_cmd.roll_active = true;
  attitude_cmd.pitch_active = true;
  attitude_cmd.yaw_active = true;
  attitude_cmd.euler_rpy.x = 0;
  attitude_cmd.euler_rpy.y = 0;
  attitude_cmd.euler_rpy.z = marker_drop_heading;
  master->attitude_pub.publish(attitude_cmd);
  ROS_INFO("PathMarker: Published marker drop attitude");

  attitude_status_sub = master->nh.subscribe<riptide_msgs::ControlStatusAngular>("/status/controls/angular", 1, &PathMarker::AttitudeStatusCB, this);
  active_subs.push_back(attitude_status_sub);*/
}

// Make sure the robot goes to the marker drop heading
void PathMarker::AttitudeStatusCB(const riptide_msgs::ControlStatusAngular::ConstPtr& status_msg) {
	// Depth is good, now verify heading error

	if(abs(status_msg->yaw.error) < master->yaw_thresh)
	{
    if(!clock_is_ticking) {
      acceptable_begin = ros::Time::now();
      clock_is_ticking = true;
    }
    else
      error_duration = ros::Time::now().toSec() - acceptable_begin.toSec();

    if(error_duration >= master->error_duration_thresh) {
      attitude_status_sub.shutdown();
      error_duration = 0;
      clock_is_ticking = false;

  		// Now align to a bit left of the PathMarker center
      align_cmd.target_pos.x = 0;
      align_cmd.target_pos.y = -(master->frame_width/6);
      align_cmd.target_pos.z = 0;
      master->alignment_pub.publish(align_cmd);
      align_id = ALIGN_OFFSET;
      alignment_status_sub = master->nh.subscribe<riptide_msgs::ControlStatusLinear>("/status/controls/linear", 1, &PathMarker::AlignmentStatusCB, this);
      ROS_INFO("PathMarker: Identified PathMarker. Now aligning to off-center");
    }
	}
  else {
    error_duration = 0;
    clock_is_ticking = false;
  }
}

// Shutdown all active subscribers
void PathMarker::Abort() {
  PathMarker::Initialize();
  
  align_cmd.surge_active = false;
  align_cmd.sway_active = false;
  align_cmd.heave_active = false;
  master->alignment_pub.publish(align_cmd);
  ROS_INFO("PathMarker: Aborting");
}