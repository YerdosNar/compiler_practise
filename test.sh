#!/usr/bin/env bash

red="\033[31m"
grn="\033[32m"
blu="\033[34m"
rst="\033[0m"

assert() {
        expected="$1"
        input="$2"
        ./tinyc "$input" > tmp.s || exit 1
        gcc -o tmp tmp.s || exit 1
        ./tmp
        actual="$?"
        if [ "$actual" = "$expected" ]; then
                echo -e "${grn}PASS: ${rst}$input => $actual"
        else
                echo -e "${red}FAIL: ${rst}$input => expected $expected, got $actual"
        fi
}

make

echo -e "${blu}Testing basic parser${rst}"
assert 0 '0'
assert 42 '42'
assert 7 '1 + 2 * 3'
assert 5 '10 - 2 - 3'
assert 9 '(1 + 2) * 3'
assert 3 '17 / 5'
assert 2 '17 % 5'
assert 7 '-3 + 10'
assert 5 '- -5'
assert 4 '2 * -3 + 10'
assert 1 '1 < 2'
assert 0 '2 < 1'
assert 1 '3 >= 3'
assert 1 '1 == 1'
assert 0 '1 != 1'
assert 1 '!0'
assert 0 '!5'
assert 2 '(1 < 2) + (3 == 3)'

echo ""
echo -e "${blu}Testing AND/OR${rst}"
assert 1 '1 && 1'
assert 0 '1 && 0'
assert 0 '0 && 1'
assert 1 '2 && 3'
assert 0 '0 || 0'
assert 1 '0 || 5'
assert 0 '0 && 1/0'
assert 1 '1 || 1/0'
assert 1 '1 || 0 && 0'
assert 1 '(0 || 1) && (1 && 2)'

echo -e "${blu}OK${rst}"
