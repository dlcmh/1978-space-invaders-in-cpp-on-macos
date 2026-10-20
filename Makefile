.PHONY: app build check checks clean dmg docs-check docs-impact format icon open release run

BUILD_DIR ?= build/debug
RELEASE_DIR ?= build/release
CONFIG ?= Debug

build:
	cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=$(CONFIG)
	cmake --build $(BUILD_DIR)

release:
	cmake -S . -B $(RELEASE_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build $(RELEASE_DIR)

run: build
	./$(BUILD_DIR)/bin/spaceinvaders_app

checks: build
	./$(BUILD_DIR)/bin/spaceinvaders_checks

check:
	./scripts/check.sh

docs-check:
	python3 scripts/check_docs.py

docs-impact:
	python3 scripts/check_docs.py --changed

app:
	./scripts/build_app.sh

open: app
	open dist/SPACE-INVADERS.app

dmg: app
	hdiutil create -volname "SPACE INVADERS" -srcfolder "dist/SPACE-INVADERS.app" \
		-ov -format UDZO "dist/SPACE-INVADERS-1.0.dmg"

icon:
	python3 scripts/make_icon.py

format:
	./scripts/format.sh

clean:
	rm -rf build dist
