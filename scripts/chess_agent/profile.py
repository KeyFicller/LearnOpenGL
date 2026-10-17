"""Model fingerprints, rebuilt from the match logs.

Every number here is recomputed from .output/chess/*.jsonl and *.truth.jsonl,
so the output can be deleted and rebuilt at any time. Run it with
`python scripts/chess_agent/profile.py`; it writes
.output/chess/profiles/<provider:model>.md and .json, plus an index.md that
puts the models side by side.

The .jsonl is the agent's account: one event per line, each with at / kind /
side / text. The .truth.jsonl is the engine's: what every move really moved and
captured, including covered pieces, which neither player is ever told. Counts
of rules come from the former, style from the latter.
"""

import collections
import glob
import json
import os
import re
import statistics
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LOG_DIR = os.path.join(ROOT, ".output", "chess")
OUT_DIR = os.path.join(LOG_DIR, "profiles")

# A refusal that means the move broke a rule. Missing note, a human's turn or a
# network game are protocol or setup problems, so they are counted on their own
# line and never inside this total.
RULE_REASONS = (
    "geometry",
    "facing",
    "undeclared_check",
    "empty",
    "not_your_piece",
    "off_board",
)
# Shown in the opponent table for a side the agent did not drive.
HUMAN = "人类"
# Refusals that mean the model misread the board rather than misjudged a piece.
READ_REASONS = ("empty", "not_your_piece", "off_board")

# Logs written before the engine returned a reason code have to be classified
# from the prose. These phrases come from explain_failure; a log that needed
# this is reported as legacy so a mixed corpus is visible.
LEGACY_REASONS = (
    ("is empty.", "empty"),
    ("which is not yours.", "not_your_piece"),
    ("same open line", "facing"),
    ("needs a check you gave", "undeclared_check"),
    ("Square is off the board", "off_board"),
    ("Illegal:", "geometry"),
)


def load(path):
    records = []
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            try:
                records.append(json.loads(line))
            except json.JSONDecodeError:
                continue
    return records


def stamp(path):
    return os.path.basename(path).split(".")[0]


def labels_of(records):
    """provider:model for each side, out of the session record.

    Older logs have no session labels, so the status line they printed at start
    is parsed instead. Both players are created once per process, so the answer
    is stable for a whole file even when it holds several matches. A side the
    agent did not drive is reported as None: the engine's truth file still has
    those moves, but they are not the model's play.
    """
    for record in records:
        if record.get("kind") == "session" and record.get("labels"):
            labels = dict(record["labels"])
            ai = record.get("ai") or {"red": True, "black": True}
            for side in ("red", "black"):
                if not ai.get(side, True):
                    labels[side] = None
            return labels, record.get("thinking"), record.get("seconds")
    for record in records:
        if record.get("kind") != "status":
            continue
        match = re.match(r"red (\S+)\s+vs\s+black (\S+?),?\s+thinking (\w+)", record.get("text", ""))
        if match:
            return (
                {"red": match.group(1), "black": match.group(2)},
                match.group(3) == "on",
                None,
            )
    return None, None, None


def reason_of(record):
    """Engine reason code, or a guess from the prose for pre-code logs."""
    if record.get("reason"):
        return record["reason"], False
    text = record.get("text", "")
    for needle, reason in LEGACY_REASONS:
        if needle in text:
            return reason, True
    return "other", False


def parse_tool(text):
    name, _, rest = text.partition(" ")
    try:
        args = json.loads(rest) if rest else {}
    except json.JSONDecodeError:
        args = {}
    return name, args if isinstance(args, dict) else {}


def percentile(values, fraction):
    if not values:
        return 0.0
    ordered = sorted(values)
    index = min(len(ordered) - 1, max(0, int(round(fraction * len(ordered))) - 1))
    return ordered[index]


def empty_side():
    return {
        "moves": 0,
        "errors": collections.Counter(),
        "legacy_errors": 0,
        "retry_same": 0,
        "timeouts": 0,
        "forfeits": 0,
        "quota_turns": {True: 0, False: 0},
        "move_calls": 0,
        "durations": {True: [], False: []},
        "tools_per_turn": {True: [], False: []},
        "redundant_board": {True: 0, False: 0},
        "turns_by_thinking": {True: 0, False: 0},
        "notes": [],
        "note_missing": 0,
        "asides": [],
        "reacts": [],
        "checks_received": 0,
        "plies": 0,
        "covered_moves": 0,
        "captures": 0,
        "covered_captures": 0,
        "king_moves": 0,
        "lost_next": 0,
    }


def collect(log_records, truth_records, labels, thinking, seconds, stats):
    """Add one log file's numbers to the per-model totals."""
    # A side may be human; then nothing is attributed to a model.
    model = {side: (labels or {}).get(side) for side in ("red", "black")}

    turn_start = {"red": None, "black": None}
    turn_tools = {"red": [], "black": []}
    last_play = {"red": None, "black": None}
    failed_play = {"red": None, "black": None}
    pending_note = {"red": "", "black": ""}
    side_of = {side: model[side] for side in ("red", "black")}

    for record in log_records:
        kind = record.get("kind")
        side = record.get("side") or ""
        if kind == "turn":
            turn_start[side] = record.get("at")
            turn_tools[side] = []
            last_play[side] = None
            failed_play[side] = None
        elif kind == "tool":
            name, args = parse_tool(record.get("text", ""))
            if not side_of.get(side):
                continue
            if name in ("board", "moves", "clock", "play"):
                turn_tools[side].append(name)
                if name == "moves":
                    stats[side_of[side]]["move_calls"] += 1
                if name == "play":
                    key = (args.get("from_row"), args.get("from_col"),
                           args.get("to_row"), args.get("to_col"))
                    if key == failed_play[side]:
                        stats[side_of[side]]["retry_same"] += 1
                        failed_play[side] = None
                    last_play[side] = key
        elif kind == "error":
            if not side_of.get(side):
                continue
            reason, legacy = reason_of(record)
            entry = stats[side_of[side]]
            entry["errors"][reason] += 1
            if reason == "note_missing":
                entry["note_missing"] += 1
            if legacy:
                entry["legacy_errors"] += 1
            if last_play[side] is not None:
                failed_play[side] = last_play[side]
                last_play[side] = None
        elif kind == "timeout":
            if side_of.get(side):
                entry = stats[side_of[side]]
                if "判负" in record.get("text", ""):
                    entry["forfeits"] += 1
                else:
                    entry["timeouts"] += 1
        elif kind in ("digest", "aside", "react"):
            if not side_of.get(side):
                continue
            entry = stats[side_of[side]]
            text = record.get("text", "")
            if kind == "digest":
                # The note belongs to a play call that may still be refused, so
                # it is held until a move record proves the call landed.
                pending_note[side] = text
            elif kind == "aside":
                entry["asides"].append(len(text))
            else:
                entry["reacts"].append(len(text))
        elif kind == "check":
            # Emitted to the side whose king is attacked, before it moves.
            if side_of.get(side):
                stats[side_of[side]]["checks_received"] += 1
        elif kind == "move":
            if not side_of.get(side):
                continue
            entry = stats[side_of[side]]
            entry["moves"] += 1
            if pending_note[side]:
                entry["notes"].append(pending_note[side])
                pending_note[side] = ""
            bucket = bool(thinking) if thinking is not None else True
            if turn_start[side] is not None:
                entry["durations"][bucket].append(record.get("at", 0) - turn_start[side])
            entry["turns_by_thinking"][bucket] += len(turn_tools[side]) > 0
            entry["tools_per_turn"][bucket].append(len(turn_tools[side]))
            entry["redundant_board"][bucket] += max(0, turn_tools[side].count("board") - 1)
            if turn_tools[side].count("moves") >= 4:
                entry["quota_turns"][bucket] += 1

    # Truth: what the engine actually moved. Style only, never shown to players.
    for record in truth_records:
        side = record.get("side")
        key = side_of.get(side)
        if not key:
            continue
        entry = stats[key]
        mover = record.get("mover") or {}
        target = record.get("target")
        entry["plies"] += 1
        if mover.get("covered"):
            entry["covered_moves"] += 1
        if mover.get("piece") == "king":
            entry["king_moves"] += 1
        if target:
            entry["captures"] += 1
            if target.get("covered"):
                entry["covered_captures"] += 1

    # Squares the engine saw this side vacate and the opponent take next ply.
    for index, record in enumerate(truth_records[:-1]):
        key = side_of.get(record.get("side"))
        if not key:
            continue
        after = truth_records[index + 1]
        if (
            after.get("to_row") == record.get("to_row")
            and after.get("to_col") == record.get("to_col")
            and after.get("side") != record.get("side")
        ):
            stats[key]["lost_next"] += 1


def ratio(part, whole):
    return "{:.0f}%".format(100.0 * part / whole) if whole else "—"


def per100(part, whole):
    return "{:.1f}".format(100.0 * part / whole) if whole else "—"


def median(values):
    return statistics.median(values) if values else 0.0


def side_table(entry):
    """The rows of a single model's fingerprint."""
    violations = sum(entry["errors"][r] for r in RULE_REASONS)
    misread = sum(entry["errors"][r] for r in READ_REASONS)
    notes = entry["notes"]
    duplicates = len(notes) - len(set(notes))
    lines = []
    lines += [
        "## 合规",
        "",
        "| 指标 | 原始 | 每 100 手 |",
        "| --- | --- | --- |",
        "| 落子 | {} | — |".format(entry["moves"]),
        "| 违规合计 | {} | {} |".format(violations, per100(violations, entry["moves"])),
        "| 走法不合法 | {} | {} |".format(
            entry["errors"]["geometry"], per100(entry["errors"]["geometry"], entry["moves"])
        ),
        "| 造成照面 | {} | {} |".format(
            entry["errors"]["facing"], per100(entry["errors"]["facing"], entry["moves"])
        ),
        "| 未将军就吃将 | {} | {} |".format(
            entry["errors"]["undeclared_check"],
            per100(entry["errors"]["undeclared_check"], entry["moves"]),
        ),
        "| 读盘错误 | {} | {} |".format(misread, per100(misread, entry["moves"])),
        "| — 源格为空 | {} | {} |".format(
            entry["errors"]["empty"], per100(entry["errors"]["empty"], entry["moves"])
        ),
        "| — 动对手的子 | {} | {} |".format(
            entry["errors"]["not_your_piece"],
            per100(entry["errors"]["not_your_piece"], entry["moves"]),
        ),
        "| — 越界 | {} | {} |".format(
            entry["errors"]["off_board"], per100(entry["errors"]["off_board"], entry["moves"])
        ),
        "| 同一手重发 | {} | {} |".format(
            entry["retry_same"], per100(entry["retry_same"], entry["moves"])
        ),
        "",
        "| 另计（不算违规） | 次数 |",
        "| --- | --- |",
        "| 缺 note | {} |".format(entry["note_missing"]),
        "| 超时重试 | {} |".format(entry["timeouts"]),
        "| 超时判负 | {} |".format(entry["forfeits"]),
        "| 其他拒绝 | {} |".format(
            sum(entry["errors"][r] for r in entry["errors"] if r not in RULE_REASONS)
        ),
        "",
    ]
    if entry["legacy_errors"]:
        lines += [
            "这一栏里有 {} 次来自没有 reason 码的旧日志，按错误文案归类。".format(
                entry["legacy_errors"]
            ),
            "",
        ]
    lines += [
        "## 节奏",
        "",
        "| 思考 | 手数 | 耗时中位 | 耗时 p90 | 工具调用/手 | 重复读盘 | 用满 moves 配额 |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    for bucket, name in ((True, "开"), (False, "关")):
        durations = entry["durations"][bucket]
        tools = entry["tools_per_turn"][bucket]
        turns = entry["turns_by_thinking"][bucket]
        if not turns:
            lines.append("| {} | 0 | — | — | — | — | — |".format(name))
            continue
        lines.append(
            "| {} | {} | {:.1f} 秒 | {:.1f} 秒 | {:.1f} | {} | {} |".format(
                name,
                turns,
                median(durations),
                percentile(durations, 0.9),
                median(tools),
                ratio(entry["redundant_board"][bucket], turns),
                ratio(entry["quota_turns"][bucket], turns),
            )
        )
    lines += [""]

    lines += [
        "## 表达",
        "",
        "| 指标 | 值 |",
        "| --- | --- |",
        "| note 数 | {} |".format(len(notes)),
        "| note 长度中位 | {:.0f} 字 |".format(median([len(n) for n in notes])),
        "| note 完全相同 | {} 次（{}） |".format(duplicates, ratio(duplicates, len(notes))),
        "| 内心长度中位 | {:.0f} 字 |".format(median(entry["asides"])),
        "| 反应长度中位 | {:.0f} 字 |".format(median(entry["reacts"])),
        "",
    ]

    lines += [
        "## 棋风",
        "",
        "| 指标 | 值 |",
        "| --- | --- |",
        "| 翻暗子 | {} |".format(ratio(entry["covered_moves"], entry["plies"])),
        "| 吃子 | {} |".format(ratio(entry["captures"], entry["plies"])),
        "| 吃到的子中暗子占比 | {} |".format(
            ratio(entry["covered_captures"], entry["captures"])
        ),
        "| 动将 | {} |".format(ratio(entry["king_moves"], entry["plies"])),
        "| 走完就被吃 | {} |".format(ratio(entry["lost_next"], entry["plies"])),
        "| 被将军 | {} 次 |".format(entry["checks_received"]),
        "",
    ]
    return lines


def main():
    jsonls = sorted(
        path
        for path in glob.glob(os.path.join(LOG_DIR, "*.jsonl"))
        if not path.endswith(".truth.jsonl")
    )
    truths = sorted(glob.glob(os.path.join(LOG_DIR, "*.truth.jsonl")))
    if not jsonls:
        print("no logs in {}".format(os.path.relpath(LOG_DIR, ROOT)))
        return 1

    stats = collections.defaultdict(empty_side)
    matches = collections.defaultdict(lambda: {"wins": 0, "losses": 0, "self_play": 0, "plies": 0})
    opponents = collections.defaultdict(lambda: collections.defaultdict(lambda: {"wins": 0, "losses": 0}))
    sources = []

    for index, path in enumerate(jsonls):
        records = load(path)
        labels, thinking, seconds = labels_of(records)
        if not labels:
            print("skip {}: no model labels".format(os.path.basename(path)), file=sys.stderr)
            continue
        # Truth files roll per match while the jsonl accumulates, so a file's
        # window is every truth stamp from its own up to the next jsonl's.
        start = stamp(path)
        end = stamp(jsonls[index + 1]) if index + 1 < len(jsonls) else None
        window = [
            t for t in truths if stamp(t) >= start and (end is None or stamp(t) < end)
        ]
        truth_records = []
        for truth_path in window:
            truth_records.extend(load(truth_path))
        sources.append(
            {
                "log": os.path.basename(path),
                "truth": [os.path.basename(t) for t in window],
                "mtime": os.path.getmtime(path),
                "labels": labels,
                "thinking": thinking,
                "seconds": seconds,
            }
        )
        collect(records, truth_records, labels, thinking, seconds, stats)

        # One result record per finished match, in order.
        plies = 0
        for record in records:
            if record.get("kind") == "turn":
                plies = max(plies, int(record.get("turn") or 0))
            elif record.get("kind") == "result":
                text = record.get("text", "")
                winner = "red" if "红方胜" in text else "black" if "黑方胜" in text else None
                if not winner:
                    continue
                # A side the agent did not drive has no model label; it still
                # appears as an opponent, just never as a profile.
                red = labels["red"] or HUMAN
                black = labels["black"] or HUMAN
                matches[red]["plies"] += plies
                matches[black]["plies"] += plies
                if red == black:
                    matches[red]["self_play"] += 1
                else:
                    loser = "black" if winner == "red" else "red"
                    matches[labels[winner] or HUMAN]["wins"] += 1
                    matches[labels[loser] or HUMAN]["losses"] += 1
                    opponents[labels[winner] or HUMAN][labels[loser] or HUMAN]["wins"] += 1
                    opponents[labels[loser] or HUMAN][labels[winner] or HUMAN]["losses"] += 1
                plies = 0

    os.makedirs(OUT_DIR, exist_ok=True)
    summary = []
    for key in sorted(stats):
        entry = stats[key]
        record = matches.get(key, {"wins": 0, "losses": 0, "self_play": 0, "plies": 0})
        games = record["wins"] + record["losses"]
        violations = sum(entry["errors"][r] for r in RULE_REASONS)
        notes = entry["notes"]
        duplicates = len(notes) - len(set(notes))
        body = [
            "# 指纹：{}".format(key),
            "",
            "由 {} 个日志文件重算，{} 手，{} 局。".format(
                len([s for s in sources if key in s["labels"].values()]),
                entry["moves"],
                games + record["self_play"],
            ),
            "",
            "## 战绩",
            "",
            "| 指标 | 值 |",
            "| --- | --- |",
            "| 已完成对局 | {} |".format(games + record["self_play"]),
            "| 胜 / 负 | {} / {} |".format(record["wins"], record["losses"]),
            "| 胜率 | {} |".format(ratio(record["wins"], games)),
            "| 同模型内战 | {} 局（不计胜率） |".format(record["self_play"]),
            "| 平均手数 | {:.0f} |".format(
                record["plies"] / (games + record["self_play"]) if games + record["self_play"] else 0
            ),
            "",
        ]
        if opponents.get(key):
            body += ["| 对手 | 胜 | 负 |", "| --- | --- | --- |"]
            for foe in sorted(opponents[key]):
                line = opponents[key][foe]
                body.append("| {} | {} | {} |".format(foe, line["wins"], line["losses"]))
            body += [""]
        body += side_table(entry)
        body += [
            "## 口径",
            "",
            "- 违规 = 走法不合法 + 造成照面 + 未将军就吃将 + 读盘错误。",
            "- 缺 note、超时、网络或程序异常另计，不算违规。",
            "- 同一手被连拒三次就记三次。",
            "- 只有分出胜负的日志计入战绩；没下完的局仍计入落子和其它统计。",
            "- 耗时按思考开关分桶，不跨桶比较。",
            "- 少于 50 手或少于 2 局时，比例只能当参考。",
            "",
        ]
        name = key.replace("/", "_").replace("\\", "_")
        with open(os.path.join(OUT_DIR, name + ".md"), "w", encoding="utf-8") as handle:
            handle.write("\n".join(body) + "\n")
        with open(os.path.join(OUT_DIR, name + ".json"), "w", encoding="utf-8") as handle:
            json.dump(
                {
                    "model": key,
                    "moves": entry["moves"],
                    "violations": violations,
                    "errors": dict(entry["errors"]),
                    "wins": record["wins"],
                    "losses": record["losses"],
                    "self_play": record["self_play"],
                    "sources": [
                        s for s in sources if key in s["labels"].values()
                    ],
                },
                handle,
                ensure_ascii=False,
                indent=2,
            )
        summary.append(
            (
                key,
                games + record["self_play"],
                entry["moves"],
                violations,
                duplicates,
                len(notes),
                median(entry["durations"][True]),
                median(entry["durations"][False]),
            )
        )

    overview = [
        "# 模型指纹",
        "",
        "| 模型 | 局 | 手 | 违规 | 违规/100 手 | note 重复 | 耗时中位（思考开） | 耗时中位（思考关） |",
        "| --- | --- | --- | --- | --- | --- | --- | --- |",
    ]
    for key, games, moves, violations, duplicates, notes, on, off in summary:
        overview.append(
            "| {} | {} | {} | {} | {} | {}/{} | {} | {} |".format(
                key,
                games,
                moves,
                violations,
                per100(violations, moves),
                duplicates,
                notes,
                "{:.1f} 秒".format(on) if on else "—",
                "{:.1f} 秒".format(off) if off else "—",
            )
        )
    overview += [
        "",
        "违规与重复的定义见各模型文件末尾的「口径」。",
        "",
    ]
    with open(os.path.join(OUT_DIR, "index.md"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(overview) + "\n")
    print("\n".join(overview))
    return 0


if __name__ == "__main__":
    sys.exit(main())
