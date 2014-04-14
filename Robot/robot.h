#ifndef ROBOT_H
#define ROBOT_H

#include "Measure/point.h"
#include "Performance/behavior.h"

class Behavior;

class Robot
{
public:
    Robot();

    //sets
    void setRobotPosition(Point);
    void setOrientation(float);
    void setID(int);
    void setR(float);
    void setL(float);
    void setCurrentBeh(Behavior *);


    //gets
    Point getRobotPosition();
    int getOrientation();
    int getID();
    int getR();
    int getL();
    Behavior getCurrentBeh();


private:
    Point robotPosition;
    float orientation; //orientation of the robot
    int id;
    float L, R; // used for robot's movements
    Behavior *currentBehavior;

};

#endif // ROBOT_H
