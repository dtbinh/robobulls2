#ifndef ATTACK_SUPPORT_H
#define ATTACK_SUPPORT_H

#include "behavior.h"
#include "skill/kicktopoint.h"
#include "skill/gotopositionwithorientation.h"
#include "skill/obstacleavoidmove.h"
#include "utilities/measurments.h"
#include "utilities/region.h"
#include "utilities/paramlist.h"
#include "model/gamemodel.h"
#include "model/robot.h"
#include <stdlib.h>

//Used in the TwoVOne strategy to control the robot that moves into position to receive passes
//TwoVOne assigns this behavior to the robot farther from the ball
//The complementary behavior is AttackMain

class AttackSupport:public Behavior
{
public:
    AttackSupport(const ParameterList& list);
    void perform(Robot *);
private:
    const double shot_distance=1200;
    const double pass_distance=1200;

    #if SIMULATED==1
    Skill::ObstacleAvoidMove *move_skill;
    #else
    Skill::ObstacleAvoidMove *move_skill;
    #endif


    Skill::GoToPosition *wait_skill;

    enum states { move, wait, initial } state;
};

#endif // ATTACK_SUPPORT_H