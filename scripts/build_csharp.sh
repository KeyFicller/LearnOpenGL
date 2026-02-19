#!/bin/bash
# Build C# scripts for Mono embedding.
# Build to staging dir first; only on success copy to runtime dir (for safe hot-reload).
set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$SCRIPT_DIR"

STAGING_DIR="$SCRIPT_DIR/csharp/bin_staging"
RUNTIME_DIR="$PROJECT_ROOT/runtime"

mkdir -p "$STAGING_DIR"
mkdir -p "$RUNTIME_DIR"

if command -v mcs &>/dev/null; then
  echo "Building Scripts.dll with mcs (staging)..."
  mcs -target:library -out:"$STAGING_DIR/Scripts.dll" csharp/*.cs
  cp -f "$STAGING_DIR/Scripts.dll" "$RUNTIME_DIR/Scripts.dll"
  echo "Done: $RUNTIME_DIR/Scripts.dll"
elif command -v dotnet &>/dev/null; then
  echo "Building Scripts.dll with dotnet (staging)..."
  dotnet build csharp/Scripts.csproj -c Release -o "$STAGING_DIR"
  cp -f "$STAGING_DIR/Scripts.dll" "$RUNTIME_DIR/Scripts.dll"
  echo "Done: $RUNTIME_DIR/Scripts.dll"
else
  echo "Error: No C# compiler (mcs or dotnet) found" >&2
  exit 1
fi
