#!/usr/bin/env bash

red="\033[31m"
grn="\033[32m"
yel="\033[33m"
blu="\033[34m"
rst="\033[0m"

pass=0
fail=0

max_width=$(( ${COLUMNS:-78} - 8 ))

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

echo -e "${blu}Testing codegen${rst}"
assert 1        '{ return 1; }'
assert 42       '{ return 42; }'
assert 7        '{ return 1 + 2 * 3; }'
assert 5        '{ return 10 - 2 - 3; }'
assert 9        '{ return (1 + 2) * 3; }'
assert 3        '{ return 17 / 5; }'
assert 2        '{ return 17 % 5; }'
assert 7        '{ return -3 + 10; }'
assert 5        '{ return - -5; }'
assert 4        '{ return 2 * -3 + 10; }'
assert 1        '{ return 1 < 2; }'
assert 0        '{ return 2 < 1; }'
assert 1        '{ return 3 >= 3; }'
assert 1        '{ return 1 == 1; }'
assert 0        '{ return 1 != 1; }'
assert 1        '{ return !0; }'
assert 0        '{ return !5; }'
assert 2        '{ return (1 < 2) + (3 == 3); }'
assert 1        '{ return (1); }'
assert 255      '{ return -1; }'

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

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ return 1 }'
assert_error '{ 1 }'
assert_error '{ return 1;'
assert_error '{ { return 1; }'

echo -e "\n${blu}Testing variables${rst}"
assert 5  '{ int count; count = 5; return count; }'
assert 12 '{ int x; int y; x = 3; y = 4; return x * y; }'
assert 14 '{ int foo; int bar; foo = bar = 7; return foo + bar; }'
assert 2  '{ int a; { a = 2; } return a; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ a = 1; }'
assert_error '{ int 5; }'
assert_error '{ int a; int a; }'
assert_error '{ int a; a = 3; b = a; }'

echo -e "\n${blu}Testing if/else${rst}"
assert 2  '{ if (1) return 2; return 3; }'
assert 3  '{ if (0) return 2; return 3; }'
assert 4  '{ if (0) return 2; else return 4; }'
assert 10 '{ int a; a = 5; if (a > 3) a = a * 2; return a; }'
assert 11 '{ int a; a = 1; if (a == 0) { return 1; } else { a = a + 10; } return a; }'
assert 2  '{ int a; a = 1; if (a == 1) { a= a + 1; } else { a = a + 10; } return a; }'
assert 2  '{ if (1) if (0) return 1; else return 2; return 3; }'
assert 5  '{ if (0 && 1/0) return 1; return 5; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ if 1 return 2; }'

echo -e "\n${blu}Testing while${rst}"
assert 10 '{ int i; i = 0; while (i < 10) i = i + 1; return i; }'
assert 55 '{ int i; int s; i = 1; s = 0; while (i <= 10) { s = s + i; i = i + 1; } return s; }'
assert 2  '{ while (0) return 1; return 2; }'
assert 89 '{ int a; int b; int t; int n; a = 0; b = 1; n = 11; while (n > 0) { t = a + b; a = b; b = t; n = n - 1; } return a; }'

echo -e "\n${blu}Testing for${rst}"
assert 55 '{ int i; int s; s = 0; for (i = 1; i <= 10; i = i + 1) s = s + i; return s; }'
assert 5  '{ int i; for (i = 0; i < 5; i = i + 1) { } return i; }'
assert 7  '{ int i; i = 3; for (; i < 7;) i = i + 1; return i; }'
assert 3  '{ for (;;) return 3; return 5; }'
assert 12 '{ int i; int j; int c; c = 0; for (i = 0; i < 3; i = i + 1) for (j = 0; j < 4; j = j + 1) c = c + 1; return c; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ for (;) return 1; }'

echo -e "\n${blu}Testing break${rst}"
assert 5  '{ int i; i = 0; while (1) { if (i == 5) break; i = i + 1; } return i; }'
assert 7  '{ int i; for (i = 0; ; i = i + 1) if (i == 7) break; return i; }'
assert 36 '{ int i; int j; int c; c = 0; for (i = 0; i < 3; i = i + 1) { for (j = 0; ; j = j + 1) { if (j == 2) break; c = c + 1; } c = c + 10; } return c; }'
assert 4  '{ int i; int j; for (i = 0; i < 10; i = i + 1) { for (j = 0; j < 3; j = j + 1) { } if (i == 4) break; } return i; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ break; }'

echo ""
rm tmp*

if [ $fail -eq 0 ]; then
        echo -e "${blu}All tests passed${rst}"
else
        echo -e "${red}$fail/$((pass+fail)) not passed${rst}"
        exit 1
fi
