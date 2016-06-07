#ifndef GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H
#define GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H

#include <boost/bind.hpp>
// #include <Eigen/Eigen>
#include <math.h>           // abs floor
#include <set>
#include <stdio.h>

#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
// #include <mav_msgs/eigen_mav_msgs.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <ros/callback_queue.h>
#include <ros/ros.h>
#include <std_msgs/ColorRGBA.h>
#include <sensor_msgs/LaserScan.h>
#include <tf/transform_listener.h> // getYaw createQuaternionMsgFromYaw 
#include <visualization_msgs/MarkerArray.h>
#include <visualization_msgs/Marker.h>

#include <octomap/octomap.h>
#include <octomap/OcTree.h>
#include <octomap_msgs/conversions.h>
#include <octomap_msgs/Octomap.h>

#include "avoidance/cell.h"
#include "avoidance/common.h"
#include "avoidance/global_planner.h"

namespace avoidance {

class GlobalPlannerNode {
 public:
  std::vector<Cell> fileGoals;
  GlobalPlannerNode();
  ~GlobalPlannerNode();

 private:
  GlobalPlanner global_planner;
  geometry_msgs::Point goalPoint;
  std::string namespace_;
  nav_msgs::Path actualPath;

  int numOctomapMessages = 0;
  int numPositionMessages = 0;



  // Subscribers
  ros::Subscriber cmd_waypoint_sub_;
  ros::Subscriber cmd_octomap_sub_;
  ros::Subscriber cmd_octomap_full_sub_;
  ros::Subscriber cmd_ground_truth_sub_;
  ros::Subscriber velocity_sub_;
  ros::Subscriber cmd_clicked_point_sub_;
  ros::Subscriber laser_sensor_sub_;

  // Publishers
  // ros::Publisher cmd_multi_dof_joint_trajectory_pub_;
  ros::Publisher cmd_global_path_pub_;
  ros::Publisher cmd_actual_path_pub_;
  ros::Publisher cmd_explored_cells_pub_;
  ros::Publisher cmd_clicked_point_pub_;

  // // lee_controler_publisher
  // ros::Publisher wp_pub;
  // // Mavros publisher
  // ros::Publisher mavros_waypoint_publisher;
  // // path_handler publisher
  // ros::Publisher path_handler_publisher;

  void SetNewGoal(Cell goal);
  void VelocityCallback(const geometry_msgs::TwistStamped& msg);
  void PositionCallback(const geometry_msgs::PoseStamped& msg);
  void ClickedPointCallback(const geometry_msgs::PointStamped& msg);
  void LaserSensorCallback(const sensor_msgs::LaserScan& msg);
  void OctomapCallback(const visualization_msgs::MarkerArray& msg);
  void OctomapFullCallback(const octomap_msgs::Octomap& msg);

  void PlanPath();

  void PublishPath();
  void PublishExploredCells();
};

} // namespace avoidance

#endif // GLOBAL_PLANNER_GLOBAL_PLANNER_NODE_H
