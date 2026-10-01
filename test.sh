#!/usr/bin/env bash

red="\033[31m"
grn="\033[32m"
blu="\033[34m"
rst="\033[0m"

pass=0
fail=0

assert() {
        expected="$1"
        input="$2"
        if ! ./tinyc "$input" > tmp.s 2> tmp.err; then
                echo -e "${red}FAIL: ${rst}$input => compiler error:"
                cat tmp.err
                ((fail++))
                return
        fi
        gcc -o tmp tmp.s || exit 1
        ./tmp
        actual="$?"
        if [ "$actual" = "$expected" ]; then
                echo -e "${grn}PASS: ${rst}$actual <= $input"
                ((pass++))
        else
                echo -e "${red}FAIL: ${rst}$input => expected $expected, got $actual"
                ((fail++))
        fi
}

assert_error() {
        input="$1"
        ./tinyc "$input" > /dev/null 2>> tmp.err
        status="$?"
        if [ "$status" -eq 1 ]; then
                echo -e "${grn}PASS: ${rst}rejected: $input"
                ((pass++))
        else
                echo -e "${red}FAIL: ${rst}$input => expected rejection (exit 1), got exit $status"
                ((fail++))
        fi
}

make >/dev/null || exit 1

echo -e "${blu}Testing codegen${rst}"
assert 1      '{ return 1; }'
assert 42     '{ return 42; }'
assert 7      '{ return 1 + 2 * 3; }'
assert 5      '{ return 10 - 2 - 3; }'
assert 9      '{ return (1 + 2) * 3; }'
assert 3      '{ return 17 / 5; }'
assert 2      '{ return 17 % 5; }'
assert 7      '{ return -3 + 10; }'
assert 5      '{ return - -5; }'
assert 4      '{ return 2 * -3 + 10; }'
assert 1      '{ return 1 < 2; }'
assert 0      '{ return 2 < 1; }'
assert 1      '{ return 3 >= 3; }'
assert 1      '{ return 1 == 1; }'
assert 0      '{ return 1 != 1; }'
assert 1      '{ return !0; }'
assert 0      '{ return !5; }'
assert 2      '{ return (1 < 2) + (3 == 3); }'

echo -e "\n${blu}More stmt tests${rst}"
assert 3      '{ 1; 2; return 3; }'
assert 1      '{ return 1; return 2; }'
assert 5      '{ { return 5; } }'
assert 0      '{ 1 + 2; }'
assert 0      '{ }'

echo -e "\n${blu}Testing AND/OR${rst}"
assert 1      '{ return 1 && 1; }'
assert 0      '{ return 1 && 0; }'
assert 0      '{ return 0 && 1; }'
assert 1      '{ return 2 && 3; }'
assert 0      '{ return 0 || 0; }'
assert 1      '{ return 0 || 5; }'
assert 0      '{ return 0 && 1/0; }'
assert 1      '{ return 1 || 1/0; }'
assert 1      '{ return 1 || 0 && 0; }'
assert 1      '{ return (0 || 1) && (1 && 2); }'

echo -e "\n${blu}Testing error cases${rst}"
assert_error '{ return 1 }'
assert_error '{ 1 }'
assert_error '{ return 1;'
assert_error '{ { return 1; }'

if [ $fail -eq 0 ]; then
        echo -e "${blu}All tests passed${rst}"
else
        echo -e "${red}$fail/$((pass+fail)) not passed${rst}"
        exit 1
fi
