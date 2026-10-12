# Development formatting commands; normal library builds use CMake.
.DEFAULT_GOAL := format-check
PYTHON ?= python3
CLANG_FORMAT ?=

.PHONY: format format-check clang-format clang-format-check
format clang-format:
	"$(PYTHON)" scripts/format.py --fix --clang-format "$(CLANG_FORMAT)"

format-check clang-format-check:
	"$(PYTHON)" scripts/format.py --check --clang-format "$(CLANG_FORMAT)"
