set shell := ["bash", "-cu"]

default:
    @just --list

setup:
    pio pkg install -d .

build:
    pio run

upload:
    pio run -t upload

monitor:
    pio device monitor

clean:
    pio run -t clean

erase:
    python flash.py --erase

flash bin="":
    if [[ -n "{{bin}}" ]]; then python flash.py "{{bin}}"; else python flash.py; fi

devcontainer-auto:
    bash .devcontainer/select-profile.sh auto

devcontainer-portable:
    bash .devcontainer/select-profile.sh portable

devcontainer-linux-usb:
    bash .devcontainer/select-profile.sh linux-usb
