//
// Created by Sarathkrishnan Ramesh on 2/26/20.
//

#include "ign-rviz/VisualizationManager.h"

VisualizationManager::VisualizationManager(int &argc, char **argv) : tfListener(tfBuffer) {
  glutInit(&argc, argv);
  common::Console::SetVerbosity(4);

  ros::NodeHandle private_nh("~");

  std::string point_topic = "/point";
  std::string pose_topic = "/pose";
  std::string imu_topic = "/imu";
  std::string marker_topic = "/visualization_marker";
  std::string tf_topic = "/tf_static";
  std::string pointcloud_topic = "/cloud_in";

  private_nh.getParam("/ign_rviz/point_topic", point_topic);
  private_nh.getParam("/ign_rviz/pose_topic", pose_topic);
  private_nh.getParam("/ign_rviz/imu_topic", imu_topic);
  private_nh.getParam("/ign_rviz/marker_topic", marker_topic);
  private_nh.getParam("/ign_rviz/tf_topic", tf_topic);
  private_nh.getParam("/ign_rviz/pointcloud_topic", pointcloud_topic);


  point_subscriber = nh.subscribe(point_topic, 1, &VisualizationManager::point_callback, this);
  pose_subscriber = nh.subscribe(pose_topic, 1, &VisualizationManager::pose_callback, this);
  orientation_subscriber = nh.subscribe(imu_topic, 100, &VisualizationManager::orientation_callback, this);
  marker_subscriber = nh.subscribe(marker_topic, 100, &VisualizationManager::marker_callback, this);
  tf_subscriber = nh.subscribe(tf_topic, 100, &VisualizationManager::tf_callback, this);
  pointcloud_subscriber = nh.subscribe(pointcloud_topic, 1, &VisualizationManager::cloud_callback, this);

  ScenePtr scene = get_scene();
  VisualPtr root = scene->RootVisual();
  axis = scene->CreateAxisVisual("axis");
  axis->SetLocalPosition(0, 0, 0);
  root->AddChild(axis);

  MaterialPtr color = scene->CreateMaterial("red");
  color->SetAmbient(1, 0, 0);
  color->SetDiffuse(1, 0, 0);

  pcl_marker = scene->CreateMarker();
  pcl_marker->SetType(MarkerType::MT_POINTS);

  VisualPtr pcl_visual = scene->CreateVisual("pcl");
  pcl_visual->AddGeometry(pcl_marker);
  root->AddChild(pcl_visual);
}

void VisualizationManager::point_callback(const geometry_msgs::PointStampedConstPtr& msg) {
  float x = msg->point.x;
  float y = msg->point.y;
  float z = msg->point.z;

  ROS_DEBUG("[ Point Received ]\tX:%f, Y:%f,  Z:%f", x, y, z);

  ScenePtr scene = get_scene();

  // Create material
  MaterialPtr color = scene->CreateMaterial();
  color->SetAmbient(0, 1, 1);
  color->SetDiffuse(0, 1, 1);
  color->SetSpecular(0.5, 0.5, 0.5);
  color->SetShininess(50);
  color->SetReflectivity(0);

  // Create visual
  VisualPtr marker = scene->CreateVisual();
  marker->AddGeometry(get_geometry((int)IGN_GEOMETRY::BOX));
  marker->SetMaterial(color);
  marker->SetLocalPosition(x, y, z);
  marker->SetWorldScale(0.1);

  VisualPtr root = scene->RootVisual();
  root->AddChild(marker);
}

void VisualizationManager::pose_callback(const geometry_msgs::PoseStampedConstPtr &msg) {
  ScenePtr scene = get_scene();
  VisualPtr root = scene->RootVisual();

  ROS_DEBUG("[ Pose Received ]");

  ArrowVisualPtr pose_arrow = scene->CreateArrowVisual();

  MaterialPtr color = scene->CreateMaterial();
  color->SetAmbient(1, 0, 0);
  color->SetDiffuse(1, 0, 0);

  tf2::Quaternion quat_orig, quat_new;
  tf2::convert(msg->pose.orientation, quat_orig);
  quat_new.setRPY(0, M_PI / 2, 0);
  quat_new = quat_orig * quat_new;
  quat_new.normalize();

  pose_arrow->SetLocalPosition(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
  pose_arrow->SetMaterial(color);

  pose_arrow->SetLocalRotation(quat_new.w(), quat_new.x(), quat_new.y(), quat_new.z());

  root->AddChild(pose_arrow);
}

void VisualizationManager::orientation_callback(const sensor_msgs::ImuConstPtr &msg) {
  axis->SetWorldScale(2);
  axis->SetLocalRotation(msg->orientation.w, msg->orientation.x, msg->orientation.y, msg->orientation.z);
  ROS_DEBUG("[Orientation Received]");
}

void VisualizationManager::marker_callback(const visualization_msgs::MarkerConstPtr &msg) {
  ScenePtr scene = get_scene();
  MarkerPtr marker = scene->CreateMarker();

  bool add_marker = false;
  std::string unique_name = msg->ns + std::to_string(msg->id);
  VisualPtr vis = scene->VisualByName(unique_name);

  // TODO: Implement DELETEALL markers action and fix delete arrow bug

  // Delete marker
  if (msg->action == visualization_msgs::Marker::DELETE) {
    if(vis != nullptr)
      vis->RemoveGeometries();
    return;
  }

  // Add marker
  if (vis == nullptr) {
    if (msg->type == visualization_msgs::Marker::ARROW) {
      vis = scene->CreateArrowVisual(unique_name);
    }
    else
      vis = scene->CreateVisual(unique_name);
    add_marker = true;
  }
  else
    vis->RemoveGeometries();

  // TODO: Implement visualization for Triangle, Cube & Sphere List, Text Message and Mesh Resource marker type

  if (msg->type == visualization_msgs::Marker::LINE_LIST || msg->type == visualization_msgs::Marker::LINE_STRIP
      || msg->type == visualization_msgs::Marker::POINTS || msg->type == visualization_msgs::Marker::CUBE_LIST
      || msg->type == visualization_msgs::Marker::SPHERE_LIST) {

    for (const auto &point : msg->points)
      marker->AddPoint(point.x, point.y, point.z, math::Color::White);

    if (msg->type == visualization_msgs::Marker::LINE_STRIP)
      marker->SetType(MarkerType::MT_LINE_STRIP);
    else if (msg->type == visualization_msgs::Marker::LINE_LIST)
      marker->SetType(MarkerType::MT_LINE_LIST);
    else
      marker->SetType(MarkerType::MT_POINTS);

  } else if (msg->type == visualization_msgs::Marker::SPHERE) {
    marker->AddPoint(0, 0, 0, math::Color::White);
    marker->SetType(MarkerType::MT_SPHERE);
  } else if (msg->type == visualization_msgs::Marker::CUBE) {
    marker->AddPoint(0, 0, 0, math::Color::White);
    marker->SetType(MarkerType::MT_BOX);
  } else if (msg->type == visualization_msgs::Marker::CYLINDER) {
    marker->AddPoint(0, 0, 0, math::Color::White);
    marker->SetType(MarkerType::MT_CYLINDER);
  }

  MaterialPtr color = scene->CreateMaterial();
  if (msg->type == visualization_msgs::Marker::ARROW) {
    color->SetAmbient(1,0,0,1);
    color->SetDiffuse(1,0,0,1);
    tf2::Quaternion quat_orig, quat_new;
    tf2::convert(msg->pose.orientation, quat_orig);
    quat_new.setRPY(0, M_PI / 2, 0);
    quat_new = quat_orig * quat_new;
    quat_new.normalize();
    vis->SetLocalRotation(quat_new.w(), quat_new.x(), quat_new.y(), quat_new.z());
  } else {
    color->SetAmbient(msg->color.r, msg->color.g, msg->color.b, msg->color.a);
    color->SetDiffuse(msg->color.r, msg->color.g, msg->color.b, msg->color.a);
    vis->AddGeometry(marker);
    vis->SetLocalRotation(1, 0, 0, 0);
  }

  vis->SetLocalPosition(msg->pose.position.x, msg->pose.position.y, msg->pose.position.z);
  vis->SetMaterial(color);

  VisualPtr root = scene->RootVisual();

  if(add_marker)
    root->AddChild(vis);
}

tf2::Quaternion get_rotation_to(const math::Vector3d &source, const math::Vector3d &destination) {
  const math::Vector3d& fallbackAxis = math::Vector3d::Zero;
  tf2::Quaternion q;

  math::Vector3d v0 = source;
  math::Vector3d v1 = destination;
  v0.Normalize();
  v1.Normalize();

  double d = v0.Dot(v1);

  if(d >= 1.0f) {
    return q;
  }

  if(d < (1e-6f - 1.0f)) {
    if(fallbackAxis != math::Vector3d::Zero) {
      tf2::Vector3 temp(fallbackAxis.X(), fallbackAxis.Y(), fallbackAxis.Z());
      q.setRotation(temp, M_PI);
    }
    else {
      math::Vector3d axis = math::Vector3d::UnitX.Cross(source);
      if(axis.Length() == 0) {
        axis = math::Vector3d::UnitY.Cross(source);
      }
      axis.Normalize();
      tf2::Vector3 temp(axis.X(), axis.Y(), axis.Z());
      q.setRotation(temp, M_PI);
    }
  } else {
    double s = sqrt((1 + d) * 2);
    double inverse = 1 / s;

    math::Vector3d c = v0.Cross(v1);

    q.setX(c.X() * inverse);
    q.setY(c.Y() * inverse);
    q.setZ(c.Z() * inverse);
    q.setW(s * 0.5f);
    q.normalize();
  }
  return q;
}



void VisualizationManager::create_tf_visual(std::unordered_map<std::string,
                                                               std::vector<geometry_msgs::TransformStamped>> &tf_tree,
                                            std::string &base_frame_id,
                                            MarkerPtr &marker,
                                            math::Vector3f pose) {

  ScenePtr scene = get_scene();
  VisualPtr tf_visual = scene->VisualByName("tf_visual");
  auto parent = tf_tree[base_frame_id];
  for (auto child : parent) {
    marker->AddPoint(pose.X(), pose.Y(), pose.Z(), math::Color(1,0,0,1));
    float x = pose.X() + child.transform.translation.x;
    float y = pose.Y() + child.transform.translation.y;
    float z = pose.Z() + child.transform.translation.z;
    marker->AddPoint(x, y, z, math::Color::White);

    AxisVisualPtr frame_axis = scene->CreateAxisVisual();
    frame_axis->SetLocalPosition(x, y, z);
    frame_axis->SetLocalScale(0.3);
    frame_axis->SetLocalRotation(child.transform.rotation.w,
                           child.transform.rotation.x,
                           child.transform.rotation.y,
                           child.transform.rotation.z);

    float d1 = -x + pose.X();
    float d2 = -y + pose.Y();
    float d3 = -z + pose.Z();

    math::Vector3d dir_vec(d1, d2, d3);
    dir_vec = dir_vec.Normalize();

    float dist = sqrt(pow(d1, 2) + pow(d2, 2) + pow(d3, 2));

    if(dist != 0) {
      VisualPtr cone = scene->CreateVisual();
      cone->AddGeometry(scene->CreateCone());

      tf2::Quaternion ori = get_rotation_to(-math::Vector3d::UnitZ, dir_vec);
      tf2::Quaternion quat;
      quat.setRPY(M_PI,0, 0);
      ori = ori * quat;

      cone->SetLocalScale(0.02, 0.02, 0.04);
      cone->SetLocalPosition(pose.X() - (0.02 * (d1/dist)), pose.Y() - (0.02 * (d2/dist)), pose.Z() - (0.02 * (d3/dist)));
      cone->SetLocalRotation(ori.w(), ori.x(), ori.y(), ori.z());

      MaterialPtr material = scene->CreateMaterial();
      material->SetAmbient(1,0,1, 0.85);
      material->SetDiffuse(1,0,1, 0.85);

      cone->SetMaterial(material);
      tf_visual->AddChild(cone);
    }

    TextPtr link_name = scene->CreateText();
    link_name->SetTextString(child.child_frame_id.c_str());
    link_name->SetShowOnTop(true);
    link_name->SetTextAlignment(TextHorizontalAlign::CENTER, TextVerticalAlign::CENTER);
    link_name->SetCharHeight(0.15);

    MaterialPtr text_color = scene->CreateMaterial();
    text_color->SetDiffuse(1, 1, 1);

    VisualPtr text_visual = scene->CreateVisual();

    text_visual->AddGeometry(link_name);
    text_visual->SetLocalPosition(x, y, z);

    text_visual->SetMaterial(text_color);
    tf_visual->AddChild(text_visual);

    tf_visual->AddChild(frame_axis);

    create_tf_visual(tf_tree, child.child_frame_id, marker, math::Vector3f(x, y, z));
  }
}

void VisualizationManager::tf_callback(const tf2_msgs::TFMessageConstPtr &msg) {
  ScenePtr scene = get_scene();
  MarkerPtr marker = scene->CreateMarker();
  marker->SetType(MarkerType::MT_LINE_LIST);

  std::string base_frame_id = "base_link";

  std::unordered_map<std::string, std::vector<geometry_msgs::TransformStamped>> tf_tree;

  for(const auto& transform : msg->transforms) {
    std::string parent = transform.header.frame_id;
    tf_tree[parent].push_back(transform);
  }

  VisualPtr tf_visual = scene->VisualByName("tf_visual");
  bool add_marker = false;
  if(tf_visual == nullptr) {
    tf_visual = scene->CreateVisual("tf_visual");
    add_marker = true;
  }

  create_tf_visual(tf_tree, base_frame_id, marker, math::Vector3f(0, 0, 0));

  TextPtr link_name = scene->CreateText();
  link_name->SetTextString(base_frame_id);
  link_name->SetShowOnTop(true);
  link_name->SetTextAlignment(TextHorizontalAlign::CENTER, TextVerticalAlign::CENTER);
  link_name->SetCharHeight(0.15);

  MaterialPtr text_color = scene->CreateMaterial();
  text_color->SetDiffuse(1, 1, 1);

  VisualPtr text_visual = scene->CreateVisual();

  text_visual->AddGeometry(link_name);
  text_visual->SetLocalPosition(0, 0, 0);

  text_visual->SetMaterial(text_color);
  tf_visual->AddChild(text_visual);

  MaterialPtr color = scene->CreateMaterial();
  color->SetAmbient(1,1,0, 0.8);

  tf_visual->AddGeometry(marker);
  tf_visual->SetGeometryMaterial(color, false);

  if(add_marker) {
    VisualPtr root = scene->RootVisual();
    root->AddChild(tf_visual);
  }
}

void VisualizationManager::cloud_callback(const sensor_msgs::PointCloud2ConstPtr& msg) {
  pcl::PointCloud<pcl::PointXYZ> laser_cloud_in, laser_cloud_out;
  pcl::fromROSMsg(*msg, laser_cloud_in);

  std::vector<int> index;
  pcl::removeNaNFromPointCloud(laser_cloud_in, laser_cloud_out, index);

  pcl_marker->ClearPoints();

  for(int i = 0; i < static_cast<int>(laser_cloud_out.size()); ++i) {
    pcl_marker->AddPoint(laser_cloud_out.points[i].x, laser_cloud_out.points[i].y, laser_cloud_out.points[i].z, math::Color::Red);
  }

  ScenePtr scene = get_scene();
  VisualPtr pcl_visual = scene->VisualByName("pcl");
  pcl_visual->SetGeometryMaterial(scene->Material("red"));

  try {
    geometry_msgs::TransformStamped transformStamped = tfBuffer.lookupTransform(
        "base_link", msg->header.frame_id.c_str(), msg->header.stamp);

    pcl_visual->SetLocalPosition(transformStamped.transform.translation.x,
                                 transformStamped.transform.translation.y,
                                 transformStamped.transform.translation.z);

    pcl_visual->SetLocalRotation(transformStamped.transform.rotation.w,
                                 transformStamped.transform.rotation.x,
                                 transformStamped.transform.rotation.y,
                                 transformStamped.transform.rotation.z);
  } catch(tf2::TransformException &ex) {
    ROS_ERROR("%s",ex.what());
  }
}

void VisualizationManager::run() {
  ::run(cameras, nodes);
}
