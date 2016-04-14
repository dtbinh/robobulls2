#ifndef DEFENDBEHAVIOR_H
#define DEFENDBEHAVIOR_H
#include "behavior/genericmovementbehavior.h"
#include "skill/kicktopointomni.h"

//! @file A general-purpose defence behavior for the game.
 
/************************************************************/
/*! @brief A base class, a state in the DefendBehavior state machine.
 * @author JamesW
 * @details States are changed by returning a new
 * instance of one from `action`. Similar to the `perform` function
 */
class DefendState
{
public:
    virtual ~DefendState();

    //! @brief Perform a state and return a new one Returns `this` if no transition is made */
    virtual DefendState* action(Robot* robot);

    //! @brief Initializes all claimed points to initial positions
    static void setupClaimedPoints();

    //! @brief Clears all robot's claimed points.
    //! @details by setting `defendPoints` back to the default
    //! positions and filling `claimed` with -1
    static void clearClaimedPoints();

protected:
    //Returns the point pointer this robot should go to, or nullptr if the
    //robot has none. Use searchClaimPoint to claim a point first.
    static Point* getClaimedPoint(Robot*);

    //Loops through, looks, and sets the point internally for this robot to claim
    static Point* searchClaimPoint(Robot*);

    static int   whoIsKicking;     //Who is moving to kick the ball?

private:
    static int   claimed[10];      //Index of Points in `defendPoints` to idle at
    static Point defendPoints[];   //Points to sit robots at.
    static const Point defPoints[];//Default points to sit at
    static int   updateCount;      //Count to delay `action` updating of points
};

//! @cond

/************************************************************/

/*! @brief DefendState To choose a point to idle at.
 * @details If activeKick is true, goes to kick the ball away if it is coming close.
 */
class DefendStateIdle : public DefendState, public StaticMovementBehavior
{
public:
     DefendStateIdle(bool activeKick = true);
     DefendState* action(Robot* robot) override;
private:
    bool activeKicking;
};


/************************************************************/

/*! @brief DefendState that merely does KickToPointOmni to kick the ball away.
 * @dtails Happens when the ball stops close to a robot on our side
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

/*! @brief DefendState to sit in the ball's incoming path.
 * @details Then, kick the ball away and return to idle when finished */
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

//! @endcond

/************************************************************/

/*! @brief DefendBehavior is the next iteration of a defense mode (circa May 2015).
 * @author JamesW
 * @details DefendBehavior sets robots to sit at points around the goal, and get in the
 * line of motion of the ball and kick it back if it is coming torwards the goal. In addition,
 * if the ball is stopped on our side a robot will break to kick it, and also
 * the robots sway formation to face the ball if it on our side.
 * This also serves as a first iteration of an "intelligent agents" type of behavior,
 * where each robot independently runs the same behavior to without the need for a
 * strategy. This is made up of `DefendStates` which link to one another. */

class DefendBehavior : public Behavior
{
public:
     DefendBehavior(bool activeKick = true);
    ~DefendBehavior();
    void perform(Robot *) override;
private:
    bool activeKicking;         //Do we move from the defence to kick away ball?
    static int currentUsers;    //Number of robots currently using this behavior
    DefendState* state;         //Current state (one of the above)
};

#endif // DEFENDBEHAVIOR_H
