#ifndef DEFENDBEHAVIOR_H
#define DEFENDBEHAVIOR_H

#include <array>
#include "utilities/point.h"
#include "behavior/genericmovementbehavior.h"
#include "behavior/behavior.h"
#include "skill/kicktopointomni.h"

/* DEFEND BAHVIOR
 * DefendBehavior is the next iteration of a defense mode (circa May 2015).
 * It sets robots to sit at points around the goal, and get in the line of motion
 * of the ball and kick it back if it is coming torwards the goal.
 * This also serves as a first iteration of an "intelligent agents" type of behavior,
 * where each robot independently runs the same behavior to without the need for a
 * strategy. This is made up of `DefendStates` which link to one another.
 */
 
/************************************************************/
/* DefendState -- A base class, represents a state in the 
 * DefendBehavior state machine. States are changed by return a new
 * instance of one from `action`. Similar to the `perform` function
 */
class DefendState
{
public:
    virtual ~DefendState();

    /* Perform a state and return a new one
     * Returns `this` if no transition is made */
    virtual DefendState* action(Robot* robot);

    //Initializes all claimed points to initial positions
    static void setupClaimedPoints();

    //Clears all robot's claimed points, by setting `defendPoints`
    //back to the default positions and filling `claimed` with -1
    static void clearClaimedPoints();

    //Returns the point pointer this robot should go to
    static Point* getClaimedPoint(Robot*);

    //Loops through and looks for the point for this robot to claim
    static Point* findClaimPoint(Robot*);

protected:
    static int   whoIsKicking;     //Who is moving to kick the ball?

private:
    static int   claimed[10];      //Index of Points in `defendPoints` to idle at
    static Point defendPoints[];   //Points to sit robots at.
    static const Point defPoints[];//Default points to sit at
    static int   updateCount;      //Count to delay `action` updating of points
};

/************************************************************/
/* DefendState To choose a point to idle at and
 * go to kick the ball away if it is coming close
 */
class DefendStateIdle : public DefendState, public GenericMovementBehavior
{
public:
     DefendStateIdle();
     DefendState* action(Robot* robot) override;
};

/************************************************************/
/* DefendState that merely does KickToPointOmni to kick the ball away
 */
class DefendStateIdleKick: public DefendState
{
public:
    DefendStateIdleKick();
   ~DefendStateIdleKick();
    DefendState* action(Robot* robot) override;
private:
    Skill::KickToPointOmni* ktpo;
};

/************************************************************/
/* DefendState to kick the ball away and return to idle
 * if finished
 */
class DefendStateKick : public DefendState, public GenericMovementBehavior
{
public:
    DefendStateKick();
   ~DefendStateKick();
    DefendState* action(Robot* robot) override;
private:
    Skill::KickToPointOmni* ktpo;
    bool  chosenLinePoint;
    bool  kickingBall;
    int   kickBallTimeout;
    Point linePoint;
    bool  tryGetValidLinePoint(Robot*);
};

/************************************************************/


class DefendBehavior : public Behavior
{
public:
     DefendBehavior();
    ~DefendBehavior();
    void perform(Robot *) override;
private:
    static int currentUsers;    //# Robots currently using this Behavior
    DefendState* state;         //Current state
};

#endif // DEFENDBEHAVIOR_H
