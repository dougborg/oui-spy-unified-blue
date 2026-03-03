set shell := ["bash", "-cu"]

default:
    @just --list

setup:
    pio pkg install -d .

setup-dev:
    python3 -m pip install -r requirements-dev.txt

build:
    pio run

upload:
    pio run -t upload

monitor:
    pio device monitor

clean:
    pio run -t clean

test:
    python3 -m pytest -q

test-cpp:
    pio test -e native

coverage-cpp:
    pio test -e native
    python3 -m gcovr --root . --txt --print-summary \
        --filter src/opendroneid.c \
        --filter src/wifi.c \
        --filter src/modules/detector_logic.cpp \
        --filter src/modules/flockyou_logic.cpp \
        --filter src/hal/buzzer_logic.cpp \
        --filter src/hal/neopixel_logic.cpp \
        --xml-pretty --xml coverage-native.xml

lint:
    pre-commit run --all-files

analyze-cpp:
        cppcheck --quiet --error-exitcode=1 \
            --enable=warning,performance \
            --inline-suppr --suppress=missingIncludeSystem \
            -I src src/main.cpp src/hal src/modules src/web src/opendroneid.c src/wifi.c

quality:
    just lint
    just test
    just test-cpp
    just analyze-cpp

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
