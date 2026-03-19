from __future__ import annotations

from dataclasses import dataclass, asdict
from typing import List, Dict, Any


@dataclass
class Player:
    id: str
    name: str | None = None
    skill: float | None = None


def pairwise_matchmaking(players: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    """
    Simple matchmaking: pair players in order: (0,1), (2,3), ...
    Raises ValueError if number of players is odd.
    """
    if len(players) < 2:
        return []

    if len(players) % 2 != 0:
        raise ValueError("Must have an even number of players")

    matches = []
    for i in range(0, len(players), 2):
        matches.append({"player1": players[i], "player2": players[i + 1]})
    return matches