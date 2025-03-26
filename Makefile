.PHONY: build clean run git_add add_file

executable_name = "Visualization"
num_threads = 4

# Extract the first argument as `file`
file = $(word 2, $(MAKECMDGOALS))
# Prevent Make from treating the argument as a target
%:
	@:

run:
	make build
	cp $(file) build
	cp src/parser.py build
	cd build && make -j $(num_threads) && ./$(executable_name) ./$(file)

test:
	cd src && python3 parser.py --path ../$(file) && mv data.json ..

build:
	mkdir -p build && cd build && cmake .. 

clean: 
	rm -rf build
	[ -f data.json ] && rm data.json || true
