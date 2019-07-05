#include <gtest/gtest.h>
#include <limits>
#include "avoidance/transform_buffer.h"

using namespace avoidance;

class TransformBufferTests : public ::testing::Test, public TransformBuffer {};

TEST(TransformBuffer, registerTransformAPI) {
  // GIVEN: a transform Buffer and source/target frames
  TransformBuffer tf_buffer(10);
  std::string source_frame = "frame1";
  std::string target_frame1 = "frame2";
  std::string target_frame2 = "frame3";

  // THEN: the buffer should not yet have this transform registered
  EXPECT_EQ(tf_buffer.registered_transforms_.size(), 0);

  // WHEN: we register that the transform and also try to register the same tf
  // twice
  tf_buffer.registerTransform(source_frame, target_frame1);
  tf_buffer.registerTransform(source_frame, target_frame2);
  tf_buffer.registerTransform(source_frame, target_frame2);

  // THEN: the buffer should have the transforms registered once each
  EXPECT_EQ(tf_buffer.registered_transforms_.size(), 2);
  EXPECT_EQ(tf_buffer.registered_transforms_[0].first, source_frame);
  EXPECT_EQ(tf_buffer.registered_transforms_[0].second, target_frame1);
  EXPECT_EQ(tf_buffer.registered_transforms_[1].first, source_frame);
  EXPECT_EQ(tf_buffer.registered_transforms_[1].second, target_frame2);
}

TEST(TransformBuffer, GetTransformAPI) {
  // GIVEN: a transform Buffer and source/target frames
  TransformBuffer tf_buffer(10);
  std::string source_frame = "frame1";
  std::string target_frame = "frame2";

  ros::Time::init();
  ros::Time time1 = ros::Time::now();
  ros::Time time2 = time1 - ros::Duration(2);
  ros::Time time3 = time1 - ros::Duration(4);
  ros::Time time_between = time1 - ros::Duration(1.5);
  ros::Time time_before = time1 - ros::Duration(6);
  ros::Time time_after = time1 + ros::Duration(1);

  tf::StampedTransform transform1, transform2, transform3, retrieved_transform;
  tf::Vector3 zero_translation = {0, 0, 0};
  transform1.stamp_ = time1;
  transform1.setIdentity();
  transform1.setOrigin(zero_translation);
  transform2.stamp_ = time2;
  transform2.setIdentity();
  transform2.setOrigin(zero_translation);
  transform3.stamp_ = time3;
  transform3.setIdentity();
  transform3.setOrigin(zero_translation);

  // WHEN: we register that transform and insert the 3 transforms into the
  // buffer
  tf_buffer.registerTransform(source_frame, target_frame);
  tf_buffer.insertTransform(source_frame, target_frame, transform3);
  tf_buffer.insertTransform(source_frame, target_frame, transform2);
  tf_buffer.insertTransform(source_frame, target_frame, transform1);

  // THEN: we should be able to retrieve transforms at different times

  // time1 should get transform1
  EXPECT_TRUE(tf_buffer.getTransform(source_frame, target_frame, time1,
                                     retrieved_transform));
  EXPECT_EQ(retrieved_transform.stamp_, time1);
  EXPECT_EQ(retrieved_transform, transform1);

  // time2 should get transform2
  EXPECT_TRUE(tf_buffer.getTransform(source_frame, target_frame, time2,
                                     retrieved_transform));
  EXPECT_EQ(retrieved_transform.stamp_, time2);
  EXPECT_EQ(retrieved_transform, transform2);

  // time3 should get transform3
  EXPECT_TRUE(tf_buffer.getTransform(source_frame, target_frame, time3,
                                     retrieved_transform));
  EXPECT_EQ(retrieved_transform.stamp_, time3);
  EXPECT_EQ(retrieved_transform, transform3);

  // time in between should give the timestamp of what we ask for
  EXPECT_TRUE(tf_buffer.getTransform(source_frame, target_frame, time_between,
                                     retrieved_transform));
  EXPECT_EQ(retrieved_transform.stamp_, time_between);

  // outside of the buffer should not give a transform
  EXPECT_FALSE(tf_buffer.getTransform(source_frame, target_frame, time_before,
                                      retrieved_transform));
  EXPECT_FALSE(tf_buffer.getTransform(source_frame, target_frame, time_after,
                                      retrieved_transform));
}

TEST_F(TransformBufferTests, insertTransform) {
  // GIVEN: a transform Buffer and source/target frames
  std::string source_frame = "frame1";
  std::string target_frame1 = "frame2";
  std::string target_frame2 = "frame3";
  std::string target_frame3 = "frame4";

  ros::Time::init();
  ros::Time time1 = ros::Time::now();
  ros::Time time2 = time1 - ros::Duration(2);
  ros::Time time3 = time1 - ros::Duration(4);

  tf::StampedTransform transform1, transform2, transform3;
  transform1.stamp_ = time1;
  transform1.setIdentity();
  transform1.setOrigin({0, 0, 0});
  transform2.stamp_ = time2;
  transform2.setIdentity();
  transform2.setOrigin({0, 0, 0});
  transform3.stamp_ = time3;
  transform3.setIdentity();
  transform3.setOrigin({0, 0, 0});

  // WHEN: we register that transform and insert transforms
  registerTransform(source_frame, target_frame1);
  registerTransform(source_frame, target_frame2);

  // THEN: the buffer should have the right transforms registered
  ASSERT_TRUE(isRegistered(source_frame, target_frame1));
  ASSERT_TRUE(isRegistered(source_frame, target_frame2));
  ASSERT_FALSE(isRegistered(source_frame, target_frame3));

  // AND: inserting transforms should work depending on timestamps
  EXPECT_TRUE(insertTransform(source_frame, target_frame1, transform3));
  EXPECT_TRUE(insertTransform(source_frame, target_frame1, transform2));
  EXPECT_FALSE(insertTransform(source_frame, target_frame1, transform2));
  EXPECT_TRUE(insertTransform(source_frame, target_frame1, transform1));
  EXPECT_FALSE(insertTransform(source_frame, target_frame1, transform1));
  EXPECT_FALSE(insertTransform(source_frame, target_frame1, transform2));

  EXPECT_TRUE(insertTransform(source_frame, target_frame2, transform3));
  EXPECT_TRUE(insertTransform(source_frame, target_frame2, transform2));

  EXPECT_FALSE(insertTransform(source_frame, target_frame3, transform2));

  // AND: the buffer should contain 3 transforms for the first target, and 2 for
  // the second
  EXPECT_EQ(buffer_[get_key(source_frame, target_frame1)].size(), 3);
  EXPECT_EQ(buffer_[get_key(source_frame, target_frame2)].size(), 2);
}

TEST_F(TransformBufferTests, interpolateTransform) {
  // GIVEN: two transforms with translations and rotations
  std::string source_frame = "frame1";
  std::string target_frame = "frame2";

  ros::Time::init();
  ros::Time time1 = ros::Time::now();
  ros::Time time_half = time1 + ros::Duration(1);
  ros::Time time2 = time1 + ros::Duration(2);

  tf::StampedTransform transform1, transform2, retrieved_transform1,
      retrieved_transform2, retrieved_transform3;
  transform1.stamp_ = time1;
  transform2.stamp_ = time2;
  tf::Vector3 translation1 = {0, 0, 0};
  tf::Vector3 translation2 = {0, 0, 2};
  tf::Vector3 translation_half = {0, 0, 1};
  tf::Quaternion rotation2, rotation_half;
  rotation2.setRPY(1, 0, 0);
  rotation_half.setRPY(0.5, 0, 0);

  transform1.setIdentity();
  transform2.setRotation(rotation2);
  transform1.setOrigin(translation1);
  transform2.setOrigin(translation2);

  // WHEN: we interpolate to a time exactly between the two given transforms
  retrieved_transform1.stamp_ = time_half;
  retrieved_transform2.stamp_ = time1;
  retrieved_transform3.stamp_ = time2;
  ASSERT_TRUE(
      interpolateTransform(transform1, transform2, retrieved_transform1));
  ASSERT_TRUE(
      interpolateTransform(transform1, transform2, retrieved_transform2));
  ASSERT_TRUE(
      interpolateTransform(transform1, transform2, retrieved_transform3));
  tf::Quaternion rotation_retrieved1 = retrieved_transform1.getRotation();

  // THEN: we should get half the translation and rotation for the time in
  // between
  EXPECT_EQ(retrieved_transform1.getOrigin(), translation_half);
  EXPECT_FLOAT_EQ(rotation_retrieved1.x(), rotation_half.x());
  EXPECT_FLOAT_EQ(rotation_retrieved1.y(), rotation_half.y());
  EXPECT_FLOAT_EQ(rotation_retrieved1.z(), rotation_half.z());
  EXPECT_FLOAT_EQ(rotation_retrieved1.w(), rotation_half.w());

  // AND: the two exact one for time 1 and time 2
  EXPECT_EQ(retrieved_transform2, transform1);
  EXPECT_EQ(retrieved_transform3, transform2);
}
