"""CompetitiveFair checks. One cost function. The 1100-point model is gone."""

from __future__ import annotations

ATTRS = (
    "chin",
    "body",
    "cut",
    "bruise",
    "guarding",
    "courage",
    "block_health",
    "block_regen",
    "reflexes",
    "footwork",
    "punch_speed",
    "accuracy",
    "stamina",
    "stamina_regen",
    "stamina_recovery",
    "conditioning",
    "endurance",
    "left_hand",
    "right_hand",
)

RATING_MIN = 50
RATING_MAX = 95
READY = 600
ALIASES = {"defence": "chin", "defense": "chin", "speed": "reflexes", "agility": "footwork"}

PRESETS = {
    "balanced": [80, 80, 70, 70, 80, 70, 75, 75, 80, 80, 80, 80, 80, 70, 70, 70, 80, 75, 75],
    "puncher": [85, 80, 70, 70, 70, 75, 70, 70, 70, 70, 80, 75, 70, 70, 70, 70, 70, 90, 95],
    "counter": [70, 70, 70, 70, 80, 70, 70, 75, 90, 85, 85, 90, 75, 70, 70, 70, 75, 70, 70],
    "pressure": [80, 80, 70, 70, 80, 70, 70, 70, 70, 70, 70, 70, 90, 70, 70, 80, 90, 80, 80],
}

DIVISION_CLASS = {
    "bantamweight": "lightweight",
    "featherweight": "lightweight",
    "lightweight": "lightweight",
    "welterweight": "middleweight",
    "middleweight": "middleweight",
    "light heavyweight": "middleweight",
    "cruiserweight": "heavyweight",
    "heavyweight": "heavyweight",
}

# Same 19-length lists as FFBFighterBalance::GetPremades. Presets above are creator shortcuts.
# High and low indexes are the ratings the selection screen treats as that style.
ROSTER = [
    ("Marcus Hale", "heavyweight", "Slugger", [88, 84, 70, 70, 66, 84, 74, 52, 66, 52, 55, 68, 84, 70, 76, 54, 76, 90, 95], [18, 17, 0], [9, 7]),
    ("Ivan Petrov", "cruiserweight", "In-fighter", [72, 88, 68, 70, 70, 84, 72, 68, 58, 56, 62, 60, 86, 70, 78, 92, 90, 74, 76], [15, 16, 1], [9, 8]),
    ("Leon Voss", "light heavyweight", "Counterpuncher", [73, 71, 69, 67, 86, 60, 79, 77, 93, 79, 82, 92, 55, 69, 67, 58, 56, 76, 80], [8, 11, 4], [12, 16]),
    ("Samir Quinn", "middleweight", "Boxer-puncher", [77, 75, 68, 70, 77, 74, 72, 64, 76, 79, 80, 84, 74, 68, 72, 66, 77, 86, 86], [17, 18, 11], [7, 15]),
    ("Diego Marte", "welterweight", "Swarmer", [54, 79, 68, 66, 70, 82, 70, 55, 70, 57, 92, 66, 88, 71, 77, 86, 90, 84, 60], [10, 16, 12], [0, 7]),
    ("Ellis Kane", "lightweight", "Out-boxer", [56, 58, 66, 64, 70, 69, 60, 72, 86, 94, 90, 91, 75, 83, 74, 66, 70, 81, 54], [9, 11, 10], [18, 0]),
    ("Nico Abebe", "featherweight", "High guard", [84, 76, 80, 74, 94, 78, 92, 90, 74, 54, 56, 72, 70, 65, 68, 67, 68, 62, 60], [4, 6, 7], [9, 10]),
    ("Rafi Solano", "bantamweight", "Southpaw", [59, 80, 61, 55, 70, 81, 66, 57, 82, 88, 79, 86, 77, 70, 68, 69, 72, 93, 81], [17, 9, 11], [3, 7]),
    ("Caleb Drummond", "heavyweight", "Brawler", [90, 90, 74, 74, 74, 93, 74, 73, 73, 51, 73, 52, 73, 73, 73, 73, 73, 73, 73], [0, 1, 5], [9, 11]),
    ("Idris Crowe", "cruiserweight", "Jab technician", [74, 51, 74, 74, 74, 73, 73, 73, 93, 73, 73, 90, 73, 73, 73, 52, 73, 90, 73], [17, 11, 8], [1, 15]),
    ("Mateo Ruiz", "light heavyweight", "Power puncher", [93, 74, 74, 74, 74, 73, 73, 73, 73, 52, 73, 73, 51, 73, 73, 73, 73, 90, 90], [18, 17, 0], [12, 9]),
    ("Andre Pell", "middleweight", "Body puncher", [51, 90, 74, 74, 74, 74, 73, 73, 52, 73, 73, 73, 73, 73, 73, 90, 93, 73, 73], [1, 15, 16], [0, 8]),
    ("Chris Adel", "welterweight", "Slick boxer", [74, 52, 74, 74, 74, 73, 73, 73, 90, 90, 73, 93, 73, 73, 73, 73, 73, 73, 51], [8, 9, 11], [18, 1]),
    ("Luis Ortega", "lightweight", "Inside fighter", [74, 90, 74, 74, 74, 73, 73, 73, 52, 51, 73, 73, 73, 73, 73, 90, 93, 73, 73], [15, 1, 16], [9, 8]),
    ("Kenji Sato", "featherweight", "Speedster", [51, 74, 74, 74, 74, 73, 73, 73, 93, 90, 90, 73, 73, 73, 73, 73, 73, 73, 52], [10, 9, 8], [0, 18]),
    ("Pavel Nemec", "bantamweight", "Peek-a-boo", [74, 74, 74, 74, 90, 93, 90, 73, 73, 51, 73, 52, 73, 73, 73, 73, 73, 73, 73], [4, 6, 5], [9, 11]),
]


def cost_of_rating(rating: int) -> int:
    if rating < RATING_MIN or rating > RATING_MAX:
        raise ValueError(rating)
    cost = 0
    for step in range(RATING_MIN + 1, rating + 1):
        if step <= 70:
            cost += 1
        elif step <= 80:
            cost += 2
        elif step <= 90:
            cost += 3
        else:
            cost += 5
    return cost


def cost_of_build(ratings: list[int]) -> int:
    if len(ratings) != 19:
        raise ValueError("count")
    return sum(cost_of_rating(rating) for rating in ratings)


def refunded(before: list[int], after: list[int]) -> int:
    return sum(max(0, cost_of_rating(a) - cost_of_rating(b)) for a, b in zip(before, after, strict=True))


def normalized(rating: int) -> float:
    return max(0.0, min(1.0, (rating - 50) / 45.0))


def migrate(payload: dict) -> dict:
    out = {}
    for key, value in payload.items():
        out[ALIASES.get(key, key)] = value
    if "strength" in out:
        raise ValueError("strength is not a combat multiplier")
    return out


def main() -> None:
    expected = {50: 0, 60: 10, 70: 20, 75: 30, 80: 40, 85: 55, 90: 70, 95: 95}
    for rating, cost in expected.items():
        assert cost_of_rating(rating) == cost, (rating, cost_of_rating(rating))
    for bad in (49, 96, -1, 1000):
        try:
            cost_of_rating(bad)
        except ValueError:
            pass
        else:
            raise SystemExit(f"accepted illegal rating {bad}")

    for name, ratings in PRESETS.items():
        spent = cost_of_build(ratings)
        assert spent == READY, f"{name} costs {spent}"

    assert cost_of_build([80] * 19) == 760
    base = [50] * 19
    assert cost_of_build(base) == 0
    assert refunded([80] * 19, [70] * 19) == 19 * (40 - 20)

    assert abs((1 + 0.20 * normalized(50)) - 1.0) < 1e-6
    assert abs((1 + 0.20 * normalized(95)) - 1.20) < 1e-6
    assert abs((1 - 0.15 * normalized(95)) - 0.85) < 1e-6
    assert abs((1 - 0.12 * normalized(95)) - 0.88) < 1e-6
    assert (1 - 0.15 * normalized(95)) > 0
    exposure = min(1.4 * 1.5, 1.60)
    assert exposure == 1.60
    assert 0 * exposure == 0

    migrated = migrate({"defence": 70, "speed": 80, "agility": 75})
    assert migrated == {"chin": 70, "reflexes": 80, "footwork": 75}
    try:
        migrate({"strength": 90, "left_hand": 80})
    except ValueError:
        pass
    else:
        raise SystemExit("strength was accepted as a second damage multiplier")

    plans_by_class: dict[str, set[str]] = {}
    per_division: dict[str, list[str]] = {}
    seen_ratings: set[tuple[int, ...]] = set()
    seen_styles: set[str] = set()
    for name, division, style, ratings, highs, lows in ROSTER:
        assert style not in seen_styles, style
        seen_styles.add(style)
        key = tuple(ratings)
        assert key not in seen_ratings, name
        seen_ratings.add(key)
        assert len(ratings) == 19, name
        assert all(RATING_MIN <= rating <= RATING_MAX for rating in ratings), name
        spent = cost_of_build(ratings)
        assert spent == READY, f"{name} costs {spent}"
        hi_vals = [ratings[index] for index in highs]
        lo_vals = [ratings[index] for index in lows]
        others_hi = [ratings[index] for index in range(19) if index not in highs]
        others_lo = [ratings[index] for index in range(19) if index not in lows]
        assert min(hi_vals) > max(others_hi), (name, hi_vals, max(others_hi))
        assert max(lo_vals) < min(others_lo), (name, lo_vals, min(others_lo))
        assert len(set(ratings)) > 4, name
        klass = DIVISION_CLASS[division]
        plans_by_class.setdefault(klass, set()).add(style)
        per_division.setdefault(division, []).append(style)
    for klass, plans in plans_by_class.items():
        assert len(plans) >= 2, klass
    assert len(per_division) == 8, per_division
    for division, styles in per_division.items():
        assert len(styles) == 2, (division, styles)
        assert len(set(styles)) == 2, (division, styles)

    print("CompetitiveFair: 19 ratings, budget 600, 4 creator presets, 16 styled division fighters, two in each division, one cost curve.")


if __name__ == "__main__":
    main()
