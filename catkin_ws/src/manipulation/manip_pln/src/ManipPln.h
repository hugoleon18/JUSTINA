#pragma once
#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include "ros/ros.h"
#include "std_msgs/Bool.h"
#include "std_msgs/Float32MultiArray.h"
#include "std_msgs/String.h"

class ManipPln
{
public:
    ManipPln();
    ~ManipPln();

private:
    ros::NodeHandle* nh;
    //Publishers for indicating that a goal pose has been reached
    ros::Publisher pubLaGoalReached;
    ros::Publisher pubRaGoalReached;
    ros::Publisher pubHdGoalReached;
    //Subscribers for the commands executed by this node
    ros::Subscriber subLaGoToAngles;
    ros::Subscriber subRaGoToAngles;
    ros::Subscriber subHdGoToAngles;
    ros::Subscriber subLaGoToPoseWrtArm;
    ros::Subscriber subRaGoToPoseWrtArm;
    ros::Subscriber subLaGoToPoseWrtRobot;
    ros::Subscriber subRaGoToPoseWrtRobot;
    ros::Subscriber subLaGoToLoc;
    ros::Subscriber subRaGoToLoc;
    ros::Subscriber subHdGoToLoc;
    ros::Subscriber subLaMove;
    ros::Subscriber subRaMove;
    ros::Subscriber subHdMove;
    //Publishers and subscribers for operating the hardware nodes
    ros::Subscriber subLaCurrentPose;
    ros::Subscriber subRaCurrentPose;
    ros::Subscriber subHdCurrentPose;
    ros::Publisher pubLaGoalPose;
    ros::Publisher pubRaGoalPose;
    ros::Publisher pubHdGoalPose;
    ros::Publisher pubLaGoalTorque;
    ros::Publisher pubRaGoalTorque;
    ros::Publisher pubHdGoalTorque;

    bool laNewGoal;
    bool raNewGoal;
    bool hdNewGoal;
    std::vector<float> laCurrentPose;
    std::vector<float> raCurrentPose;
    std::vector<float> hdCurrentPose;
    std::vector<float> laGoalPose;
    std::vector<float> raGoalPose;
    std::vector<float> hdGoalPose;
    std::map<std::string, std::vector<float> > laPredefPoses;
    std::map<std::string, std::vector<float> > raPredefPoses;
    std::map<std::string, std::vector<float> > hdPredefPoses;
    std::map<std::string, std::vector<std::vector<float> > > laPredefMoves;
    std::map<std::string, std::vector<std::vector<float> > > raPredefMoves;
    std::map<std::string, std::vector<std::vector<float> > > hdPredefMoves;

public:
    void setNodeHandle(ros::NodeHandle* n);
    bool loadKnownPosesAndMovs(std::string directory);
    void spin();

private:
    float calculateError(std::vector<float>& v1, std::vector<float>& v2);
    //Callback for subscribers for the commands executed by this node
    void callbackLaGoToAngles(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackRaGoToAngles(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackHdGoToAngles(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackLaGoToPoseWrtArm(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackRaGoToPoseWrtArm(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackLaGoToPoseWrtRobot(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackRaGoToPoseWrtRobot(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackLaGoToLoc(const std_msgs::String::ConstPtr& msg);
    void callbackRaGoToLoc(const std_msgs::String::ConstPtr& msg);
    void callbackHdGoToLoc(const std_msgs::String::ConstPtr& msg);
    void callbackLaMove(const std_msgs::String::ConstPtr& msg);
    void callbackRaMove(const std_msgs::String::ConstPtr& msg);
    void callbackHdMove(const std_msgs::String::ConstPtr& msg);
    //Callback for subscribers for operating the hardware nodes
    void callbackLaCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackRaCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg);
    void callbackHdCurrentPose(const std_msgs::Float32MultiArray::ConstPtr& msg);
};
