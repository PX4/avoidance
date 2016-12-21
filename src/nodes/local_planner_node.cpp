#include "local_planner_node.h"

LocalPlannerNode::LocalPlannerNode() {
   nh_ = ros::NodeHandle("~"); 

	pointcloud_sub_ = nh_.subscribe<sensor_msgs::PointCloud2>("/omi_cam/point_cloud", 1, &LocalPlannerNode::pointCloudCallback, this);
	pose_sub_ = nh_.subscribe<geometry_msgs::PoseStamped>("/mavros/local_position/pose", 1, &LocalPlannerNode::positionCallback, this);
	velocity_sub_ = nh_.subscribe("/mavros/local_position/velocity", 1, &LocalPlannerNode::velocityCallback, this);

	pose_array_pub_ = nh_.advertise<geometry_msgs::PoseArray>("/pose_array",1);
	local_pointcloud_pub_ = nh_.advertise<pcl::PointCloud<pcl::PointXYZ>>("/local_pointcloud", 1);
	path_candidates_pub_ = nh_.advertise<nav_msgs::GridCells>("/path_candidates", 1);
  path_rejected_pub_ = nh_.advertise<nav_msgs::GridCells>("/path_rejected", 1);
  path_blocked_pub_ = nh_.advertise<nav_msgs::GridCells>("/path_blocked", 1);
  path_selected_pub_ = nh_.advertise<nav_msgs::GridCells>("/path_selected", 1);
  marker_pub_ = nh_.advertise<visualization_msgs::MarkerArray>( "/visualization_marker", 1);

  waypoint_pub_ = nh_.advertise<nav_msgs::Path>("/waypoint", 1);
  path_pub_ = nh_.advertise<nav_msgs::Path>("/path_actual", 1);
  path_ideal_pub_ = nh_.advertise<nav_msgs::Path>("/path_ideal", 1);

  mavros_waypoint_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/mavros/setpoint_position/local", 10);
  current_waypoint_pub_ = nh_.advertise<geometry_msgs::PoseStamped>("/current_setpoint", 1);

  readParams();
  local_planner.setGoal();


}

LocalPlannerNode::~LocalPlannerNode(){}

void LocalPlannerNode::positionCallback(const geometry_msgs::PoseStamped msg){
	
  auto rot_msg = msg;
  tf_listener_.transformPose("world", ros::Time(0), msg, "local_origin", rot_msg);
	local_planner.setPose(rot_msg);
	fillPath(rot_msg);
}

void LocalPlannerNode::velocityCallback(const geometry_msgs::TwistStamped msg) {

  auto transformed_msg = avoidance::transformTwistMsg(tf_listener_, "/world", "/local_origin", msg); // 90 deg fix
  local_planner.curr_vel = transformed_msg;
}

void LocalPlannerNode::readParams() {

  nh_.param<double>("goal_x_param", local_planner.goal_x_param, 9);
  nh_.param<double>("goal_y_param", local_planner.goal_y_param, 13);
  nh_.param<double>("goal_z_param", local_planner.goal_z_param, 3.5);

}

void LocalPlannerNode::fillPath(const geometry_msgs::PoseStamped input) {

    path_actual.header.stamp = input.header.stamp;
    path_ideal.header.stamp = input.header.stamp;
    path_actual.header.frame_id = input.header.frame_id;
    path_ideal.header.frame_id = input.header.frame_id;
    path_actual.poses.push_back(input);
    
    geometry_msgs::PoseStamped traj ;
    traj.header = input.header;
    traj.pose.position.x = input.pose.position.x;
    traj.pose.position.y = local_planner.goal.y;
    traj.pose.position.z = local_planner.goal.z;
    traj.pose.orientation  = input.pose.orientation;
    path_ideal.poses.push_back(traj);

    pose_array.header.frame_id = input.header.frame_id;
    pose_array.header.stamp = input.header.stamp;
    pose_array.poses.push_back(input.pose);

}

void LocalPlannerNode::publishMarker() {

    visualization_msgs::Marker marker;
    marker.header.frame_id = "world";
    marker.header.stamp = ros::Time::now();
    marker.ns = "my_namespace";
    marker.id = i++;
    marker.type = visualization_msgs::Marker::ARROW;
    marker.action = visualization_msgs::Marker::ADD;
    marker.points.push_back(local_planner.pose.pose.position);

    geometry_msgs::Point p;
    p.x = local_planner.waypt.vector.x;
    p.y = local_planner.waypt.vector.y;
    p.z = local_planner.waypt.vector.z;
    marker.points.push_back(p);

    marker.scale.x = 0.1;
    marker.scale.y = 0.2;
    marker.scale.z = 0.2;
    marker.color.a = 1.0; // Don't forget to set the alpha!
    marker.color.r = 1.0;
    marker.color.g = 0.0;
    marker.color.b = 0.0;
    if(local_planner.first_brake) {
     	marker_array.markers.push_back(marker);
  	}
    marker_pub_.publish(marker_array);
}


void LocalPlannerNode::pointCloudCallback(const sensor_msgs::PointCloud2 input){

	pcl::PointCloud<pcl::PointXYZ> complete_cloud;
    sensor_msgs::PointCloud2 pc2cloud_world;

    try{
        tf_listener_.waitForTransform("/world", input.header.frame_id, input.header.stamp, ros::Duration(3.0));
        tf::StampedTransform transform;
        tf_listener_.lookupTransform("/world", input.header.frame_id, input.header.stamp, transform);
        pcl_ros::transformPointCloud("/world", transform, input, pc2cloud_world);
        pcl::fromROSMsg(pc2cloud_world, complete_cloud); 
        local_planner.filterPointCloud(complete_cloud);
    }
    catch(tf::TransformException& ex){
         ROS_ERROR("Received an exception trying to transform a point from \"camera_optical_frame\" to \"world\": %s", ex.what());
    }

    
   if(local_planner.obstacleAhead() && local_planner.init!=0) {
	    local_planner.createPolarHistogram();
	    local_planner.findFreeDirections();
	    local_planner.calculateCostMap();
      local_planner.getNextWaypoint();
      local_planner.getPathMsg();
	}
   else{
      local_planner.goFast();
      local_planner.getPathMsg();
       
   }

	publishAll();

    local_planner.cropPointCloud(); 


    if(local_planner.init == 0) { 
        ros::Duration(2).sleep();
        local_planner.init =1;
    }


}

void LocalPlannerNode::publishAll() {

	ROS_INFO("Current pose: [%f, %f, %f].",
           	local_planner.pose.pose.position.x,
           	local_planner.pose.pose.position.y,
			local_planner.pose.pose.position.z);

    


	local_pointcloud_pub_.publish(local_planner.final_cloud);
	path_candidates_pub_.publish(local_planner.Ppath_candidates);
  path_rejected_pub_.publish(local_planner.Ppath_rejected);
  path_blocked_pub_.publish(local_planner.Ppath_blocked);
	path_selected_pub_.publish(local_planner.Ppath_selected);
  waypoint_pub_.publish(local_planner.path_msg);
  path_pub_.publish(path_actual);
  path_ideal_pub_.publish(path_ideal);
   
	pose_array_pub_.publish(pose_array);

  current_waypoint_pub_.publish(local_planner.waypt_p);
  tf_listener_.transformPose("local_origin", ros::Time(0), local_planner.waypt_p, "world", local_planner.waypt_p);
  mavros_waypoint_pub_.publish(local_planner.waypt_p);

	publishMarker();

}


int main(int argc, char** argv) {
  ros::init(argc, argv, "local_planner_node");
  LocalPlannerNode local_planner_node;
  ros::Duration(2).sleep(); 
  ros::spin();
  
  return 0;
}


 	
