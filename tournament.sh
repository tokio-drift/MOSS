#!/bin/bash

LOG_FILE="./logs/tournament.txt"
> "$LOG_FILE"

teams=(-IND -AUS -SRI -PAK -ENG -NZ -WI -SA -AFG)

RESET="\033[0m"
declare -A color
color[-IND]="\033[1;34m"   # blue
color[-PAK]="\033[1;32m"   # green
color[-AUS]="\033[1;33m"   # yellow
color[-ENG]="\033[1;35m"   # magenta
color[-NZ]="\033[1;36m"    # cyan
color[-SA]="\033[1;31m"    # red
color[-WI]="\033[1;91m"    # bright red
color[-AFG]="\033[1;92m"   # bright green
color[-SRI]="\033[1;93m"   # bright yellow

paint() {
    t=$1
    echo -e "${color[$t]}$t${RESET}"
}

# stats
declare -A points wins draws losses

for t in "${teams[@]}"; do
    points[$t]=0
    wins[$t]=0
    draws[$t]=0
    losses[$t]=0
done

# league phase
echo "=== LEAGUE STAGE START ===" | tee -a "$LOG_FILE"
n=${#teams[@]}
for ((i=0;i<n;i++)); do
    for ((j=i+1;j<n;j++)); do
        t1=${teams[$i]}
        t2=${teams[$j]}

        echo -e "Match: $(paint $t1) vs $(paint $t2)"
        echo "Match: $t1 vs $t2" >> "$LOG_FILE"

        ./stripped_moss -P "$t1" "$t2"
        res=$?

        if [[ $res -eq 10 ]]; then
            ((points[$t1]+=3)); ((wins[$t1]++)); ((losses[$t2]++))
            echo -e "Result: $(paint $t1) won"
            echo "Result: $t1 won" >> "$LOG_FILE"

        elif [[ $res -eq 11 ]]; then
            ((points[$t2]+=3)); ((wins[$t2]++)); ((losses[$t1]++))
            echo -e "Result: $(paint $t2) won"
            echo "Result: $t2 won" >> "$LOG_FILE"

        else
            ((points[$t1]+=1)); ((points[$t2]+=1))
            ((draws[$t1]++)); ((draws[$t2]++))
            echo "Result: Draw"
            echo "Result: Draw" >> "$LOG_FILE"
        fi

        echo "" >> "$LOG_FILE"
    done
done

echo "=== LEAGUE STAGE END ===" | tee -a "$LOG_FILE"

echo "=== POINTS TABLE ===" | tee -a "$LOG_FILE"

table=()
for t in "${teams[@]}"; do
    table+=("$t ${points[$t]} ${wins[$t]} ${draws[$t]} ${losses[$t]}")
done

sorted=$(printf "%s\n" "${table[@]}" | sort -k2 -nr)

echo "Team  Pts  W  D  L" | tee -a "$LOG_FILE"

while read -r line; do
    t=$(echo "$line" | awk '{print $1}')
    echo -e "$(paint $t) $(echo $line | cut -d' ' -f2-)"
    echo "$line" >> "$LOG_FILE"
done <<< "$sorted"

# Top 4
top4=($(printf "%s\n" "$sorted" | head -n 4 | awk '{print $1}'))

echo "=== SEMIFINALISTS ===" | tee -a "$LOG_FILE"
for t in "${top4[@]}"; do
    echo -e "$(paint $t)"
    echo "$t" >> "$LOG_FILE"
done

# Knockouts
play_match_no_draw() {
    t1=$1
    t2=$2

    while true; do
        ./stripped_moss -P "$t1" "$t2" 
        res=$?

        if [[ $res -eq 10 ]]; then
            winner="$t1"
            return
        elif [[ $res -eq 11 ]]; then
            winner="$t2"
            return
        fi
    done
}

# Semifinals
echo "=== SEMIFINALS ===" | tee -a "$LOG_FILE"

play_match_no_draw "${top4[0]}" "${top4[3]}"
sf1_winner=$winner
echo -e "SF1: $(paint ${top4[0]}) vs $(paint ${top4[3]}) → $(paint $sf1_winner)"
echo "SF1: ${top4[0]} vs ${top4[3]} → $sf1_winner" >> "$LOG_FILE"

play_match_no_draw "${top4[1]}" "${top4[2]}"
sf2_winner=$winner
echo -e "SF2: $(paint ${top4[1]}) vs $(paint ${top4[2]}) → $(paint $sf2_winner)"
echo "SF2: ${top4[1]} vs ${top4[2]} → $sf2_winner" >> "$LOG_FILE"

# Finals
echo "=== FINAL ===" | tee -a "$LOG_FILE"

play_match_no_draw "$sf1_winner" "$sf2_winner"
champion=$winner

echo -e "Final: $(paint $sf1_winner) vs $(paint $sf2_winner) → $(paint $champion)"
echo "Final: $sf1_winner vs $sf2_winner → $champion" >> "$LOG_FILE"

echo -e "\n🏆 TOURNAMENT WINNER: $(paint $champion)"
echo "=== TOURNAMENT WINNER: $champion ===" >> "$LOG_FILE"