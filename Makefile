.PHONY: build clean run git_add add_file

executable_name = "Visualization"
num_threads = 4

run:
	make build
	cp data.txt build
	cp src/parser.py build
	cd build && make -j $(num_threads) && ./$(executable_name) ./data.txt 

test:
	cd src && python3 parser.py --path ../data.txt && mv data.json ..

build:
	mkdir -p build && cd build && cmake .. 

clean: 
	rm -rf build
	[ -f data.json ] && rm data.json || true
