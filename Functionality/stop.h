#ifndef STOP_H
#define STOP_H

#endif // STOP_H
#include <iostream>
#include "Performance/skill.h"
#include "communication/robcomm.h"

using namespace std;

class Stop:public Skill
{
public:
    Stop();
    void perform(Robot *);
};