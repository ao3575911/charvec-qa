# charvec-qa Cheatsheet

Copy and paste these commands from the repository root.

## Build Everything

```sh
make
```

## Run Tests

```sh
make test
```

## Build A Model

```sh
./build/charvec build -i examples/dataset.txt -o build/sample.cvec
```

## Inspect A Model

```sh
./build/charvec inspect -m build/sample.cvec
```

## Ask Sample Questions

```sh
./build/charvec ask -m build/sample.cvec -q "What is charvec-qa?"
./build/charvec ask -m build/sample.cvec -q "Who created C?"
./build/charvec ask -m build/sample.cvec -q "What is one-hot encoding?"
./build/charvec ask -m build/sample.cvec -q "When should I use this project?"
./build/charvec ask -m build/sample.cvec -q "What does inspect show?"
```

## Clean Build Artifacts

```sh
make clean
```

## Debug Build With Sanitizers

```sh
mkdir -p build
cc -std=c99 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g -Iinclude src/main.c src/charvec.c -o build/charvec_asan
./build/charvec_asan build -i examples/dataset.txt -o build/asan.cvec
./build/charvec_asan ask -m build/asan.cvec -q "Who created C?"
```

## Create Your Own Dataset

```sh
cp examples/dataset.txt examples/my_dataset.txt
$EDITOR examples/my_dataset.txt
./build/charvec build -i examples/my_dataset.txt -o build/my_dataset.cvec
./build/charvec ask -m build/my_dataset.cvec -q "Your exact question here"
```

