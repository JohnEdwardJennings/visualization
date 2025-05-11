.PHONY: build clean run 

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
	cp quadratic_bspline_example_10x10x4.vtu build
	cd build && make -j $(num_threads) && ./$(executable_name) ./$(file)

test:
	cd src && python3 parser.py --path ../$(file) && mv data.json ..

build:
	mkdir -p build && cd build && cmake .. 

install: 
	pip install -r requirements.txt

test: 
	python3 -m unittest discover tests	

clean: 
	rm -rf build
	[ -f data.json ] && rm data.json || true
	[ -f hexahedral_mesh.vtu ] && rm hexahedral_mesh.vtu || true
