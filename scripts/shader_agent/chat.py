"""Multi-turn shader editor chat.

LearnOpenGL has to be running on the Shader Editor scene, which serves MCP on
a local port. Launch it with LEARNOPENGL_SCENE=shader_editor, or pick Shader
Editor in Scene Selection. The in-app panel starts this script itself and
passes SHADER_EDITOR_MCP_URL, because the editor takes 127.0.0.1:8765 when it
is free and any free port otherwise. From a terminal, read the URL in the
panel's built-in uniforms section and put it in SHADER_EDITOR_MCP_URL.

The API key is read from the repository .env (DEEPSEEK_API_KEY,
DEEPSEEK_BASE_URL, optional DEEPSEEK_MODEL).

    .venv/bin/python scripts/shader_agent/chat.py
"""

import asyncio
import json
import os
import sys
import threading
import warnings

# The in-app panel reads stdout as JSON lines. Drop the import-time warnings
# before they land in that stream.
if "--ui" in sys.argv:
    warnings.filterwarnings("ignore")

from langchain.agents import create_agent
from langchain.agents.middleware import AgentMiddleware, SummarizationMiddleware
from langchain.mcp import MCPAdapter
from langchain_core.messages import AIMessage, ToolMessage
from langchain_deepseek import ChatDeepSeek
from langgraph.checkpoint.memory import MemorySaver
from langgraph.graph.message import REMOVE_ALL_MESSAGES, RemoveMessage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# The machine proxy (macOS system proxy) returns 502 for this loopback server.
os.environ["NO_PROXY"] = "127.0.0.1,localhost"
os.environ["no_proxy"] = "127.0.0.1,localhost"
MCP_URL = os.environ.get("SHADER_EDITOR_MCP_URL", "http://127.0.0.1:8765/mcp")
CATALOG_URI = "shader-editor://uniforms"


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


def preview(value, limit=160):
    text = value if isinstance(value, str) else json.dumps(value, ensure_ascii=False)
    if len(text) <= limit:
        return text
    return text[:limit] + "…"


def message_text(content):
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        parts = []
        for block in content:
            if isinstance(block, str):
                parts.append(block)
            elif isinstance(block, dict):
                parts.append(block.get("text", ""))
            else:
                parts.append(getattr(block, "text", "") or "")
        return "".join(parts)
    return str(content)


UI = "--ui" in sys.argv


def parent_gone():
    return os.getppid() == 1


def emit(kind, text="", path=""):
    """One JSON line. The in-app panel reads these; a terminal prints text."""
    if not UI:
        if kind == "tool":
            print("  tool {}".format(text))
        elif kind == "agent":
            print("agent> {}".format(text))
        elif kind == "preview":
            print("  preview {}".format(path))
        elif kind == "error":
            print(text, file=sys.stderr)
        elif kind == "status":
            print(text)
        elif kind == "ready":
            print()
        return
    payload = {"kind": kind}
    if text:
        payload["text"] = text
    if path:
        payload["path"] = path
    sys.stdout.write(json.dumps(payload, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def show(message):
    if isinstance(message, AIMessage):
        calls = getattr(message, "tool_calls", None) or []
        for call in calls:
            emit("tool", "{} {}".format(call["name"], preview(call.get("args", {}))))
        text = message_text(message.content)
        if text.strip():
            emit("agent", text.strip())
    elif isinstance(message, ToolMessage):
        body = message_text(message.content)
        try:
            payload = json.loads(body)
        except (TypeError, json.JSONDecodeError):
            payload = None
        if isinstance(payload, dict) and payload.get("path"):
            emit("preview", path=payload["path"])
        elif isinstance(payload, dict) and payload.get("error"):
            emit("error", payload["error"])


async def catalog(adapter):
    contents = await adapter.client.read_resource(CATALOG_URI)
    return "".join(block.text for block in contents if hasattr(block, "text"))


def is_image_block(block):
    if isinstance(block, dict):
        kind = block.get("type")
        return kind in ("image", "image_url") or "image_url" in block
    kind = getattr(block, "type", None)
    return kind in ("image", "image_url") or getattr(block, "image_url", None)


def without_images(content):
    if not isinstance(content, list):
        return content
    kept = [block for block in content if not is_image_block(block)]
    if len(kept) == len(content):
        return content
    return kept or [{"type": "text", "text": "[earlier preview removed]"}]


class LatestPreviewMiddleware(AgentMiddleware):
    """Keeps only the newest capture_preview image in the thread.

    Each preview is a whole framebuffer, so a few of them fill the window before
    summarization gets a chance to run. Older ones are replaced with a line of
    text; the newest stays, because that is the frame the reply describes.
    """

    def before_model(self, state, runtime):
        messages = list(state["messages"])
        last_image = -1
        for index, message in enumerate(messages):
            content = getattr(message, "content", None)
            if isinstance(content, list) and any(is_image_block(b) for b in content):
                last_image = index
        if last_image < 0:
            return None
        stripped = []
        for index, message in enumerate(messages):
            content = getattr(message, "content", None)
            if index == last_image or not isinstance(content, list):
                stripped.append(message)
                continue
            trimmed = without_images(content)
            if trimmed is content:
                stripped.append(message)
                continue
            copied = message.model_copy(deep=True)
            copied.content = trimmed
            stripped.append(copied)
        if all(a is b for a, b in zip(messages, stripped)):
            return None
        return {"messages": [RemoveMessage(id=REMOVE_ALL_MESSAGES), *stripped]}


def make_model():
    # capture_preview returns an image block. deepseek-chat rejects that;
    # deepseek-flash is the vision model that reads it.
    kwargs = {"model": os.environ.get("DEEPSEEK_MODEL", "deepseek-flash"), "temperature": 0}
    base = os.environ.get("DEEPSEEK_BASE_URL")
    if base:
        kwargs["api_base"] = base
    return ChatDeepSeek(**kwargs)


async def chat():
    load_env(os.path.join(ROOT, ".env"))
    if not os.environ.get("DEEPSEEK_API_KEY"):
        emit("error", "DEEPSEEK_API_KEY is not set. Put it in {}.".format(os.path.join(ROOT, ".env")))
        return 1

    emit("status", "Connecting to {} …".format(MCP_URL))
    try:
        async with MCPAdapter(MCP_URL) as adapter:
            tools = await adapter.list_tools()
            emit("status", "tools: {}".format(", ".join(tool.name for tool in tools)))
            notes = await catalog(adapter)
            prompt = (
                "You edit a live GLSL shader in the LearnOpenGL shader editor. "
                "The user watches the viewport.\n\n"
                "Read the current source with get_shader before rewriting it. "
                "set_shader takes the full source of a stage; pass an empty "
                "string to leave a stage alone. After a change, call "
                "capture_preview and describe the image it returns. Use only "
                "the built-in uniforms below.\n\n"
                "Replies are one or two sentences: what changed, and what the "
                "frame looks like. No preamble, no list, no source dump. "
                "End every reply with 喵～\n\n"
                + notes
            )
            model = make_model()
            agent = create_agent(
                model,
                tools,
                system_prompt=prompt,
                checkpointer=MemorySaver(),
                middleware=[
                    # Strips old previews first, so the summary trigger counts
                    # what is actually sent rather than the removed images.
                    LatestPreviewMiddleware(),
                    SummarizationMiddleware(
                        model,
                        trigger=("fraction", 0.5),
                        keep=("messages", 6),
                    ),
                ],
            )
            config = {"configurable": {"thread_id": "shader-editor"}}
            while True:
                if parent_gone():
                    return 0
                emit("ready")
                try:
                    if UI:
                        raw = await asyncio.to_thread(sys.stdin.readline)
                        if raw == "":
                            return 0
                        line = raw.strip()
                    else:
                        line = (await asyncio.to_thread(input, "you> ")).strip()
                except EOFError:
                    return 0
                if not line:
                    continue
                if line in ("exit", "quit"):
                    return 0
                try:
                    # "updates" yields each graph step as it finishes, so a tool
                    # call shows up before the next model reply instead of the
                    # whole turn arriving at once.
                    async for update in agent.astream(
                        {"messages": [{"role": "user", "content": line}]},
                        config,
                        stream_mode="updates",
                    ):
                        for step in update.values():
                            messages = step.get("messages", []) if isinstance(step, dict) else []
                            if not isinstance(messages, list):
                                messages = [messages]
                            for message in messages:
                                show(message)
                except Exception as exc:
                    emit("error", str(exc))
    except KeyboardInterrupt:
        return 0
    except Exception as exc:
        emit("error", "Could not reach the shader editor: {}".format(exc))
        return 1


if __name__ == "__main__":
    if UI:
        # httpx looks up the system proxy through the Security framework.
        # On the process main thread macOS logs
        # "This method should not be called on the main thread...".
        worker = threading.Thread(target=lambda: os._exit(asyncio.run(chat())), name="shader-chat")
        worker.start()
        worker.join()
    else:
        raise SystemExit(asyncio.run(chat()))
