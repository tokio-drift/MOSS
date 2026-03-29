#include <stdlib.h>
#include "../../include/types.h"

bool attempt_catch(player *fielder, bool aerial)
{
    if (!aerial) return false;

    int skill = fielder->fielding_skill;
    int prob  = 20 + (skill * 45) / 100;
    if (prob > 75) prob = 75;
    if (prob < 20) prob = 20;

    return (rand() % 100 < prob);
}