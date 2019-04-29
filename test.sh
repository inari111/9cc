##!/bin/bash
try() {
	expected="$1"
	input="$2"

	./9cc "$input" > tmp.s
	gcc -o tmp tmp.s
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "$input => $actual"
	else
		echo "$expected expected, but got $actual"
	fi
}

try 0 0
try 42 42

# 足し算、引き算対応
try 21 '5+20-4'

# 空白対応
try 41 " 12 + 34 - 5 "

echo OK
