#!/usr/bin/env bash

red="\033[31m"
grn="\033[32m"
blu="\033[34m"
rst="\033[0m"

pass=0
fail=0

assert() {
        status_code="$1"
        expected="$2"
        input="$3"

        ./tinyc "$input" > tmp.s 2>/dev/null
        actual="$?"
        # Handling error cases
        if [ $actual -ne 0 ]; then
                if [ "$actual" = "$status_code" ]; then
                        echo -e "${grn}PASS: ${rst}Crashed as expected $input"
                        ((pass++))
                        return
                fi
                exit 1
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

make >/dev/null || exit 1

echo -e "${blu}Testing codegen${rst}"
assert 0 1      '{ return 1; }'
assert 0 42     '{ return 42; }'
assert 0 7      '{ return 1 + 2 * 3; }'
assert 0 5      '{ return 10 - 2 - 3; }'
assert 0 9      '{ return (1 + 2) * 3; }'
assert 0 3      '{ return 17 / 5; }'
assert 0 2      '{ return 17 % 5; }'
assert 0 7      '{ return -3 + 10; }'
assert 0 5      '{ return - -5; }'
assert 0 4      '{ return 2 * -3 + 10; }'
assert 0 1      '{ return 1 < 2; }'
assert 0 0      '{ return 2 < 1; }'
assert 0 1      '{ return 3 >= 3; }'
assert 0 1      '{ return 1 == 1; }'
assert 0 0      '{ return 1 != 1; }'
assert 0 1      '{ return !0; }'
assert 0 0      '{ return !5; }'
assert 0 2      '{ return (1 < 2) + (3 == 3); }'

echo -e "\n${blu}More stmt tests${rst}"
assert 0 3      '{ 1; 2; return 3; }'
assert 0 1      '{ return 1; return 2; }'
assert 0 5      '{ { return 5; } }'
assert 0 0      '{ 1 + 2; }'
assert 0 0      '{ }'

echo -e "\n${blu}Testing AND/OR${rst}"
assert 0 1      '{ return 1 && 1; }'
assert 0 0      '{ return 1 && 0; }'
assert 0 0      '{ return 0 && 1; }'
assert 0 1      '{ return 2 && 3; }'
assert 0 0      '{ return 0 || 0; }'
assert 0 1      '{ return 0 || 5; }'
assert 0 0      '{ return 0 && 1/0; }'
assert 0 1      '{ return 1 || 1/0; }'
assert 0 1      '{ return 1 || 0 && 0; }'
assert 0 1      '{ return (0 || 1) && (1 && 2); }'

echo -e "\n${blu}Testing error cases${rst}"
assert 1 1      '{ return 1 }'
assert 1 1      '{ 1 }'
assert 1 1      '{ return 1;'
assert 1 1      '{ not closed'

if [ $fail -eq 0 ]; then
        echo -e "${blu}All tests passed${rst}"
else
        echo -e "${red}$fail / $((pass+fail)) not passed${rst}"
        exit 1
fi
