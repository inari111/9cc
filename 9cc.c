#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// トークンの型を表す値
enum {
	TK_NUM = 256, // 整数トークン
	TK_EOF,       // 入力の終わりを表すトークン
};

// トークンの型
typedef struct {
	int ty;      // トークンの型
	int val;     // tyがTK_NUMの場合、その数値
	char *input; // トークン文字列(エラーメッセージ用)
} Token;

// トークナイズした結果のトークン列はこの配列に保存する
Token tokens[100];

enum {
	ND_NUM = 256, // 整数のノードの型
};

typedef struct Node {
	int ty;           // 演算子かND_NUM
	struct Node *lhs; // 左辺
	struct Node *rhs; // 右辺
	int val;          // tyがND_NUMの場合のみ使う
} Node;

static int pos;

// 次のトークンが期待した型かどうかをチェックして、
// 期待した型場合だけ入力を1トークン進める
// posはトークンのインデックスで、初期値0
int consume(int ty) {
	if (tokens[pos].ty != ty)
		return 0;
	pos++;
	return 1;
}

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

static Node *add();
static Node *mul();
static Node *term();
static Node *unary();

// 新しいノードを作成する
// 左辺と右辺を受け取る2項演算子
Node *new_node(int ty, Node *lhs, Node *rhs) {
	Node *node = malloc(sizeof(Node));
	node->ty = ty;
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

// 新しいノードを作成する
// 数値を受け取る
Node *new_node_num(int val) {
	Node *node = malloc(sizeof(Node));
	node->ty = ND_NUM;
	node->val = val;
	return node;
}

// 左結合の演算子をパーズする
Node *add() {
	Node *node = mul();

	for (;;) {
		if (consume('+'))
			node = new_node('+', node, mul());
		else if (consume('-'))
			node = new_node('-', node, mul());
		else
			return node;
	}
}

// 左結合の演算子をパーズする
Node *mul() {
	Node *node = unary();

	for (;;) {
		if (consume('*'))
			node = new_node('*', node, term());
		else if (consume('/'))
			node = new_node('/', node, term());
		else
			return node;
	}
}

// 左結合演算子ではない
Node *term() {
	if (consume('(')) {
		Node *node = add();
		if (!consume(')'))
			error("開きカッコに対応する閉じカッコがありません: %s", tokens[pos].input);
		return node;
	}

	// そうでなければ数値のはず
	if (tokens[pos].ty == TK_NUM)
		return new_node_num(tokens[pos++].val);

	error("数値でも開きカッコでもないトークンです: %s", tokens[pos].input);
}

Node *unary() {
	if (consume('+'))
		return term();
	// -xを0-xに置き換える
	if (consume('-'))
		return new_node('-', new_node_num(0), term());
	return term();
}

void gen(Node *node) {
	if (node->ty == ND_NUM) {
		printf("  push %d\n", node->val);
		return;
	}

	gen(node->lhs);
	gen(node->rhs);

	printf("  pop rdi\n");
	printf("  pop rax\n");

	switch (node->ty) {
	case '+':
		printf("  add rax, rdi\n");
		break;
	case '-':
		printf("  sub rax, rdi\n");
		break;
	case '*':
		printf("  mul rdi\n");
		break;
	case '/':
		printf("  mov rdx, 0\n");
		printf("  div rdi\n");
	}

	printf("  push rax\n");
}


// pが指している文字列をトークンに分割してtokensに保存する
void tokenize(char *p) {
	int i = 0;
	while (*p) {
		// 空白文字をスキップ
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '(' || *p == ')') {
			tokens[i].ty = *p;
			tokens[i].input = p;
			i++;
			p++;
			continue;
		}

		if (isdigit(*p)) {
			tokens[i].ty = TK_NUM;
			tokens[i].input = p;
			tokens[i].val = strtol(p, &p, 10);
			i++;
			continue;
		}

		error("トークナイズできません: %s", p);
		exit(1);
	}

	tokens[i].ty = TK_EOF;
	tokens[i].input = p;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "引数の個数が正しくありません\n");
		return 1;
	}

	// トークナイズしてパースする
	tokenize(argv[1]);
	Node *node = add();

	// アセンブリの前半部分を出力
	printf(".intel_syntax noprefix\n");
	printf(".global main\n");
	printf("main:\n");

	// 抽象構文木を下りながらコード生成
	gen(node);

	// スタックトップに式全体の値が残っているはずなので
	// それをRAXにロードして関数からの返り値とする
	printf("  pop rax\n");
	printf("  ret\n");
	return 0;
}
