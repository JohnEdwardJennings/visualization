.PHONY: build clean run git_add add_file

executable_name = "Visualization"
num_threads = 4

run:
	make build
	cd build && make -j $(num_threads) && ./$(executable_name) 

build:
	mkdir -p build && cd build && cmake .. 

clean: 
	rm -rf build
