FROM node:22-slim AS node

FROM mcr.microsoft.com/devcontainers/base:ubuntu

ENV DEBIAN_FRONTEND=noninteractive

# Node.js — copy pre-built binaries instead of curl|bash + NodeSource apt repo
COPY --from=node /usr/local/bin/node /usr/local/bin/
COPY --from=node /usr/local/lib/node_modules /usr/local/lib/node_modules
RUN ln -sf ../lib/node_modules/npm/bin/npm-cli.js /usr/local/bin/npm \
    && ln -sf ../lib/node_modules/npm/bin/npx-cli.js /usr/local/bin/npx

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        build-essential \
        git \
        curl \
        ca-certificates \
        udev \
        clang-format \
        cppcheck \
        just \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

USER vscode

COPY --from=ghcr.io/astral-sh/uv:0.9.29 /uv /uvx /home/vscode/.local/bin/

ENV PATH="/home/vscode/.local/bin:${PATH}"

# --- Layer: Python dev deps + pre-commit hook envs ---
# Invalidated by: pyproject.toml, uv.lock, .pre-commit-config.yaml
COPY --chown=vscode:vscode pyproject.toml /tmp/pio-project/pyproject.toml
COPY --chown=vscode:vscode uv.lock /tmp/pio-project/uv.lock
COPY --chown=vscode:vscode .pre-commit-config.yaml /tmp/pio-project/.pre-commit-config.yaml

ENV UV_PROJECT_ENVIRONMENT="/home/vscode/.venv"

RUN cd /tmp/pio-project \
    && uv sync \
    && git init && pre-commit install-hooks && rm -rf .git

ENV PATH="/home/vscode/.venv/bin:${PATH}"

# --- Layer: PlatformIO toolchain + libraries (~2.5 GB) ---
# Invalidated by: platformio.ini only
COPY --chown=vscode:vscode platformio.ini /tmp/pio-project/platformio.ini

RUN pio pkg install -d /tmp/pio-project \
    && rm -rf /tmp/pio-project
