.PHONY: all build clean run

GREEN = \033[0;32m
RESET = \033[0m

define green
	@printf "$(GREEN)%s$(RESET)\n" "$(1)"
endef

all: run

build:
	$(call green,=== Building Audio Target ===)
	@mkdir -p build
	@g++ src/main.cpp src/audio.cpp -o build/main -lasound

clean:
	$(call green,=== Cleaning Build/ Target ===)
	@rm -f build/main

run: build
	$(call green,=== Running Audio Target ===)
	@./build/main