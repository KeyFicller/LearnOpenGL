"""Two reveal-chess players.

LearnOpenGL has to be on the Reveal Chess scene, which serves MCP at
http://127.0.0.1:8766/mcp. The in-app panel shows each side's thoughts.
There is no conversation: whichever side is marked AI plays when it is
that side's turn. Red and Black are separate models and do not share a
transcript, so a covered piece stays hidden from both.

The API key is read from the repository .env (DEEPSEEK_API_KEY,
DEEPSEEK_BASE_URL, optional DEEPSEEK_MODEL).

Every match is also written to .output/chess/, under a stamp the engine picks
and passes over as CHESS_LOG_STAMP. The agent writes <stamp>.jsonl (one event
per line) and <stamp>.md (the same events grouped by turn, each turn opening
with the board it was decided on). The engine writes <stamp>.truth.jsonl, the
one place that records what a covered piece really was. The .md merges both
and is the file to hand to a model for review.
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

from langchain.mcp import MCPAdapter
from langchain_core.messages import AIMessage, ToolMessage
from langchain_deepseek import ChatDeepSeek
from langchain_openai import ChatOpenAI
from langgraph.checkpoint.memory import MemorySaver
from langgraph.prebuilt import create_react_agent

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
os.environ["NO_PROXY"] = "127.0.0.1,localhost"
os.environ["no_proxy"] = "127.0.0.1,localhost"
MCP_URL = os.environ.get("REVEAL_CHESS_MCP_URL", "http://127.0.0.1:8766/mcp")
UI = "--ui" in sys.argv


def parent_gone():
    return os.getppid() == 1

def prompt_for(side):
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
    return (
        "你是{me}，下揭棋（暗子象棋），红先走，对手是另一个模型（{foe}）。\n\n"
        "全程用中文：推理过程和回答都写中文。\n\n"
        "## 每回合\n\n"
        "本手预算 {seconds} 秒。超时会被取消，关闭思考后重试一次，再超时判负。"
        "想知道还剩多少秒就调 `clock`。\n\n"
        "1. 先调 `board` 一次，读 `diagram`。若消息里给了{foe}的上一手，以它为准判断翻了什么、吃了什么、自己是否被将军，不要凭记忆。\n"
        "2. 按「选子」里的硬约束自己推演。没有合法着法列表，每个候选都要自己核对规则。\n"
        "3. 调 `play(from_row, from_col, to_row, to_col, note)`。`note` 必填，一两句中文，说清走哪步、为什么、走完将为何安全。"
        "它会原样进复盘日志，所以只写结论和理由，不要贴推演过程，不要罗列候选；空 note 会被拒绝。\n"
        "4. 报错时棋盘没动。先把错误读完：它说明源格上是什么、为何非法；若该子是你的，还会列出它所有能去的格。同一手绝不再发，\n"
        "   改为走列出的格（仍需做安全检查）或换一个子。不要因为报错就去再调 `board`；若错误说源格为空或不是你的子，说明坐标读错了，重新数行列。\n"
        "   唯一的例外是 `note` 缺失或为空：这一手本身没被受理，补上 note 原样重发即可。\n"
        "5. `play` 成功后立即停手，不要再调 `board`。\n\n"
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
        "## 己方子\n\n"
        "- 所有以「{mine}」开头的明子，含己方将。\n"
        "- 所有{mine}·。暗子不走不翻，所以{mine}·一定还在起始格。\n"
        "- 30 个暗子的兵种已在两色间打乱：翻{mine}·可能翻出「{theirs}」的子，此后归{foe}。\n"
        "- 不要猜、也不要说出暗子的兵种。没人知道，`play` 也不会透露。\n\n"
        "## 本棋盘强制执行的规则\n\n"
        "| 条目 | 规则 |\n"
        "| --- | --- |\n"
        "| 暗子 | 暗子按它所在起始格的兵种走，不按隐藏兵种走。 |\n"
        "| 翻明 | 暗子移动后在落点翻开；此后按真实兵种走，归真实颜色。 |\n"
        "| 明子 | 明仕/士、明相/象可走全盘，不受九宫与河界限制；明将/帅只能在本方九宫内。 |\n"
        "| 吃子 | 可落在任何子上，只不能吃本方明子；所有暗子都可吃，包括自己的 {mine}·。 |\n"
        "| 无将军 | 引擎不会拒绝让自己将被吃的走法。那不是许可，对手下一手即可吃将取胜。 |\n"
        "| 照面 | 两将可以照面，没有飞将规则。 |\n"
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
        "- 有子能合法吃{foe_king}，就吃。\n"
        "- 若你的将下一手会被吃，本手必须解将，解完即停；不许借机出动，也不许只在回答里提一句将军。\n"
        "- 不允许走完之后{foe}能吃你的将。\n\n"
        "其余取舍得失由你判断。\n\n"
        "## 回答\n\n"
        "三五句话，用中文：是否被将军、走哪一手、走完之后将为何安全。被将军时这一手必须是解将。不要复述棋盘；复盘日志留下的是 `note`，不是这段回答。\n"
        "走完再看{foe_king}：若它还在盘上，且你有子下一手能吃它，回答中必须出现「将军」二字。\n"
        "明子按自身算；没动过的暗子按起始格兵种算；刚被翻开的暗子新兵种未知，不能作为喊将军的理由。\n"
        "吃将直接获胜，不要喊将军。"
    ).format(**words)


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
    """Red defaults to DeepSeek, Black to Qwen. Either side can be overridden."""
    prefix = "CHESS_{}_".format(side.upper())
    provider = os.environ.get(prefix + "PROVIDER", "").strip().lower()
    model = os.environ.get(prefix + "MODEL", "").strip()
    if not provider and not model:
        if side == "red":
            return "deepseek", os.environ.get("DEEPSEEK_MODEL", "deepseek-flash")
        return "qwen", os.environ.get("QWEN_MODEL", "qwen3.8-flash")
    if not provider:
        provider = "qwen" if model.startswith(("qwen", "glm")) else "deepseek"
    if not model:
        if provider == "qwen":
            model = os.environ.get("QWEN_MODEL", "qwen3.8-flash")
        else:
            model = os.environ.get("DEEPSEEK_MODEL", "deepseek-flash")
    return provider, model


class qwen_chat(ChatOpenAI):
    """Keeps Qwen's reasoning_content.

    Thinking mode plus a tool call requires that field on the assistant message
    in the next request. ChatOpenAI drops it, and the follow-up call then stalls.
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


def thinking_body(provider, on):
    if provider == "qwen":
        return {"enable_thinking": on}
    return {"thinking": {"type": "enabled" if on else "disabled"}}


def make_model(side, thinking=None):
    provider, model = side_setup(side)
    on = thinking_on() if thinking is None else thinking
    if provider == "qwen":
        key = os.environ.get("QWEN_API_KEY")
        base = os.environ.get("QWEN_BASE_URL")
        if not key or not base:
            raise RuntimeError("QWEN_API_KEY and QWEN_BASE_URL are required for {}.".format(side))
        chat = qwen_chat(
            model=model,
            api_key=key,
            base_url=base,
            temperature=0,
            timeout=120,
            max_retries=1,
            extra_body=thinking_body(provider, on),
        )
    elif provider == "deepseek":
        kwargs = {
            "model": model,
            "temperature": 0,
            "timeout": 120,
            "max_retries": 1,
            "extra_body": thinking_body(provider, on),
        }
        base = os.environ.get("DEEPSEEK_BASE_URL")
        if base:
            kwargs["api_base"] = base
        chat = ChatDeepSeek(**kwargs)
    else:
        raise RuntimeError("Unknown provider '{}' for {}.".format(provider, side))
    return chat, "{}:{}".format(provider, model)


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
        "源格上原显示 {}。".format(src or "空"),
        "那枚子现在在落点显示为 {}。".format(dst or "空"),
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
    if result == "red_win":
        lines.append("红方胜。")
    elif result == "black_win":
        lines.append("黑方胜。")
    return "\n".join(lines)


async def speak(chat, provider, text):
    """One short line. Thinking stays off so the game is not left waiting."""
    extra = thinking_body(provider, False)
    message = await chat.bind(extra_body=extra).ainvoke([{"role": "user", "content": text}])
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


async def write_aside(chat, provider, outcome, earlier):
    """Private reaction after the move has resolved. Thinking stays off."""
    lead = ""
    if earlier:
        lead = "你之前的内心笔记：\n{}\n\n".format("\n".join(earlier[-6:]))
    text = (
        lead
        + "刚刚发生的、已经确认的事实：\n"
        + outcome
        + "\n\n用中文写两三句内心戏，回应这些事实。"
        "如果暗子翻出来是对手的子，或者你吃了自己的暗子，就针对这点反应。"
        "这些话只有你自己看。凡是上面没写明的暗子兵种，一律不要猜、不要说出。"
        "不要替对手设想下一步。"
    )
    return await speak(chat, provider, text)


async def tool_json(tools, name):
    raw = await tools[name].ainvoke({})
    return json.loads(message_text(raw))


async def run_turn(agent, side, content, thread_id):
    """Stream one move. Returns the play result the stream reported."""
    played = {"ok": False}
    async for update in agent.astream(
        {"messages": [{"role": "user", "content": content}]},
        {
            "configurable": {"thread_id": thread_id},
            "recursion_limit": 12,
        },
        stream_mode="updates",
    ):
        for step in update.values():
            messages = step.get("messages", []) if isinstance(step, dict) else []
            if not isinstance(messages, list):
                messages = [messages]
            for message in messages:
                show(message, played, side)
        if played["ok"]:
            break
    return played


async def session(adapter):
    tools = {tool.name: tool for tool in await adapter.list_tools()}
    emit("status", "tools: {}".format(", ".join(sorted(tools))))
    shared = list(tools.values())
    agents = {}
    chats = {}
    providers = {}
    labels = {}
    notes = {"red": [], "black": []}
    quick_agents = {}
    for side in ("red", "black"):
        chat, label = make_model(side)
        chats[side] = chat
        providers[side] = side_setup(side)[0]
        labels[side] = label
        agents[side] = create_react_agent(
            chat, shared, prompt=prompt_for(side), checkpointer=MemorySaver()
        )
    emit(
        "status",
        "red {}  vs  black {}, thinking {}".format(
            labels["red"], labels["black"], "on" if thinking_on() else "off"
        ),
    )
    if LOG:
        LOG.setup(labels, thinking_on(), move_seconds())
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
            for side in ("red", "black"):
                if not state.get("{}_ai".format(side)):
                    continue
                try:
                    text = await speak(
                        chats[side],
                        providers[side],
                        summary_text(side, result, state.get("diagram", ""), notes[side]),
                    )
                except Exception as exc:
                    emit("error", str(exc), side)
                    continue
                if text:
                    emit("summary", text, side)
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
                    text = await speak(chats[side], providers[side], taunt_text(side))
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
                run_turn(
                    agents[side], side, content, "{}-{}".format(side, turns[side])
                ),
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
                quick_chat, _ = make_model(side, thinking=False)
                quick_agents[side] = create_react_agent(
                    quick_chat,
                    shared,
                    prompt=prompt_for(side),
                    checkpointer=MemorySaver(),
                )
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
            note = await write_aside(chats[side], providers[side], outcome, notes[side])
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
    if not os.environ.get("DEEPSEEK_API_KEY"):
        emit("error", "DEEPSEEK_API_KEY is not set. Put it in the repository .env.")
        return 1
    LOG = game_log()
    announced = False
    while True:
        try:
            async with MCPAdapter(MCP_URL) as adapter:
                announced = False
                await session(adapter)
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
