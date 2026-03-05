FROM mcr.microsoft.com/devcontainers/base:ubuntu

ENV DEBIAN_FRONTEND=noninteractive

ARG NODE_MAJOR=22
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        python3 \
        python3-pip \
        python3-venv \
        python3-setuptools \
        python3-wheel \
        build-essential \
        git \
        curl \
        ca-certificates \
        udev \
        clang-format \
        cppcheck \
        just \
    && curl -fsSL https://deb.nodesource.com/setup_${NODE_MAJOR}.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

USER vscode

COPY --from=ghcr.io/astral-sh/uv:0.9.29 /uv /uvx /home/vscode/.local/bin/

ENV PATH="/home/vscode/.local/bin:${PATH}"

# Install dev dependencies into a persistent venv via uv
COPY --chown=vscode:vscode pyproject.toml /tmp/pio-project/pyproject.toml
COPY --chown=vscode:vscode uv.lock /tmp/pio-project/uv.lock
COPY --chown=vscode:vscode platformio.ini /tmp/pio-project/platformio.ini
COPY --chown=vscode:vscode .pre-commit-config.yaml /tmp/pio-project/.pre-commit-config.yaml

ENV UV_PROJECT_ENVIRONMENT="/home/vscode/.venv"

RUN cd /tmp/pio-project && uv sync

ENV PATH="/home/vscode/.venv/bin:${PATH}"

RUN cd /tmp/pio-project && git init && pre-commit install-hooks && rm -rf .git

RUN pio pkg install -d /tmp/pio-project \
    && rm -rf /tmp/pio-project
