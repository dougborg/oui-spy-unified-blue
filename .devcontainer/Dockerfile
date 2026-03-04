FROM mcr.microsoft.com/devcontainers/base:ubuntu

ENV DEBIAN_FRONTEND=noninteractive

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
    && curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y --no-install-recommends nodejs \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

USER vscode

RUN python3 -m venv /home/vscode/.venv/pio \
    && /home/vscode/.venv/pio/bin/pip install --upgrade pip \
    && /home/vscode/.venv/pio/bin/pip install platformio esptool pyserial

ENV PATH="/home/vscode/.venv/pio/bin:/home/vscode/.local/bin:${PATH}"

COPY --chown=vscode:vscode requirements-dev.txt /tmp/pio-project/requirements-dev.txt
COPY --chown=vscode:vscode platformio.ini /tmp/pio-project/platformio.ini
COPY --chown=vscode:vscode .pre-commit-config.yaml /tmp/pio-project/.pre-commit-config.yaml

RUN /home/vscode/.venv/pio/bin/pip install -r /tmp/pio-project/requirements-dev.txt

RUN cd /tmp/pio-project && git init && pre-commit install-hooks && rm -rf .git

RUN pio pkg install -d /tmp/pio-project \
    && rm -rf /tmp/pio-project
