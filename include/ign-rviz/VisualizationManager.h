//
// Created by sarath on 2/26/20.
//

#ifndef IGN_RVIZ_VISUALIZATIONMANAGER_H
#define IGN_RVIZ_VISUALIZATIONMANAGER_H

#include "ign-rviz/SceneManager.h"
#include "ign-rviz/GlutWindow.hh"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#elif not defined(_WIN32)
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#endif

#include <ros/ros.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <sensor_msgs/Imu.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

class VisualizationManager : public SceneManager {
 private:
  ros::Subscriber point_subscriber;
  ros::Subscriber pose_subscriber;
  ros::Subscriber orientation_subscriber;
  ros::NodeHandle nh;
  AxisVisualPtr axis;
 public:
  VisualizationManager(int &argc, char** argv);
  void point_callback(const geometry_msgs::PointStampedConstPtr&);
  void pose_callback(const geometry_msgs::PoseStampedConstPtr&);
  void orientation_callback(const sensor_msgs::ImuConstPtr&);
  void run();
};

#endif //IGN_RVIZ_VISUALIZATIONMANAGER_H