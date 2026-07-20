# Convenience wrapper for the modern macOS build.

.PHONY: all clean run

all:
	$(MAKE) -C rogue

run:
	$(MAKE) -C rogue run

clean:
	$(MAKE) -C rogue clean
