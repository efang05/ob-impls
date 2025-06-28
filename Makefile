PY_SOURCES := $(shell find . -type f -name '*.py')

.PHONY: install
install:
	conan install . --build=missing

.PHONY: build
build: install
	cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release -G Ninja
	cd build && cmake --build . -j

.PHONY: test
test: build
	cd build && ./main_tests

.PHONY: lint-check
lint-check:
	run-clang-tidy -j $(shell nproc) -p build
	flake8 $(PY_SOURCES)

.PHONY: format-check
format-check:
	find include src tst bench -name '*.cpp' -o -name '*.hpp' | xargs clang-format --style=file --Werror --dry-run
	black --check $(PY_SOURCES)
	
.PHONY: format
format:
	find include src tst bench -name '*.cpp' -o -name '*.hpp' | xargs clang-format --style=file -i
	run-clang-tidy -fix -j $(shell nproc) -p build
	black $(PY_SOURCES)

.PHONY: clean
clean:
	rm -rf build

