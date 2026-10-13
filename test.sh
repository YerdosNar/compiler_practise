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
        timeout 3 ./tmp
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
assert 1        'int main() { return 1; }'
assert 42       'int main() { return 42; }'
assert 7        'int main() { return 1 + 2 * 3; }'
assert 5        'int main() { return 10 - 2 - 3; }'
assert 9        'int main() { return (1 + 2) * 3; }'
assert 3        'int main() { return 17 / 5; }'
assert 2        'int main() { return 17 % 5; }'
assert 7        'int main() { return -3 + 10; }'
assert 5        'int main() { return - -5; }'
assert 4        'int main() { return 2 * -3 + 10; }'
assert 1        'int main() { return 1 < 2; }'
assert 0        'int main() { return 2 < 1; }'
assert 1        'int main() { return 3 >= 3; }'
assert 1        'int main() { return 1 == 1; }'
assert 0        'int main() { return 1 != 1; }'
assert 1        'int main() { return !0; }'
assert 0        'int main() { return !5; }'
assert 2        'int main() { return (1 < 2) + (3 == 3); }'
assert 1        'int main() { return (1); }'
assert 255      'int main() { return -1; }'

echo -e "\n${blu}More stmt tests${rst}"
assert 3      'int main() { 1; 2; return 3; }'
assert 1      'int main() { return 1; return 2; }'
assert 5      'int main() { { return 5; } }'
assert 0      'int main() { 1 + 2; }'
assert 0      'int main() { }'

echo -e "\n${blu}Testing AND/OR${rst}"
assert 1      'int main() { return 1 && 1; }'
assert 0      'int main() { return 1 && 0; }'
assert 0      'int main() { return 0 && 1; }'
assert 1      'int main() { return 2 && 3; }'
assert 0      'int main() { return 0 || 0; }'
assert 1      'int main() { return 0 || 5; }'
assert 0      'int main() { return 0 && 1/0; }'
assert 1      'int main() { return 1 || 1/0; }'
assert 1      'int main() { return 1 || 0 && 0; }'
assert 1      'int main() { return (0 || 1) && (1 && 2); }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { return 1 }'
assert_error 'int main() { 1 }'
assert_error 'int main() { return 1;'
assert_error 'int main() { { return 1; }'

echo -e "\n${blu}Testing variables${rst}"
assert 5  'int main() { int count; count = 5; return count; }'
assert 12 'int main() { int x; int y; x = 3; y = 4; return x * y; }'
assert 14 'int main() { int foo; int bar; foo = bar = 7; return foo + bar; }'
assert 2  'int main() { int a; { a = 2; } return a; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { a = 1; }'
assert_error 'int main() { int 5; }'
assert_error 'int main() { int a; int a; }'
assert_error 'int main() { int a; a = 3; b = a; }'

echo -e "\n${blu}Testing if/else${rst}"
assert 2  'int main() { if (1) return 2; return 3; }'
assert 3  'int main() { if (0) return 2; return 3; }'
assert 4  'int main() { if (0) return 2; else return 4; }'
assert 10 'int main() { int a; a = 5; if (a > 3) a = a * 2; return a; }'
assert 11 'int main() { int a; a = 1; if (a == 0) { return 1; } else { a = a + 10; } return a; }'
assert 2  'int main() { int a; a = 1; if (a == 1) { a= a + 1; } else { a = a + 10; } return a; }'
assert 2  'int main() { if (1) if (0) return 1; else return 2; return 3; }'
assert 5  'int main() { if (0 && 1/0) return 1; return 5; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { if 1 return 2; }'

echo -e "\n${blu}Testing while${rst}"
assert 10 'int main() { int i; i = 0; while (i < 10) i = i + 1; return i; }'
assert 55 'int main() { int i; int s; i = 1; s = 0; while (i <= 10) { s = s + i; i = i + 1; } return s; }'
assert 2  'int main() { while (0) return 1; return 2; }'
assert 89 'int main() { int a; int b; int t; int n; a = 0; b = 1; n = 11; while (n > 0) { t = a + b; a = b; b = t; n = n - 1; } return a; }'

echo -e "\n${blu}Testing for${rst}"
assert 55 'int main() { int i; int s; s = 0; for (i = 1; i <= 10; i = i + 1) s = s + i; return s; }'
assert 5  'int main() { int i; for (i = 0; i < 5; i = i + 1) { } return i; }'
assert 7  'int main() { int i; i = 3; for (; i < 7;) i = i + 1; return i; }'
assert 3  'int main() { for (;;) return 3; return 5; }'
assert 12 'int main() { int i; int j; int c; c = 0; for (i = 0; i < 3; i = i + 1) for (j = 0; j < 4; j = j + 1) c = c + 1; return c; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { for (;) return 1; }'

echo -e "\n${blu}Testing break${rst}"
assert 5  'int main() { int i; i = 0; while (1) { if (i == 5) break; i = i + 1; } return i; }'
assert 7  'int main() { int i; for (i = 0; ; i = i + 1) if (i == 7) break; return i; }'
assert 36 'int main() { int i; int j; int c; c = 0; for (i = 0; i < 3; i = i + 1) { for (j = 0; ; j = j + 1) { if (j == 2) break; c = c + 1; } c = c + 10; } return c; }'
assert 4  'int main() { int i; int j; for (i = 0; i < 10; i = i + 1) { for (j = 0; j < 3; j = j + 1) { } if (i == 4) break; } return i; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { break; }'

echo -e "\n${blu}Testing comments${rst}"
assert 7 'int main() { return 7; /* hi */ }'
assert 8 'int main() { /* a */ /* b */ return /* mid */ 8; }'
assert 3 'int main() { return 6 / 2; }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error 'int main() { return 1; /* never closed'

echo -e "\n${blu}Testing functions${rst}"
assert 5 'int foo() { return 3; } int main() { return 5; }'
assert 5 'int main() { return 5; } int foo() { return 3; }'
assert 2 'int foo() { int a; a = 1; return a; } int main() { int a; a = 2; return a; }'
assert 3  'int foo() { return 3; } int main() { return foo(); }'
assert 8  'int main() { return foo() + 5; } int foo() { return 3; }'
assert 20 'int five() { return 5; } int main() { return five() * five() - five(); }'
assert 2  'int main() { int a; a = 2; b(); return a; } int b() { int a; a = 7; return a; }'
assert 3  'int one() { return 1; } int two() { return one() + one(); } int main() { return two() + one(); }'

echo -e "\n${yel}Testing error cases${rst}"
assert_error '{ return 1; }'

echo ""
rm tmp

if [ $fail -eq 0 ]; then
        echo -e "${blu}All tests passed${rst}"
else
        echo -e "${red}$fail/$((pass+fail)) not passed${rst}"
        exit 1
fi
