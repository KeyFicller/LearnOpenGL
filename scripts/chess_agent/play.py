"""Two reveal-chess players.

LearnOpenGL has to be on the Reveal Chess scene, which serves MCP at
http://127.0.0.1:8766/mcp. The in-app panel shows each side's thoughts.
There is no conversation: whichever side is marked AI plays when it is
that side's turn. Red and Black are separate models and do not share a
transcript, so a covered piece stays hidden from both.

The API key is read from the repository .env (DEEPSEEK_API_KEY,
DEEPSEEK_BASE_URL, optional DEEPSEEK_MODEL).
"""

import asyncio
import json
import os
import sys
import threading
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
        "me": "Red" if red else "Black",
        "foe": "Black" if red else "Red",
        "mine": "红" if red else "黑",
        "theirs": "黑" if red else "红",
        "king": "红将, which starts at (0, 4)" if red else "黑帅, which starts at (9, 4)",
        "foe_king": "黑帅" if red else "红将",
        "half": "0–4" if red else "5–9",
        "foe_half": "5–9" if red else "0–4",
        "forward": "toward row 9 (row + 1)" if red else "toward row 0 (row − 1)",
    }
    return (
        "You are {me} in a 揭棋 (reveal xiangqi) game. Red moves first. "
        "You play against {foe}, a separate model.\n\n"
        "## Turn\n\n"
        "1. Call `board` once and read `diagram`. The message describes {foe}'s previous move when there is one. "
        "Use that, not memory, to judge what was revealed, what was captured, and whether your king is in check.\n"
        "2. Work out the position with the checklist under \"Choosing a move\". There is no list of legal moves. "
        "You must check each candidate against the rules yourself.\n"
        "3. Call `play(from_row, from_col, to_row, to_col)`.\n"
        "4. On an error, the board is unchanged. Read the whole error before anything else. "
        "It says what is on the source square and why the move failed. If the piece is yours, it also lists every square that piece can reach. "
        "Never resend a move that failed. Either pick a listed square that passes your safety checks, or move a different piece. "
        "Do not call `board` again just because of an error. "
        "If the error says the square is empty or not yours, you misread the diagram, so recount the row and column.\n"
        "5. After a successful `play`, stop. Do not call `board` again.\n\n"
        "## Reading the board\n\n"
        "`diagram` is a markdown table. Split a body row on `|`. The first field is the row number, "
        "0 at the top through 9 at the bottom. The next nine fields are columns 0 through 8, left to right. "
        "A blank field is an empty square and still counts as a column. A square is written (row, col).\n\n"
        "Red starts on rows 0–4 at the top. Black starts on rows 5–9 at the bottom. The river lies between rows 4 and 5.\n\n"
        "| Cell | Meaning |\n"
        "| --- | --- |\n"
        "| blank | Empty |\n"
        "| 红· | Covered piece on rows 0–4. Red moves it. Its type and true color are hidden. |\n"
        "| 黑· | Covered piece on rows 5–9. Black moves it. Its type and true color are hidden. |\n"
        "| 红将 红仕 红相 红车 红马 红炮 红兵 | Face-up red piece |\n"
        "| 黑帅 黑士 黑象 黑车 黑马 黑炮 黑卒 | Face-up black piece |\n\n"
        "The red king is 红将 and the black king is 黑帅. This is the reverse of usual xiangqi, so always trust the 红 or 黑 prefix. "
        "车, 马 and 炮 share one name on both sides.\n\n"
        "## Your pieces\n\n"
        "- Every face-up piece with the {mine} prefix, wherever it stands, including your king, {king}.\n"
        "- Every covered {mine}·. Covered pieces never move without flipping, so a {mine}· always sits on its starting square on rows {half}.\n"
        "- The 30 hidden pieces were shuffled across both colors. When you flip a {mine}·, it may turn out to be a {theirs} piece. "
        "From then on it belongs to {foe}.\n"
        "- Never guess or name a covered piece's type. Nobody knows it, and `play` does not reveal it.\n\n"
        "## Rules this board enforces\n\n"
        "| | Rule |\n"
        "| --- | --- |\n"
        "| 暗子 | A covered piece moves as the piece that starts on its square (table below), not as its hidden type. |\n"
        "| 翻明 | Moving a covered piece flips it face up on its destination. After that it moves as its real type and belongs to its real color. |\n"
        "| 明子 | A face-up 仕/士 or 相/象 may go anywhere on the board, with no palace or river limit. A face-up king stays inside its own palace. |\n"
        "| 吃子 | You may land on any piece except a face-up piece of your own color. That includes every covered piece, even your own {mine}·. |\n"
        "| 无将军 | The engine does not reject a move that leaves your king under attack. "
        "That is not permission to stay there. The opponent can capture the king on the next move and win. |\n"
        "| 照面 | The two kings may face each other. There is no flying-general rule. |\n"
        "| 胜负 | Capturing the enemy king wins immediately. |\n\n"
        "## Starting squares\n\n"
        "A covered piece moves as the piece listed for its square.\n\n"
        "| Row | Columns 0–8 |\n"
        "| --- | --- |\n"
        "| 0 | 车 马 相 仕 将 仕 相 马 车 |\n"
        "| 2 | 炮 on 1 and 7 |\n"
        "| 3 | 兵 on 0, 2, 4, 6, 8 |\n"
        "| 6 | 卒 on 0, 2, 4, 6, 8 |\n"
        "| 7 | 炮 on 1 and 7 |\n"
        "| 9 | 车 马 象 士 帅 士 象 马 车 |\n\n"
        "## Moves\n\n"
        "| Piece | Rule |\n"
        "| --- | --- |\n"
        "| 车 | Slides along a row or column through empty squares. May stop on the first occupied square it reaches if that is a legal capture. |\n"
        "| 马 | One orthogonal step, then one diagonal step outward. Blocked if the first orthogonal square is occupied: (r±1, c) for a two-row move, (r, c±1) for a two-column move. |\n"
        "| 炮 | Moves like 车 but cannot capture that way. To capture, exactly one piece of any kind stands between it and the target on the same line. |\n"
        "| 相 / 象 | Two squares diagonally. Blocked if the middle square is occupied. |\n"
        "| 仕 / 士 | One square diagonally. A covered one on a guard square may only step into that back rank's palace, (1, 4) or (8, 4). |\n"
        "| 将 / 帅 | One square orthogonally inside its palace: Red rows 0–2, Black rows 7–9, columns 3–5. |\n"
        "| 兵 / 卒 | Covered: one step toward the other side only. Face up: Red forward is row + 1, Black forward is row − 1. "
        "Once across the river (red soldier on rows 5–9, black soldier on rows 0–4) it may also step sideways. It never steps back. |\n\n"
        "Your forward is {forward}.\n\n"
        "## Choosing a move\n\n"
        "Go through these in order, and stop at the first step that settles the move.\n\n"
        "1. Win. If any of your pieces can legally capture {foe_king}, play it.\n"
        "2. In check, escape. Before any other idea, find every {foe} piece that could capture your king on its next move: "
        "车 and 炮 on the king's row and column, 马 an L-shape away with its leg square empty, face-up 兵/卒, 仕/士 and 相/象, "
        "and covered {theirs}· whose starting-square move reaches the king. "
        "If any such piece exists, you are in check. The move you play must get the king out, and then you stop. "
        "Do not develop, do not take an unrelated piece, and do not only mention the check in your reply.\n"
        "   Escape in this order. Play the first one that leaves no {foe} piece able to capture the king:\n"
        "   - Move the king one square inside its palace, off the attack line, onto a square that piece cannot reach.\n"
        "   - Capture the checking piece.\n"
        "   - Interpose on the line of a 车 or 炮, or step onto the 马's leg square. "
        "A block only counts if that checker can no longer capture the king. Filling a 炮's line can create the screen it needed, so check the count.\n"
        "   If several escapes work, prefer moving the king. If none works, the king is lost; still capture the checker if you can.\n"
        "3. Do not blunder. For each candidate, check that after it {foe} cannot capture your king. "
        "Moving a piece off a line can open a 车 attack or remove a 炮 screen. "
        "Flipping a covered piece beside your king is a gamble, because it may turn out to be {theirs}.\n"
        "4. Material. Capture face-up {foe} pieces, most valuable first: 车, then 马 and 炮, then the rest. "
        "Before you land, check whether {foe} can recapture on that square, and avoid trading a strong piece for a weak one.\n"
        "5. Covered targets. A {theirs}· on rows {foe_half} could be either color. Capturing one denies {foe} a move or a 炮 screen, "
        "but it is a gamble on material, so prefer known face-up targets. Capturing your own {mine}· is legal but almost never useful.\n"
        "6. Otherwise develop. Flip or move a piece to a square where {foe} cannot capture it, preferably one that adds pressure or opens lines. "
        "Covered pieces on the 车 and 炮 starting squares usually have the most moves. "
        "For example, at the start a covered 炮 on (2, 1) can capture (9, 1) by jumping (7, 1), "
        "and one on (7, 1) can capture (0, 1) by jumping (2, 1).\n\n"
        "## Reply\n\n"
        "Think in Chinese, a few sentences: whether your king is in check, the move you are playing, and why the king is safe after it. "
        "If the king is in check, that move has to be the escape. Do not dump the board.\n"
        "After the move, look at {foe_king}. If it is still on the board and one of your pieces can capture it on the next move, "
        "the reply must contain the word 将军. Count a face-up piece as itself. Count a covered piece that you did not move as the piece on its starting square. "
        "A covered piece you just moved has flipped, and its new type is unknown, so it cannot be the reason you say 将军. "
        "Capturing the king wins outright. Do not say 将军 for that capture."
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


def emit(kind, text="", side=""):
    text = text.strip()
    if not text:
        return
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
        for call in getattr(message, "tool_calls", None) or []:
            emit(
                "tool",
                "{} {}".format(call.get("name", ""), json.dumps(call.get("args", {}), ensure_ascii=False)),
                side,
            )
        text = message_text(message.content).strip()
        if text:
            emit("agent", text, side)
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


def thinking_body(provider, on):
    if provider == "qwen":
        return {"enable_thinking": on}
    return {"thinking": {"type": "enabled" if on else "disabled"}}


def make_model(side):
    provider, model = side_setup(side)
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
            extra_body=thinking_body(provider, thinking_on()),
        )
    elif provider == "deepseek":
        kwargs = {
            "model": model,
            "temperature": 0,
            "timeout": 120,
            "max_retries": 1,
            "extra_body": thinking_body(provider, thinking_on()),
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
    lines = ["Opponent's last move: ({}, {}) -> ({}, {}).".format(fr, fc, tr, tc)]
    if src:
        lines.append("The piece that moved showed {}.".format(src))
    if now:
        lines.append("It is now {} on ({}, {}).".format(now, tr, tc))
    if before is not None:
        if not taken:
            lines.append("The destination was empty. Nothing was captured.")
        elif taken.endswith("·"):
            lines.append(
                "It captured a covered piece ({}). The type was not revealed. Do not name it.".format(
                    taken
                )
            )
        else:
            lines.append("It captured {}.".format(taken))
    lines.append(
        "Judge the position from this move: what was revealed, what was captured, and whether your king is in check."
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
        "You played ({}, {}) -> ({}, {}).".format(fr, fc, tr, tc),
        "The source square showed {}.".format(src or "nothing"),
        "That piece now sits on the destination as {}.".format(dst or "nothing"),
    ]
    if src.endswith("·"):
        if dst.startswith(mine):
            lines.append("It was covered and flipped face up. It is your piece.")
        elif dst:
            lines.append(
                "It was covered and flipped face up as an opponent piece. It belongs to the opponent now."
            )
    elif src:
        lines.append("It was already face up.")
    if not taken:
        lines.append("The destination was empty. You captured nothing.")
    elif taken.endswith("·"):
        whose = "your own" if taken.startswith(mine) else "an opponent"
        lines.append(
            "You captured {} covered piece ({}). Its type was never revealed. Do not name it.".format(
                whose, taken
            )
        )
    else:
        lines.append("You captured the face-up piece {}.".format(taken))
    result = payload.get("result") or after.get("result")
    if result == "red_win":
        lines.append("Red wins.")
    elif result == "black_win":
        lines.append("Black wins.")
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
        lead = "Your earlier private notes:\n{}\n\n".format("\n".join(earlier[-6:]))
    text = (
        lead
        + "What just happened, already resolved:\n"
        + outcome
        + "\n\nWrite two or three sentences of private thoughts in Chinese, reacting to these facts. "
        "If a covered piece flipped into an opponent piece, or you captured your own covered piece, react to that. "
        "Only you will read this. Do not name a covered piece whose type is not written above. "
        "Do not suggest the opponent's move."
    )
    return await speak(chat, provider, text)


async def tool_json(tools, name):
    raw = await tools[name].ainvoke({})
    return json.loads(message_text(raw))


async def session(adapter):
    tools = {tool.name: tool for tool in await adapter.list_tools()}
    emit("status", "tools: {}".format(", ".join(sorted(tools))))
    shared = list(tools.values())
    agents = {}
    chats = {}
    providers = {}
    labels = {}
    notes = {"red": [], "black": []}
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
        played = {"ok": False}
        content = "Your turn."
        last = describe_last_move(
            state.get("diagram", ""), before_opponent, state.get("last_move")
        )
        if last:
            content += "\n\n" + last
        if notes[side]:
            content += "\n\nPrivate notes only you can see:\n" + "\n".join(notes[side][-6:])
        try:
            async for update in agents[side].astream(
                {"messages": [{"role": "user", "content": content}]},
                {
                    "configurable": {"thread_id": "{}-{}".format(side, turns[side])},
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
    load_env(os.path.join(ROOT, ".env"))
    if not os.environ.get("DEEPSEEK_API_KEY"):
        emit("error", "DEEPSEEK_API_KEY is not set. Put it in the repository .env.")
        return 1
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
