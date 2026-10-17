"""Two reveal-chess players.

LearnOpenGL has to be on the Reveal Chess scene, which serves MCP at
http://127.0.0.1:8766/mcp. The in-app panel shows each side's thoughts.
There is no conversation: whichever side is marked AI plays when it is
that side's turn. Red and Black are separate models and do not share a
transcript, so a covered piece stays hidden from both.

Every side speaks the OpenAI protocol. .env describes each one with four
values: CHESS_RED_PROVIDER, CHESS_RED_MODEL, CHESS_RED_BASE_URL,
CHESS_RED_API_KEY, and the same four for black. PROVIDER plus MODEL is the
side's identity: it labels the log and names the folder long-term memory is
kept in. Any OpenAI-compatible model works without code changes.
CHESS_<SIDE>_EXTRA_BODY is a JSON object merged into each request for
endpoint-specific knobs such as thinking.

Every match is also written to .output/chess/, under a stamp the engine picks
and passes over as CHESS_LOG_STAMP. The agent writes <stamp>.jsonl (one event
per line) and <stamp>.md (the same events grouped by turn, each turn opening
with the board it was decided on). The engine writes <stamp>.truth.jsonl, the
one place that records what a covered piece really was. The .md merges both
and is the file to hand to a model for review.

Each side keeps one conversation for the whole match. Messages only append, and
SummarizationMiddleware folds the oldest part into a summary once it gets long.
The opponent's thoughts are never in that conversation.

Long-term memory is per model, one folder named provider:model each, holding
self.md and one vs-<provider:model>.md per opponent it has met. Both files are
read into the prompt before the match and rewritten after it, from that side's
own messages only. When the same provider:model plays both colours the two
sides are one mind writing to one set of files, so that deposit happens once
from both halves of the game.
"""

import asyncio
import json
import os
import re
import sys
import threading
import time
import warnings

if "--ui" in sys.argv:
    warnings.filterwarnings("ignore")

from langchain.agents import create_agent
from langchain.agents.middleware import SummarizationMiddleware
from langchain.mcp import MCPAdapter
from langchain_core.messages import AIMessage, RemoveMessage, ToolMessage
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import MemorySaver

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.environ["NO_PROXY"] = "127.0.0.1,localhost"
os.environ["no_proxy"] = "127.0.0.1,localhost"
MCP_URL = os.environ.get("REVEAL_CHESS_MCP_URL", "http://127.0.0.1:8766/mcp")
UI = "--ui" in sys.argv


def parent_gone():
    return os.getppid() == 1

def prompt_for(side, self_memory="", foe_memory=""):
    red = side == "red"
    words = {
        "me": "红方" if red else "黑方",
        "foe": "黑方" if red else "红方",
        "mine": "红" if red else "黑",
        "theirs": "黑" if red else "红",
        "foe_king": "黑帅" if red else "红将",
        "forward": "row + 1" if red else "row − 1",
        "seconds": str(move_seconds()),
    }
    text = (
        "你是{me}，下揭棋（暗子象棋），红先走，对手是另一个模型（{foe}）。\n\n"
        "## 揭棋怎么下\n\n"
        "开局所有子都盖着。`红·` 是摆在你这一半的暗子，`黑·` 是摆在对面的暗子；"
        "兵种和真色开局前就一起打乱了，没人知道，翻之前也不许猜。\n\n"
        "- 暗子按它所在起始格的兵种走，不按隐藏兵种走；移动后在落点翻开。\n"
        "- 翻开后按真实兵种和真色走。翻出「{theirs}」的子就归{foe}——"
        "你出的这一手等于替{foe}把子挪了出来，所以暗子不是稳赚的战力，挪开也不等于丢子。\n"
        "- 可以吃任何子，只不能吃本方明子；暗子（含自己的 {mine}·）都能吃。\n"
        "- 吃{foe_king}即胜，但将不能白吃：只有你上一手已经将军、对手没能解，本手才吃得到将。\n"
        "- 其余按象棋走：车直、马走日且蹩腿、炮隔一子吃、相塞象眼、士斜走、兵过河可横走；"
        "两将不能在同一条直线上照面。与常规象棋不同之处，以「走法」表为准。\n\n"
        "全程用中文：推理过程和回答都写中文。\n\n"
        "## 每回合\n\n"
        "本手预算 {seconds} 秒。超时会被取消，关闭思考后重试一次，再超时判负。"
        "想知道还剩多少秒就调 `clock`。\n\n"
        "1. 先调 `board` 一次，读 `diagram`。若消息里给了{foe}的上一手，以它为准判断翻了什么、吃了什么、自己是否被将军，不要凭记忆。\n"
        "2. 按「选子」里的硬约束自己推演。没有全盘着法列表；拿不准某一枚子能去哪，就调 `moves(row, col)` 问那一枚。"
        "它只给机械合法性——走完会送将的格也照样列出——所以安全和取舍仍由你判断。每手能问的次数有限，拒绝也算一次，回复里的 `queries_left` 是余量；"
        "别把它当成逐子列举的解算器，挑真正在考虑的几枚问。\n"
        "3. 调 `play(from_row, from_col, to_row, to_col, note)`。`note` 必填，一两句中文，说清走哪步、为什么、走完将为何安全。"
        "它会原样进复盘日志，所以只写结论和理由，不要贴推演过程，不要罗列候选；空 note 会被拒绝。\n"
        "4. 报错时棋盘没动。先把错误读完：它说明源格上是什么、为何非法；若该子是你的，还会列出它所有能去的格。同一手绝不再发，\n"
        "   改为走列出的格（仍需做安全检查）或换一个子。不要因为报错就去再调 `board`；若错误说源格为空或不是你的子，说明坐标读错了，重新数行列。\n"
        "   唯一的例外是 `note` 缺失或为空：这一手本身没被受理，补上 note 原样重发即可。\n"
        "5. `play` 成功后立即停手，不要再调 `board` 或 `moves`。\n\n"
        "## 读盘\n\n"
        "`diagram` 是 markdown 表。每行按 `|` 切分，首字段是行号 0（最上）到 9（最下），其后九格是列 0 到 8（左到右）。\n"
        "空字段是空格，但仍占一列。格点记作 (row, col)。红方在 0–4、黑方在 5–9，河界在 4、5 行之间。\n\n"
        "| 内容 | 含义 |\n"
        "| --- | --- |\n"
        "| 空 | 空格 |\n"
        "| 红· | 0–4 行上的暗子，红方走，兵种与真色均未知 |\n"
        "| 黑· | 5–9 行上的暗子，黑方走，兵种与真色均未知 |\n"
        "| 红将 红仕 红相 红车 红马 红炮 红兵 | 红方明子 |\n"
        "| 黑帅 黑士 黑象 黑车 黑马 黑炮 黑卒 | 黑方明子 |\n\n"
        "红将/黑帅 的写法与常规象棋相反，一律以红/黑前缀为准。车、马、炮两色同名。\n\n"
        "## 你能走的子\n\n"
        "- 所有以「{mine}」开头的明子，含己方将。\n"
        "- 所有{mine}·。暗子不走不翻，所以{mine}·一定还在起始格。\n"
        "- 但{mine}·不等于你的子：它只说明这枚子盖着、站在{mine}这一半、由你走。"
        "真色和兵种是一起打乱的，翻开才定归属；翻出「{theirs}」的子就归{foe}，"
        "等于你替{foe}出动了一个子。它不是已知的战力，挪开也不等于丢子。\n"
        "- 不要猜、也不要说出暗子的兵种。没人知道，`play` 也不会透露。\n\n"
        "## 本棋盘强制执行的规则\n\n"
        "| 条目 | 规则 |\n"
        "| --- | --- |\n"
        "| 暗子 | 暗子按它所在起始格的兵种走，不按隐藏兵种走。 |\n"
        "| 翻明 | 暗子移动后在落点翻开；此后按真实兵种走，归真实颜色。 |\n"
        "| 明子 | 明仕/士、明相/象可走全盘，不受九宫与河界限制；明将/帅只能在本方九宫内。 |\n"
        "| 吃子 | 可落在任何子上，只不能吃本方明子；所有暗子都可吃，包括自己的 {mine}·。 |\n"
        "| 无将军 | 引擎不会拒绝让自己将被吃的走法。那不是许可：棋盘会公开标出你的将被将军，对手很快就能吃将取胜。 |\n"
        "| 照面 | 两将不能在同一条直线上直接照面。中间有任意一枚子（暗子也算）就隔开了；会让两将照面的走法非法，引擎直接拒绝。 |\n"
        "| 将军 | 引擎在你走完之后判定，把正被将的一方写进棋盘的 `check`，对手下一手一定看得到。不用你在回答里喊。 |\n"
        "| 吃将 | 只有你上一手已经将军、对手没解，本手才吃得到将，`may_take_king` 为真才行。对手自己走进被吃的位置也吃不到，要先将军一手。 |\n"
        "| 胜负 | 吃对方将即胜。 |\n\n"
        "## 起始格（暗子按此兵种走）\n\n"
        "```\n"
        "row 0  车 马 相 仕 将 仕 相 马 车\n"
        "row 2  炮 on 1, 7\n"
        "row 3  兵 on 0, 2, 4, 6, 8\n"
        "row 6  卒 on 0, 2, 4, 6, 8\n"
        "row 7  炮 on 1, 7\n"
        "row 9  车 马 象 士 帅 士 象 马 车\n"
        "```\n\n"
        "## 走法\n\n"
        "| 子 | 规则 |\n"
        "| --- | --- |\n"
        "| 车 | 沿横线或竖线穿过空格，可停在遇到的第一枚子上（合法时即吃）。 |\n"
        "| 马 | 先直一步再斜一步向外。马腿被占即蹩：走两行时看 (r±1, c)，走两列时看 (r, c±1)。 |\n"
        "| 炮 | 走同车，但不能这样吃。吃子须在与目标同一线上、二者之间恰有一枚子。 |\n"
        "| 相 / 象 | 斜走两格，象眼被占即塞。 |\n"
        "| 仕 / 士 | 斜走一格。起始格上的暗仕/暗士只能进本方九宫，(1,4) 或 (8,4)。 |\n"
        "| 将 / 帅 | 九宫内直走一格：红 0–2 行、黑 7–9 行，列 3–5。 |\n"
        "| 兵 / 卒 | 暗兵/暗卒只朝对面直走一格；明兵/明卒前进方向为 {forward}，过河（红兵在 5–9 行、黑卒在 0–4 行）后可横走一格，永不后退。 |\n\n"
        "## 选子\n\n"
        "自己评估局面，只有三条是硬约束：\n\n"
        "- {foe_king}能吃就吃，但引擎只在 `may_take_king` 为真时接受吃将（见上）。\n"
        "- 若你的将下一手会被吃，本手必须解将，解完即停；不许借机出动。\n"
        "- 不允许走完之后{foe}能吃你的将。\n\n"
        "其余取舍得失由你判断。\n\n"
        "## 回答\n\n"
        "三五句话，用中文：是否被将军、走哪一手、走完之后将为何安全。被将军时这一手必须是解将。不要复述棋盘；复盘日志留下的是 `note`，不是这段回答。\n"
        "将军由引擎宣告，回答里不用喊；`note` 只写这一手的意图。\n"
        "明子按自身算；没动过的暗子按起始格兵种算。"
    ).format(**words)
    if self_memory:
        text += (
            "\n\n## 你的长期记忆\n\n"
            "这是你自己在前几局结束后写下的，可能已经过时。"
            "只当参考，本局一律以棋盘和你读到的规则为准。\n\n"
            + self_memory
        )
    if foe_memory:
        text += (
            "\n\n## 你对{}的笔记\n\n".format(words["foe"])
            + "也是你自己写的，同样可能过时，同样以棋盘为准。\n\n"
            + foe_memory
        )
    return text


def load_env(path):
    if not os.path.exists(path):
        return
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            os.environ.setdefault(key.strip(), value.strip().strip('"').strip("'"))


def message_text(content):
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        parts = []
        for block in content:
            if isinstance(block, str):
                parts.append(block)
            elif isinstance(block, dict):
                parts.append(block.get("text", "") or "")
            else:
                parts.append(getattr(block, "text", "") or "")
        return "".join(parts)
    return str(content)


def clip(text, limit):
    text = " ".join(text.split())
    if len(text) <= limit:
        return text
    return text[:limit] + "…"


LOG_DIR = os.path.join(ROOT, ".output", "chess")
SIDE_NAMES = {"red": "红方", "black": "黑方"}
LOG_LABELS = {
    "think": "思考",
    "digest": "摘要",
    "tool": "调用",
    "move": "落子",
    "error": "错误",
    "timeout": "超时",
    "aside": "内心",
    "taunt": "狠话",
    "summary": "总结",
    "memory": "记忆",
    "check": "将军",
}
LOG = None
PIECE_NAMES = {
    "king": "将",
    "guard": "仕",
    "bishop": "相",
    "rook": "车",
    "horse": "马",
    "cannon": "炮",
    "soldier": "兵",
}
BLACK_PIECE_NAMES = {
    "king": "帅",
    "guard": "士",
    "bishop": "象",
    "rook": "车",
    "horse": "马",
    "cannon": "炮",
    "soldier": "卒",
}


def piece_label(piece):
    """红兵, 黑车, ... from an engine truth entry."""
    black = piece.get("color") == "black"
    names = BLACK_PIECE_NAMES if black else PIECE_NAMES
    return ("黑" if black else "红") + names.get(piece.get("piece"), "?")


def truth_text(record):
    """One line from the engine's record, in the words both players use."""
    side = record.get("side")
    clauses = []
    mover = record.get("mover") or {}
    if mover.get("covered"):
        clause = "翻开成" + piece_label(mover)
        if mover.get("color") != side:
            clause += "，改归对手"
        clauses.append(clause)
    target = record.get("target")
    if target:
        if target.get("covered"):
            clauses.append("吃掉一枚暗子（实为{}）".format(piece_label(target)))
        else:
            clauses.append("吃掉" + piece_label(target))
    return "；".join(clauses) or "移动明子，未吃子"


class game_log:
    """The match record, written twice while it happens.

    <stamp>.jsonl is one event per line with its turn and the seconds since the
    match started, which is what a script scans. <stamp>.md is the same events
    grouped by turn, each turn opening with the board it was decided on, which
    is what gets handed to a model for review. Both are flushed as events
    arrive, so stopping midway still leaves a usable file.

    <stamp>.truth.jsonl comes from the engine, not from here: it holds what each
    move really did, including the type of a covered piece that was captured.
    Neither player is ever told that, so it exists only for the review.
    """

    def __init__(self):
        stamp = os.environ.get("CHESS_LOG_STAMP") or time.strftime(
            "%Y-%m-%d_%H-%M-%S"
        )
        os.makedirs(LOG_DIR, exist_ok=True)
        self.jsonl = os.path.join(LOG_DIR, stamp + ".jsonl")
        self.markdown = os.path.join(LOG_DIR, stamp + ".md")
        self.truth = os.path.join(LOG_DIR, stamp + ".truth.jsonl")
        self.handle = open(self.jsonl, "a", encoding="utf-8")
        self.started = time.time()
        self.entries = []
        self.header = []

    def setup(self, labels, thinking, seconds):
        if self.header:
            return
        self.header = [
            "- 红方：{}".format(labels["red"]),
            "- 黑方：{}".format(labels["black"]),
            "- 思考模式：{}".format("开" if thinking else "关"),
            "- 每手时限：{} 秒".format(seconds),
            "- 开始：{}".format(time.strftime("%Y-%m-%d %H:%M:%S")),
        ]
        self.write({"kind": "session"})

    def turn(self, side, number, diagram):
        self.write({"kind": "turn", "side": side, "turn": number})
        if diagram:
            self.write({"kind": "board", "side": side, "turn": number, "text": diagram})

    def event(self, kind, text, side=""):
        self.write({"kind": kind, "side": side, "text": text})

    def write(self, record):
        record["at"] = round(time.time() - self.started, 2)
        self.entries.append(record)
        self.handle.write(json.dumps(record, ensure_ascii=False) + "\n")
        self.handle.flush()
        self.render()

    def render(self):
        lines = ["# 揭棋对局", ""] + self.header + [""]
        # Legend for the marked board: struck = where the move left, bold =
        # where it landed, struck-and-bolded = what was captured.
        lines += [
            "读图：~~红·~~ 是本手起点，**红仕** 是本手落点（暗子即翻开后的真身），"
            "~~黑车~~**红车** 是落点原有被吃的子。",
            "",
        ]
        truth = self.truth_by_ply()
        opened = False
        turn_at = None
        turn_over = False
        current = None
        digested = False
        think_tail = ""
        for record in self.entries:
            kind = record["kind"]
            if kind == "session":
                continue
            if kind == "turn":
                opened = True
                turn_over = False
                digested = False
                think_tail = ""
                turn_at = record["at"]
                current = record["turn"]
                lines += [
                    "",
                    "## 第 {} 手 · {}".format(
                        record["turn"], SIDE_NAMES.get(record["side"], "")
                    ),
                    "",
                ]
            elif kind == "board":
                # Bare markdown table: a fenced block would render as code
                # instead of a table. The blank line after it keeps the list
                # below from being swallowed by the table.
                lines += [annotate_board(record["text"], truth.get(current)), ""]
            elif kind == "result":
                lines += ["", "## 终局", "", "- 结果：{}".format(record["text"]), ""]
            else:
                label = LOG_LABELS.get(kind)
                if not label:
                    continue
                if not opened:
                    opened = True
                    lines += ["", "## 开局前", ""]
                text = record["text"]
                if kind == "think":
                    # Kept in the .jsonl for full fidelity; the review reads
                    # the digest instead. Hold the tail back as a fallback in
                    # case the model skipped the digest.
                    think_tail = text[-200:]
                    continue
                if kind == "tool" and text.startswith("| "):
                    # A board read echoes the whole diagram; the turn snapshot
                    # above already has it.
                    continue
                if kind == "move" and not digested and think_tail:
                    lines += ["- 思考（节选）：…{}".format(think_tail), ""]
                if kind == "digest":
                    digested = True
                if kind == "move" and turn_at is not None:
                    text = "{}（用时 {:.1f} 秒）".format(text, record["at"] - turn_at)
                lines += ["- {}：{}".format(label, text), ""]
                if kind == "move" and current in truth and not turn_over:
                    turn_over = True
                    lines += ["- 实况：{}".format(truth_text(truth[current])), ""]
        body = re.sub(r"\n{3,}", "\n\n", "\n".join(lines))
        with open(self.markdown, "w", encoding="utf-8") as handle:
            handle.write(body + "\n")

    def truth_by_ply(self):
        """The engine's record of each move, keyed by ply."""
        if not os.path.exists(self.truth):
            return {}
        by_ply = {}
        with open(self.truth, encoding="utf-8") as handle:
            for line in handle:
                try:
                    record = json.loads(line)
                except json.JSONDecodeError:
                    continue
                by_ply[record.get("ply")] = record
        return by_ply


def annotate_board(diagram, record):
    """Show the move on the position it was chosen in.

    The snapshot is the board before the move. The source cell is struck
    through, and the destination is bold: what stands there afterwards - taken
    from the engine's record, so a covered mover shows its real type, which is
    what the review needs and what neither player was told. A capture keeps the
    struck target ahead of it. Only the .md is annotated; the board the model
    reads stays plain.
    """
    if not record:
        return diagram.rstrip("\n")
    try:
        from_row = int(record["from_row"])
        from_col = int(record["from_col"])
        to_row = int(record["to_row"])
        to_col = int(record["to_col"])
    except (KeyError, TypeError, ValueError):
        return diagram.rstrip("\n")
    mover = record.get("mover") or {}
    if not mover.get("piece"):
        return diagram.rstrip("\n")
    arrived = piece_label(mover)
    out = []
    for line in diagram.rstrip("\n").splitlines():
        parts = line.split("|")
        # | row | c0 | ... | c8 |  ->  ["", row, c0..c8, ""]
        if len(parts) != 12:
            out.append(line)
            continue
        row_text = parts[1].strip()
        if not row_text.isdigit():
            out.append(line)
            continue
        row = int(row_text)
        cells = [parts[2 + col].strip() for col in range(9)]
        if row == from_row:
            cells[from_col] = "~~{}~~".format(cells[from_col] or "空")
        if row == to_row:
            taken = cells[to_col]
            cells[to_col] = (
                "~~{}~~**{}**".format(taken, arrived) if taken else "**{}**".format(arrived)
            )
        out.append("| " + " | ".join([row_text] + cells) + " |")
    return "\n".join(out)


def emit(kind, text="", side=""):
    text = text.strip()
    if not text:
        return
    if LOG:
        LOG.event(kind, text, side)
    if not UI:
        print("{} {} {}".format(side, kind, text).strip())
        return
    payload = {"kind": kind, "text": text}
    if side:
        payload["side"] = side
    sys.stdout.write(json.dumps(payload, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def show(message, played, side):
    if isinstance(message, AIMessage):
        extra = getattr(message, "additional_kwargs", {}) or {}
        reasoning = extra.get("reasoning_content") or extra.get("reasoning") or ""
        if isinstance(reasoning, list):
            reasoning = "".join(str(part) for part in reasoning)
        if str(reasoning).strip():
            emit("think", clip(str(reasoning), 4000), side)
        calls = getattr(message, "tool_calls", None) or []
        text = message_text(message.content).strip()
        if text:
            emit("agent", text, side)
        for call in calls:
            # play carries its own note. The engine rejects the call without
            # one, so this is the digest the review can always rely on; the
            # chain of thought is never what the .md shows. The note leaves
            # the logged arguments so the line stays one short line.
            args = call.get("args", {})
            note = ""
            if isinstance(args, dict):
                args = dict(args)
                note = str(args.pop("note", "")).strip()
            if note:
                emit("digest", clip(note, 300), side)
            emit(
                "tool",
                "{} {}".format(call.get("name", ""), json.dumps(args, ensure_ascii=False)),
                side,
            )
    elif isinstance(message, ToolMessage):
        body = message_text(message.content)
        name = getattr(message, "name", "") or ""
        if name == "play":
            try:
                payload = json.loads(body)
            except (TypeError, json.JSONDecodeError):
                payload = None
            if isinstance(payload, dict) and payload.get("ok"):
                played["ok"] = True
                played["payload"] = payload
                played["move"] = "({}, {}) -> ({}, {})".format(
                    payload.get("from_row"),
                    payload.get("from_col"),
                    payload.get("to_row"),
                    payload.get("to_col"),
                )
                emit("move", played["move"], side)
                return
            if isinstance(payload, dict) and payload.get("error"):
                emit("error", payload["error"], side)
                return
        if name == "board":
            try:
                payload = json.loads(body)
            except (TypeError, json.JSONDecodeError):
                payload = None
            diagram = payload.get("diagram") if isinstance(payload, dict) else None
            if diagram:
                emit("tool", diagram, side)
                return
        emit("tool", clip(body, 240), side)


def side_setup(side):
    """Everything about one side's model, straight from .env.

    Nothing here names a provider. Each side supplies what an OpenAI-style
    endpoint needs, and only that:

        CHESS_RED_PROVIDER   name this side is logged and remembered under
        CHESS_RED_MODEL      model id sent in the request
        CHESS_RED_BASE_URL   root the endpoint serves, usually ending in /v1
        CHESS_RED_API_KEY    bearer token

    EXTRA_BODY is a JSON object merged into every request, for providers with
    their own knobs (thinking, reasoning budgets, top_k). EXTRA_BODY_NOTHINK
    replaces it when thinking is off, which is what the timeout retry uses.
    """
    prefix = "CHESS_{}_".format(side.upper())

    def value(name):
        return os.environ.get(prefix + name, "").strip()

    provider = value("PROVIDER").lower()
    model = value("MODEL")
    if not provider or not model:
        raise RuntimeError(
            "{}PROVIDER and {}MODEL must be set in .env.".format(prefix, prefix)
        )
    # Sharing one account between the sides is common enough to allow.
    shared = provider.upper()
    key = value("API_KEY") or os.environ.get(shared + "_API_KEY", "").strip()
    base = value("BASE_URL") or os.environ.get(shared + "_BASE_URL", "").strip()
    if not key:
        raise RuntimeError(
            "No API key for {} ({}). Set {}{} or {}_API_KEY in .env.".format(
                side, provider, prefix, "API_KEY", shared
            )
        )
    if not base:
        raise RuntimeError(
            "No base URL for {} ({}). Set {}{} or {}_BASE_URL in .env.".format(
                side, provider, prefix, "BASE_URL", shared
            )
        )
    body = extra_body(prefix + "EXTRA_BODY")
    plain = extra_body(prefix + "EXTRA_BODY_NOTHINK")
    return provider, model, key, base, body, plain


def extra_body(name):
    """A JSON object from .env, or empty. Bad JSON is a config error, not a 400."""
    raw = os.environ.get(name, "").strip()
    if not raw:
        return {}
    try:
        parsed = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise RuntimeError("{} is not valid JSON: {}".format(name, exc))
    if not isinstance(parsed, dict):
        raise RuntimeError("{} must be a JSON object.".format(name))
    return parsed


class reasoning_chat(ChatOpenAI):
    """Plain OpenAI chat, plus whatever reasoning text the endpoint returns.

    Some endpoints put the chain of thought in `reasoning_content`, and in
    thinking mode with a tool call they require that field back on the
    assistant message in the next request. ChatOpenAI drops it, and the
    follow-up call then stalls. This keeps any such field it finds.
    """

    def _create_chat_result(self, response, generation_info=None):
        result = super()._create_chat_result(response, generation_info)
        choices = getattr(response, "choices", None)
        if not choices:
            return result
        message = choices[0].message
        reasoning = getattr(message, "reasoning_content", None)
        if not reasoning:
            extra = getattr(message, "model_extra", None) or {}
            reasoning = extra.get("reasoning_content")
        if reasoning:
            result.generations[0].message.additional_kwargs["reasoning_content"] = reasoning
        return result

    def _get_request_payload(self, input_, *, stop=None, **kwargs):
        payload = super()._get_request_payload(input_, stop=stop, **kwargs)
        sent = payload.get("messages")
        if not isinstance(sent, list):
            return payload
        originals = [
            message
            for message in self._convert_input(input_).to_messages()
            if getattr(message, "type", None) == "ai"
        ]
        sent_ai = [
            message
            for message in sent
            if isinstance(message, dict) and message.get("role") == "assistant"
        ]
        for original, message in zip(originals, sent_ai):
            reasoning = (getattr(original, "additional_kwargs", None) or {}).get(
                "reasoning_content"
            )
            if reasoning:
                message["reasoning_content"] = reasoning
        return payload


def thinking_on():
    """Set by the Think checkbox. Defaults on, matching the previous behavior."""
    return os.environ.get("CHESS_THINKING", "1").strip().lower() not in (
        "0",
        "false",
        "off",
        "no",
    )


def move_seconds():
    """Set by the Move limit box. The agent cancels at this budget, the engine at twice it."""
    try:
        return max(1, int(os.environ.get("CHESS_MOVE_SECONDS", "60")))
    except ValueError:
        return 60


class Player:
    """One side of the match: config, chat model, and the name it logs under.

    Nothing here knows which vendor is behind the endpoint. A side is whatever
    .env describes in OpenAI terms, so a new model is a .env edit and no code.
    """

    def __init__(self, side):
        self.side = side
        (
            self.provider,
            self.model,
            self.key,
            self.base_url,
            self.body,
            self.plain_body,
        ) = side_setup(side)
        self.label = "{}:{}".format(self.provider, self.model)
        self.chat = self.build()

    def build(self, thinking=None):
        """A chat client for this player. thinking=False drops extra_body."""
        on = thinking_on() if thinking is None else thinking
        return reasoning_chat(
            model=self.model,
            api_key=self.key,
            base_url=self.base_url,
            temperature=0,
            timeout=120,
            max_retries=1,
            extra_body=self.body if on else self.plain_body,
        )

    def plain(self):
        """No thinking. Used for short lines, summaries, and the retry."""
        return self.build(thinking=False)


def describe_last_move(diagram, before, last):
    """Public facts about the opponent's move. A captured covered piece stays unnamed."""
    if not isinstance(last, dict):
        return ""
    fr = last.get("from_row")
    fc = last.get("from_col")
    tr = last.get("to_row")
    tc = last.get("to_col")
    if None in (fr, fc, tr, tc):
        return ""
    src = cell_at(before, fr, fc) if before else ""
    taken = cell_at(before, tr, tc) if before else None
    now = cell_at(diagram, tr, tc)
    lines = ["对手上一手：({}, {}) -> ({}, {})。".format(fr, fc, tr, tc)]
    if src:
        lines.append("走子的那枚子原显示为 {}。".format(src))
    if now:
        lines.append("它现在位于 ({}, {})，显示为 {}。".format(tr, tc, now))
    if before is not None:
        if not taken:
            lines.append("落点原本是空格，没有吃到子。")
        elif taken.endswith("·"):
            lines.append(
                "它吃掉了一枚暗子（{}），兵种未公开，不要猜。".format(taken)
            )
        else:
            lines.append("它吃掉了明子 {}。".format(taken))
    lines.append(
        "以这一手判断局势：翻出了什么、吃掉了什么、你的将是否被将军。"
    )
    return "\n".join(lines)


def cell_at(diagram, row, col):
    """One diagram cell. Empty string if the square is empty or the table is unreadable."""
    if row is None or col is None or not diagram:
        return ""
    for line in diagram.splitlines():
        parts = [part.strip() for part in line.split("|")]
        if len(parts) < 12 or parts[1] != str(row):
            continue
        return parts[2 + col]
    return ""


def describe_outcome(side, before_diagram, after, payload):
    """Public facts about a move that has already been played.

    A captured covered piece leaves the board without showing its type, so
    that type is never included. A mover that was covered is face up afterward,
    and that revealed piece is on the board for both sides.
    """
    fr = payload.get("from_row")
    fc = payload.get("from_col")
    tr = payload.get("to_row")
    tc = payload.get("to_col")
    src = cell_at(before_diagram, fr, fc)
    dst = cell_at(after.get("diagram", ""), tr, tc)
    taken = cell_at(before_diagram, tr, tc)
    mine = "红" if side == "red" else "黑"
    lines = [
        "你走了 ({}, {}) -> ({}, {})。".format(fr, fc, tr, tc),
        # One piece, one move. Said separately, the two squares read as two
        # pieces, and the model then mourns the covered piece it just moved.
        "源格当时显示 {}；同一枚子移到了落点，现在显示 {}；"
        "源格空出来是因为它走开了，不是丢了子。".format(src or "空", dst or "空"),
    ]
    if src.endswith("·"):
        if dst.startswith(mine):
            lines.append("它原本是暗子，已翻开，是你的子。")
        elif dst:
            lines.append(
                "它原本是暗子，翻开后是对手的子，从现在起归对手。"
            )
    elif src:
        lines.append("它本来就是明子。")
    if not taken:
        lines.append("落点原本是空格，没有吃到子。")
    elif taken.endswith("·"):
        whose = "你自己的" if taken.startswith(mine) else "对手的"
        lines.append(
            "你吃掉了{}暗子（{}），兵种从未公开，不要说出它是什么。".format(
                whose, taken
            )
        )
    else:
        lines.append("你吃掉了明子 {}。".format(taken))
    result = payload.get("result") or after.get("result")
    if payload.get("check"):
        lines.append(
            "将军：{}的将正被你将着，它本手必须解将；它不解，你下一手就吃将。"
            "这次将军已由引擎公开，不用你在回答里喊。".format(
                "黑方" if side == "red" else "红方"
            )
        )
    if side in (after.get("check") or []):
        lines.append("你自己的将也正被对方将着：轮到你时必须先解将，否则会被吃将。")
    if result == "red_win":
        lines.append("红方胜。")
    elif result == "black_win":
        lines.append("黑方胜。")
    return "\n".join(lines)


async def speak(player, text):
    """One short line. Thinking stays off so the game is not left waiting."""
    message = await player.plain().ainvoke([{"role": "user", "content": text}])
    return message_text(message.content).strip()


def taunt_text(side):
    me = "红方" if side == "red" else "黑方"
    foe = "黑方" if side == "red" else "红方"
    return (
        "你是揭棋的{}，对局还没开始，棋子都盖着。对{}放一句狠话，一两句中文。"
        "不要声称知道任何暗子的兵种，不要说具体走法。"
    ).format(me, foe)


def summary_text(side, result, diagram, notes):
    me = "红方" if side == "red" else "黑方"
    outcome = "红方胜" if result == "red_win" else "黑方胜"
    lead = ""
    if notes:
        lead = "你自己的内心笔记：\n{}\n\n".format("\n".join(notes[-6:]))
    return (
        lead
        + "对局结束，{}。你是{}。\n终盘：\n{}\n\n"
        "用中文写三四句赛后总结：这盘怎么走的，你怎么看这个结果。"
        "只提终盘上明着的棋子，或笔记里已经写明翻开的棋子。"
        "被吃掉的暗子兵种没人知道，不要猜。"
    ).format(outcome, me, diagram)


async def write_aside(player, outcome, earlier):
    """Private reaction after the move has resolved. Thinking stays off."""
    lead = ""
    if earlier:
        lead = "你之前的内心笔记：\n{}\n\n".format("\n".join(earlier[-6:]))
    mine = "红" if player.side == "red" else "黑"
    text = (
        lead
        + "刚刚发生的、已经确认的事实：\n"
        + outcome
        + "\n\n用中文写两三句内心戏，回应这些事实。"
        + "{}· 只是「盖着、站在你这一半」，不代表是你的子，翻开才定归属，"
        "所以自己的暗子挪了窝不算丢东西。".format(mine)
        + "如果暗子翻出来是对手的子，或者你吃了自己的暗子，就针对这点反应。"
        "这些话只有你自己看。凡是上面没写明的暗子兵种，一律不要猜、不要说出。"
        "不要替对手设想下一步。"
    )
    return await speak(player, text)


async def tool_json(tools, name):
    raw = await tools[name].ainvoke({})
    return json.loads(message_text(raw))


MEMORY_DIR = os.path.join(LOG_DIR, "memory")


def memory_files(key, foe):
    """<key>/self.md and <key>/vs-<foe>.md, where key is provider:model.

    One folder per model, holding its own lessons and one file per opponent it
    has faced. Two models under the same PROVIDER label therefore keep
    separate memories, and a model's opinion of one opponent survives a match
    against another.
    """
    folder = os.path.join(MEMORY_DIR, path_name(key))
    return (
        folder,
        os.path.join(folder, "self.md"),
        os.path.join(folder, "vs-{}.md".format(path_name(foe))),
    )


def path_name(key):
    """A model id used in a path. Provider and model stay readable, separated
    by a colon; slashes, which many model ids use, become underscores."""
    return key.replace("/", "_").replace("\\", "_")


def read_memory(path):
    """A memory file, or empty if this model has never written one."""
    if not os.path.exists(path):
        return ""
    with open(path, encoding="utf-8") as handle:
        return handle.read().strip()


def write_memory(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(text.strip() + "\n")


def transcript(messages, limit=12000):
    """The side's own conversation as text, for the memory pass.

    Tool results are dropped: a board read echoes the whole diagram, and a play
    result is already said in the note on the call. The opponent's thoughts
    were never in this list. The tail is what carries, so that is what a long
    match keeps.
    """
    lines = []
    for message in messages:
        kind = getattr(message, "type", "")
        if kind == "human":
            text = clip(message_text(message.content), 400)
            if text:
                lines.append("局面：" + text)
        elif kind == "ai":
            text = clip(message_text(message.content), 160)
            if text:
                lines.append("我说：" + text)
            for call in getattr(message, "tool_calls", None) or []:
                args = call.get("args", {})
                note = ""
                if isinstance(args, dict):
                    args = dict(args)
                    note = str(args.pop("note", "")).strip()
                lines.append(
                    "行动：{} {}{}".format(
                        call.get("name", ""),
                        json.dumps(args, ensure_ascii=False),
                        "（{}）".format(note) if note else "",
                    )
                )
    body = "\n".join(lines)
    if len(body) > limit:
        body = "…（前面略）\n" + body[-limit:]
    return body


SELF_MEMORY_PROMPT = (
    "你又要下揭棋了。先更新你给自己留的长期记忆。\n\n"
    "## 之前的长期记忆\n\n{prior}\n\n"
    "## 刚结束的这一局（{scope}）\n\n{history}\n\n"
    "用中文重写这份记忆，下一局开局就用它。"
    "保留仍然成立的经验，过期或被推翻的删掉，补上这一局学到的。"
    "分三节：开局偏好 / 翻子与吃子的经验 / 失误与教训。"
    "只写下一局还用得上的判断：不要复述棋谱，不要写坐标推演，"
    "不要编造没有发生的翻子。直接输出记忆正文，不要开场白。"
)

FOE_MEMORY_PROMPT = (
    "你又要和 {foe} 下揭棋了。先更新你记下的、关于它的那份笔记。\n\n"
    "## 之前记下的\n\n{prior}\n\n"
    "## 刚结束的这一局（{scope}）\n\n{history}\n\n"
    "用中文重写这份笔记，只写能用来对付 {foe} 的东西。"
    "分两节：它的习惯与偏好 / 可以被利用的弱点。"
    "被这一局推翻的旧结论要删掉，不要留着一个已经不准的判断。"
    "没有证据的猜测不要写；暗子的兵种没人知道，不要编。"
    "直接输出正文，不要开场白。"
)


async def rewrite_memory(player, prompt, prior, messages, foe, mirror=None):
    """One model call: the old file plus this match in, the new file out.

    `mirror` is the other side's messages in a match where the same
    provider:model played both colours. There is no opponent in that game, only
    the same mind twice, so both halves are fed in and the prompt says so.
    """
    history = transcript(messages)
    if not history:
        return ""
    if mirror is None:
        scope = "你自己的对话，没有对手的想法"
    else:
        history += "\n\n## 同一个 provider:model 执另一方的记录\n\n" + transcript(mirror)
        scope = "两方都是同一个 provider:model，记忆只有一份，两半都算你自己的"
    return await speak(
        player,
        prompt.format(
            prior=prior or "（还没有）", history=history, foe=foe, scope=scope
        ),
    )


async def deposit_memory(player, foe, entry, messages, mirror=None):
    """Rewrite both of this model's files after a match.

    One call each: self.md is general lessons, vs-<foe>.md is only about the
    model just played, named provider:model. Returns what was written, for the
    log.
    """
    written = []
    for key, prompt, index in (
        ("self", SELF_MEMORY_PROMPT, 1),
        ("foe", FOE_MEMORY_PROMPT, 2),
    ):
        text = await rewrite_memory(
            player, prompt, entry[key], messages, foe, mirror
        )
        if not text:
            continue
        entry[key] = text
        write_memory(entry["files"][index], text)
        written.append((key, entry["files"][index], text))
    return written


async def run_turn(agent, side, content, thread_id):
    """Stream one move. Returns the play result the stream reported."""
    played = {"ok": False}
    async for update in agent.astream(
        {"messages": [{"role": "user", "content": content}]},
        {
            "configurable": {"thread_id": thread_id},
            "recursion_limit": 24,
        },
        stream_mode="updates",
    ):
        for node, step in update.items():
            if node not in ("model", "tools"):
                # The summarization node rewrites the history and re-emits the
                # messages it kept. Those belong to earlier plies, so logging
                # them here would repeat them under this turn. It runs before
                # every model call, so only a rewrite is worth reporting.
                messages = step.get("messages", []) if isinstance(step, dict) else []
                if any(isinstance(message, RemoveMessage) for message in messages):
                    emit("status", "上下文过长，旧消息已压缩成摘要", side)
                continue
            messages = step.get("messages", []) if isinstance(step, dict) else []
            if not isinstance(messages, list):
                messages = [messages]
            for message in messages:
                show(message, played, side)
        if played["ok"]:
            break
    return played


SUMMARY_PROMPT = (
    "你在下一盘揭棋。下面的历史马上会被这份摘要替换，后面紧接着最近几手。\n\n"
    "只保留之后还用得上的东西：已经翻开的暗子成了什么、吃过谁的子、"
    "你定下的计划、你踩过的坑、你对对手习惯的判断。"
    "不要复述棋盘，不要写坐标推演，不要编造没发生的翻子。"
    "用中文，三百字以内，直接输出摘要正文。\n\n<messages>\n{messages}\n</messages>"
)


def make_agent(player, tools, self_memory="", foe_memory="", thinking=None):
    """One conversation per side, kept for the whole match.

    Messages only append: each turn adds the position and the agent's own tool
    exchanges. The opponent's thoughts never enter this list. Once the context
    gets long the middleware folds the oldest part into a summary, so a match
    can run past the window without the list being rebuilt by hand.

    thinking=False is the retry that runs after a turn timed out.
    """
    chat = player.chat if thinking is None else player.build(thinking)
    return create_agent(
        chat,
        tools,
        system_prompt=prompt_for(player.side, self_memory, foe_memory),
        middleware=[
            SummarizationMiddleware(
                model=player.plain(),
                trigger=("tokens", 200000),
                keep=("messages", 10),
                summary_prompt=SUMMARY_PROMPT,
            )
        ],
        checkpointer=MemorySaver(),
    )


async def session(adapter, players):
    tools = {tool.name: tool for tool in await adapter.list_tools()}
    emit("status", "tools: {}".format(", ".join(sorted(tools))))
    shared = list(tools.values())
    agents = {}
    notes = {"red": [], "black": []}
    # Recall happens once, before the match: each side reads its own folder and
    # the file for the provider:model it is about to face.
    memory = {"red": {"self": "", "foe": "", "files": None},
              "black": {"self": "", "foe": "", "files": None}}
    # One thread per side for the whole match. It is only replaced when a run
    # dies, since a half-written turn would poison every later request.
    threads = {"red": "red", "black": "black"}
    quick_agents = {}
    for side in ("red", "black"):
        foe = "black" if side == "red" else "red"
        files = memory_files(players[side].label, players[foe].label)
        memory[side] = {
            "self": read_memory(files[1]),
            "foe": read_memory(files[2]),
            "files": files,
        }
        agents[side] = make_agent(
            players[side], shared, memory[side]["self"], memory[side]["foe"]
        )
    emit(
        "status",
        "red {}  vs  black {}, thinking {}".format(
            players["red"].label,
            players["black"].label,
            "on" if thinking_on() else "off",
        ),
    )
    if LOG:
        LOG.setup(
            {"red": players["red"].label, "black": players["black"].label},
            thinking_on(),
            move_seconds(),
        )
    turns = {"red": 0, "black": 0}
    idle = None
    greeted = False
    seen_ongoing = False
    summarized = None
    saved_diagram = None
    saved_turn = None
    before_opponent = None
    while True:
        if parent_gone():
            return
        try:
            state = await tool_json(tools, "board")
        except Exception as exc:
            emit("error", str(exc))
            await asyncio.sleep(1)
            continue
        result = state.get("result", "waiting")
        local = state.get("mode") == "local"
        if local and result == "ongoing":
            if summarized:
                # A new match. Each side starts a fresh conversation, and the
                # memory written when the last one ended is in its prompt now.
                for side in ("red", "black"):
                    notes[side] = []
                    agents[side] = make_agent(
                        players[side],
                        shared,
                        memory[side]["self"],
                        memory[side]["foe"],
                    )
                    threads[side] = side
                quick_agents.clear()
            seen_ongoing = True
            summarized = None
        elif (
            local
            and seen_ongoing
            and result in ("red_win", "black_win")
            and result != summarized
        ):
            if LOG:
                LOG.event("result", "红方胜" if result == "red_win" else "黑方胜")
            ai_sides = [
                s for s in ("red", "black") if state.get("{}_ai".format(s))
            ]
            # The same provider:model on both sides is the same folder and the
            # same two files, so two deposits would overwrite each other. Treat
            # it as one memory and write once, from both halves of the game.
            same_mind = (
                len(ai_sides) == 2
                and players["red"].label == players["black"].label
            )
            for side in ("red", "black"):
                if side not in ai_sides:
                    continue
                try:
                    text = await speak(
                        players[side],
                        summary_text(side, result, state.get("diagram", ""), notes[side]),
                    )
                except Exception as exc:
                    emit("error", str(exc), side)
                    continue
                if text:
                    emit("summary", text, side)
                # Deposit: the same conversation is written back into the
                # side's folder, once as self-knowledge and once as a note on
                # the model it just played. The opponent file is chosen by
                # provider:model, so facing someone else never overwrites it.
                if same_mind and side != ai_sides[0]:
                    continue
                try:
                    snapshot = await agents[side].aget_state(
                        {"configurable": {"thread_id": threads[side]}}
                    )
                    mirror = None
                    if same_mind:
                        other = ai_sides[1]
                        other_snapshot = await agents[other].aget_state(
                            {"configurable": {"thread_id": threads[other]}}
                        )
                        mirror = other_snapshot.values.get("messages", [])
                    foe = "black" if side == "red" else "red"
                    written = await deposit_memory(
                        players[side],
                        players[foe].label,
                        memory[side],
                        snapshot.values.get("messages", []),
                        mirror,
                    )
                except Exception as exc:
                    emit("error", str(exc), side)
                    continue
                if same_mind:
                    # Both entries point at the files just written, so they
                    # must not keep the pre-match text for the next match.
                    for key, _, text in written:
                        memory[ai_sides[1]][key] = text
                for key, path, text in written:
                    emit(
                        "memory",
                        "{} → {}：{}".format(
                            "自己" if key == "self" else "对手",
                            os.path.relpath(path, ROOT),
                            text,
                        ),
                        side,
                    )
            summarized = result
            greeted = False
        if not local or result != "ongoing":
            if result != idle:
                emit("status", result)
                idle = result
            await asyncio.sleep(0.5)
            continue
        if "last_move" not in state:
            before_opponent = None
        elif saved_turn != state.get("turn") and saved_diagram:
            before_opponent = saved_diagram
        saved_diagram = state.get("diagram")
        saved_turn = state.get("turn")
        fresh = "last_move" not in state
        if fresh and not greeted:
            for side in ("red", "black"):
                if not state.get("{}_ai".format(side)):
                    continue
                try:
                    text = await speak(players[side], taunt_text(side))
                except Exception as exc:
                    emit("error", str(exc), side)
                    continue
                if text:
                    emit("taunt", text, side)
            greeted = True
        elif not fresh:
            greeted = False
        side = state.get("turn")
        if side not in agents or not state.get("{}_ai".format(side)) or not state.get("can_move"):
            await asyncio.sleep(0.3)
            continue
        idle = None
        emit("status", "thinking…", side)
        turns[side] += 1
        if LOG:
            # The engine counts every move it committed, so its ply is right
            # even when a human played one of the earlier turns.
            LOG.turn(side, int(state.get("ply", 0)) + 1, state.get("diagram", ""))
        content = "轮到你了。"
        if side in state.get("check", []):
            # The opponent was never told by the mover; the engine says it here,
            # so a check always reaches the side that has to answer it.
            foe = "黑方" if side == "red" else "红方"
            content += "\n\n你正被将军：{}上一手将着你的将，本手必须解将。".format(foe)
            emit("check", "{}的将被将军，本手必须解将。".format(SIDE_NAMES[side]), side)
        last = describe_last_move(
            state.get("diagram", ""), before_opponent, state.get("last_move")
        )
        if last:
            content += "\n\n" + last
        if notes[side]:
            content += "\n\n只有你能看到的内心笔记：\n" + "\n".join(notes[side][-6:])
        budget = move_seconds()
        try:
            played = await asyncio.wait_for(
                run_turn(agents[side], side, content, threads[side]),
                timeout=budget,
            )
        except asyncio.TimeoutError:
            emit("timeout", "{} 秒内没落子，关闭思考重试。".format(budget), side)
            try:
                latest = await tool_json(tools, "board")
            except Exception:
                latest = {}
            # The cancelled attempt may have landed its move, or the engine may
            # have forfeited; either way a second play would be for the wrong
            # side.
            if latest.get("turn") != side or latest.get("result") != "ongoing":
                continue
            if side not in quick_agents:
                quick_agents[side] = make_agent(players[side], shared, thinking=False)
            hurry = (
                "时间到。棋盘如下，只调 `play`，不要调 `board`，不要解释。\n\n"
                + state.get("diagram", "")
            )
            try:
                played = await asyncio.wait_for(
                    run_turn(
                        quick_agents[side],
                        side,
                        hurry,
                        "{}-{}-quick".format(side, turns[side]),
                    ),
                    timeout=budget,
                )
            except asyncio.TimeoutError:
                emit("timeout", "再次超时，判负。", side)
                await asyncio.sleep(2)
                continue
            except Exception as exc:
                emit("error", str(exc), side)
                await asyncio.sleep(2)
                continue
        except Exception as exc:
            emit("error", str(exc), side)
            # A run that died can leave an assistant tool call with no result,
            # and every later request on that thread would be rejected. Give
            # the side a fresh one instead of failing the rest of the match.
            threads[side] = "{}-{}".format(side, turns[side])
            await asyncio.sleep(2)
            continue
        if not played["ok"]:
            emit("error", "No move. Trying again shortly.", side)
            await asyncio.sleep(2)
            continue
        payload = played.get("payload") or {}
        try:
            after = await tool_json(tools, "board")
        except Exception as exc:
            emit("error", str(exc), side)
            after = {}
        outcome = describe_outcome(side, state.get("diagram", ""), after, payload)
        try:
            note = await write_aside(players[side], outcome, notes[side])
        except Exception as exc:
            emit("error", str(exc), side)
            note = ""
        if note:
            notes[side].append(note)
            del notes[side][:-6]
            emit("aside", note, side)


async def main():
    global LOG
    load_env(os.path.join(ROOT, ".env"))
    try:
        players = {side: Player(side) for side in ("red", "black")}
    except RuntimeError as exc:
        emit("error", str(exc))
        return 1
    LOG = game_log()
    announced = False
    while True:
        try:
            async with MCPAdapter(MCP_URL) as adapter:
                announced = False
                await session(adapter, players)
                return 0
        except Exception as exc:
            if not announced:
                emit("error", "Could not reach the board: {}".format(exc))
                announced = True
            await asyncio.sleep(0.5)


if __name__ == "__main__":
    if UI:
        worker = threading.Thread(target=lambda: os._exit(asyncio.run(main())), name="chess-ai")
        worker.start()
        worker.join()
    else:
        raise SystemExit(asyncio.run(main()))
