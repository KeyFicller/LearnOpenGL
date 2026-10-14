"""Multi-turn shader editor chat.

LearnOpenGL has to be running on the Shader Editor scene, which serves MCP at
http://127.0.0.1:8765/mcp. Launch it with LEARNOPENGL_SCENE=shader_editor, or
pick Shader Editor in Scene Selection.

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

from langchain.mcp import MCPAdapter
from langchain_core.messages import AIMessage, ToolMessage
from langchain_deepseek import ChatDeepSeek
from langgraph.checkpoint.memory import MemorySaver
from langgraph.prebuilt import create_react_agent

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
            agent = create_react_agent(
                make_model(), tools, prompt=prompt, checkpointer=MemorySaver()
            )
            config = {"configurable": {"thread_id": "shader-editor"}}
            while True:
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
