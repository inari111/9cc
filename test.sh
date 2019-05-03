##!/bin/bash
try() {
	expected="$1"
	input="$2"

	./9cc "$input" > tmp.s
	gcc -o tmp tmp.s
	./tmp
	actual="$?"

	if [ "$actual" = "$expected" ]; then
		echo "[OK] $input => $actual"
		echo "\n-------------------------------------------------\n"
	else
		echo "[ERROR] $expected expected, but got $actual"
		echo "\n-------------------------------------------------\n"
	fi
}

try 0 0
try 42 42

# 足し算、引き算対応
try 21 '5+20-4'

# 空白対応
try 41 ' 12 + 34 - 5 '

# 加減乗算と優先順位のカッコからなる式に対応
try 47 '5+6*7'
try 15 '5*(9-6)'
try 4 '(3+5)/2'

# 単項プラスと単項マイナス
try 5 '-10+15'
try 15 '14+-(-7)-(+6)'

echo OK
