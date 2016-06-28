#ifndef NORMALGAMESTRATEGY_H
#define NORMALGAMESTRATEGY_H
#include "model/robot.h"
#include "strategy/strategy.h"
#include "utilities/region/defencearea.h"
#include "behavior/genericmovementbehavior.h"

/*! @brief This strategy is meant for the ' ' gamestate (space) which means
 * normal play.
 * @author JamesW, Muhaimen Shamsi
 */
class NormalGameStrategy : public Strategy
{
public:
    NormalGameStrategy();
    void assignBeh() override;
    bool update() override;

public:
    /*! @brief Assigns goalie behavior to ID GOALIE_ID if it is on the team
     * @details Meant to be used outside the class, if the robot with GOALIE_ID
     * isn't null, assigns Goalie to it */
    static void assignGoalieIfOk();

private:
    Point initialBallPos;
    DefenceArea our_def_area;
    DefenceArea opp_def_area;
    bool clearing_ball = false;

    // Used to switch when ball is closer to the other
    Robot* supp;
    Robot* main;

    enum {opp_kickoff, our_kickoff_1, our_kickoff_2, our_kickoff_3,
          evaluate, attack, defend, goalkick, clearball} state, prev_state;
};

#endif // NORMALGAMESTRATEGY_H
