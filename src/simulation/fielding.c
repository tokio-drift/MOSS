#include <stdlib.h>
#include "../../include/types.h"

// Catch probabilities depends on fielding skill
bool attempt_catch(player *fielder, bool aerial)
{
    if (!aerial)
        return false;
    int skill = fielder->fielding_skill;
    // Strong scaling
    int prob = 30 + (skill * 6) / 10;  
    if (prob > 95) prob = 95;
    if (prob < 20) prob = 20;
    int r = rand() % 100;
    return (r < prob);
}