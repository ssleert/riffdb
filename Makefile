prepare:
	cmake -B ./build -DCMAKE_BUILD_TYPE=${type} -DCMAKE_COLOR_MAKEFILE=ON

build:
	cmake --build ./build --parallel

format:
	clang-format --style=Mozilla -i `find ./src | grep '\.c' | grep -v Greeting`

lint:
	clang-tidy `find ./src | grep '\.c'` -fix

clean:
	rm -rf ./build

.PHONY: prepare build format clean
all: prepare
