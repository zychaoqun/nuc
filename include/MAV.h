#ifndef _MAV_
#define _MAV_
#include "TooN/TooN.h"
#include "ros/ros.h"
#include "std_msgs/Bool.h"

class MAV
{
public:
    MAV();
    ~MAV(){}

    void Init(ros::NodeHandle* nh_, bool simulation_);
    void glDraw();
    void SetGoal(TooN::Vector<3> goalpos);
    void Update(double dt);
    bool AtGoal(){return atGoal;}
    void atGoalCallback(const std_msgs::Bool::Ptr &msg);
    void test();
    static void ChangeSpeed(double ds){speed +=ds;}
    static double speed;
private:
    TooN::Vector<3> pos;
    TooN::Vector<3> goal;
    TooN::Vector<3> toGoalNorm;
    bool atGoal;
    bool simulation;

    ros::NodeHandle * nh;
    ros::ServiceClient gotoPosService;
    ros::Subscriber atGoalSub;
};

#endif
