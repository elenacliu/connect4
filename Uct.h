#include <time.h>
#include <algorithm>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <iostream>
#include <vector>
#include <utility>
#include "Judge.h"

using namespace std;

const int USER = 1;
const int MACHINE = 2;
const int MACHINE_WIN = 1;
const int USER_WIN = -1;
const int TIE = 0;
const int UNTERMINAL = -2;
const double c = 0.8;


int* tmp_top;
int** tmp_board;

class Node {
private:
	int row, col;		// 棋盘对应的行列
	int lastX, lastY;	// 本状态上一步走子
	int** board;		// 本状态棋盘
	int* top;			// 本状态可落子点
	int noX, noY;		// 本状态不可落子点
	int winTimes, totTimes;	// 上一步执子方的获胜次数，总模拟次数
	int chessman;		// 本状态上一步走子是谁走的，即上一步执子方(或者入边的标号)
	Node* parent;
	Node** children;
	int expandableNum;
	int* expandCol;
	friend class UCT;

	int* copyTop(int* _top) {
		int* newTop = new int[col];
		for (int i = 0; i < col; i++) {
			newTop[i] = _top[i];
		}
		return newTop;
	}

	int** copyBoard(int** _board) {
		int** newBoard = new int* [row];
		for (int i = 0; i < row; i++) {
			newBoard[i] = new int[col];
			for (int j = 0; j < col; j++) {
				newBoard[i][j] = _board[i][j];
			}
		}
		return newBoard;
	}

public:
	// 在构造的时候都是深拷贝的，传入的board和top我们不去考虑它的销毁问题，因为根本不是这个节点自己的。
	// 所以在构造以前，如果只读并不修改，则直接使用原来的指针即可
	// 如果需要修改，但是又怕牵一发动全身的话，则依旧先复制一份，修改，传递，再销毁
	Node(int _row, int _col, int _x, int _y, int** _board, int* _top, int _noX, int _noY,
		int _winTimes, int _totTimes, int _chessman, Node* _parent) :row(_row), col(_col), lastX(_x), lastY(_y),
		noX(_noX), noY(_noY), winTimes(_winTimes), totTimes(_totTimes), chessman(_chessman), parent(_parent) {
		top = copyTop(_top);
		board = copyBoard(_board);
		children = new Node * [col];		// 但孩子节点的信息还没有确定啊！等到生成孩子节点的时候再调整
		expandableNum = 0;
		expandCol = new int[col];
		for (int i = 0; i < col; i++) {
			if (top[i] != 0) {
				expandCol[expandableNum++] = i;
			}
			children[i] = nullptr;
		}
	}

	int getChessman() { return chessman; }
	int getX() { return lastX; }
	int getY() { return lastY; }
	int* getTop() { return top; }
	int** getBoard() { return board; }

	bool isTerminal() {
		if (lastX == -1 && lastY == -1) {
			return false;
		}
		// 如果对方有两个制胜位，可以认为此时就是终止节点（？）不行！
		// 在扩展的时候不能这样认为！
		// 之前走的子赢了
		if ((chessman == USER && userWin(lastX, lastY, row, col, board)) || (chessman == MACHINE && machineWin(lastX, lastY, row, col, board)) ||
			isTie(col, top)) {
			return true;
		}
		return false;
	}
	bool isExpandable() {
		if (isTerminal())
			return false;
		if (expandableNum > 0) {
			return true;
		}
		else {
			return false;
		}
	}
	Node* expand() {	// expand是增加孩子节点，减少可扩展节点
		// 首先复制，为什么要复制一份呢？因为你的top和board需要被修改,children的top和board正是在此基础上的
		/*int* newTop = copyTop(top);
		int** newBoard = copyBoard(board);*/
		int* newTop = top;
		int** newBoard = board;
		int id = rand() % expandableNum;	// 随机选择一个可落子点
		int y = expandCol[id], orig = newTop[y];
		int x = --newTop[y];		// 新棋子的位置
		newBoard[x][y] = 3 - chessman;		// 在新棋子的位置放置己方的棋子
		if (x - 1 == noX && y == noY) {		// 假如遇到了不可落子点
			--newTop[y];
		}
		children[y] = new Node(row, col, x, y, newBoard, newTop, noX, noY, 0, 0, 3 - chessman, this);	// 3-chessman就相当于棋权变换了
		swap(expandCol[id], expandCol[--expandableNum]);
		// 恢复
		newBoard[x][y] = 0;
		newTop[y] = orig;
		newTop = nullptr;
		newBoard = nullptr;
		/*if (newTop) {
			delete[] newTop;
			newTop = nullptr;
		}
		if (newBoard) {
			for (int i = 0; i < row; i++) {
				delete[] newBoard[i];
			}
			delete[] newBoard;
			newBoard = nullptr;
		}*/
		return children[y];
	}
	Node* bestChild(double c) {			// 从中挑选出bestChild，并返回
		Node* bestC = nullptr;
		double maxval = -INT_MAX;
		for (int i = 0; i < col; i++) {
			if (children[i]) {		// 如果不为空
				double val = (children[i]->winTimes * 1.0 / children[i]->totTimes)
					+ c * sqrt(2 * log(totTimes) / children[i]->totTimes);
				if ((maxval < val) || (maxval == val && rand() % 2 == 0)) {
					maxval = val;
					bestC = children[i];
				}
			}
		}
		return bestC;
	}
	void backUp(int who_win) {
		Node* v = this;
		while (v != nullptr) {
			++v->totTimes;
			if (v->getChessman() == who_win) {	// if who_win == 0, means nothing added.
				++v->winTimes;
			}
			v = v->parent;
		}
	}
	~Node() {
		// delete board, top, children, expandCol
		if (top) {
			delete[] top;
			top = nullptr;
		}
		//		fprintf(stderr, "delete top!");
		if (board) {
			for (int i = 0; i < row; i++) {
				delete[] board[i];
				board[i] = nullptr;
			}
			delete[] board;
			board = nullptr;
		}
		//		fprintf(stderr, "delete board!");
		//		if (parent)
		//			delete parent;
		//		fprintf(stderr, "parent!");
		if (expandCol) {
			delete[] expandCol;
			expandCol = nullptr;
		}
		// fprintf(stderr, "delete expanCol!");
		if (children) {
			for (int i = 0; i < col; i++) {
				// children[i]->clear();
				delete children[i];
				children[i] = nullptr;
			}
			delete[] children;
			children = nullptr;
		}
		parent = nullptr;
		// fprintf(stderr, "deleteAnode in a function!");
	}
};

// 每一步都要重新建一次UCT树
class UCT {
	Node* root;		// UCT树的根节点 
	int row, col;
	int noX, noY;
	int startTime;		// 开始计时时间 
	Node* treePolicy(Node* v) {
		while (v->isTerminal() == false) {
			if (v->isExpandable()) {
				return v->expand();
			}
			else {
				v = v->bestChild(c);
			}
		}
		return v;
	}
	// 返回获胜与否
	int getProfit(int chessman, int x, int y, int** board, int* top) {
		// 常规
		if (chessman == USER && userWin(x, y, row, col, board))
			return USER_WIN;
		if (chessman == MACHINE && machineWin(x, y, row, col, board))
			return MACHINE_WIN;
		if (isTie(col, top))
			return TIE;
		// 若常规意义上讲没有获胜
		// 对于本步来讲，chessman是对方
		int me = 3 - chessman, you = chessman;
		int mycount = 0;
		// 若我有至少一个制胜位，我就赢了，因为这一步是我走
		for (int i = 0; i < col; i++) {
			if (top[i] != 0) {
				board[top[i] - 1][i] = me;	// place a chess
				bool flag = (me == USER) ? userWin(top[i] - 1, i, row, col, board) : machineWin(top[i] - 1, i, row, col, board);
				if (flag) {
					++mycount;
					board[top[i] - 1][i] = 0;
					break;
				}
				board[top[i] - 1][i] = 0;	// 回位
			}
		}
		if (mycount > 0)
			return me == USER ? USER_WIN: MACHINE_WIN;
		// 若对方有两个制胜位
		int chessman_count = 0;
		for (int i = 0; i < col; i++) {
			if (top[i] != 0) {
				board[top[i] - 1][i] = you;	// place a chess
				bool flag = (you == USER) ? userWin(top[i] - 1, i, row, col, board) : machineWin(top[i] - 1, i, row, col, board);
				if (flag)
					++chessman_count;
				board[top[i] - 1][i] = 0;	// 回位
			}
		}
		if (chessman_count >= 2) {
			return you == USER ? USER_WIN : MACHINE_WIN;
		}
		return UNTERMINAL;
	}
	// 返回谁赢了
	int defaultPolicy(Node* v) {		// 根据v当前状态进行判定
		// 先把v的状态取出来，但是节点v的top和board因为不能修改，所以得拷贝一份
		int tmp_chessman = v->getChessman();
		int* v_top = v->getTop();
		int** v_board = v->getBoard();
		// 将v的top和board状态复制一份
		for (int i = 0; i < col; i++) {
			tmp_top[i] = v_top[i];
		}
		for (int i = 0; i < row; i++) {
			for (int j = 0; j < col; j++) {
				tmp_board[i][j] = v_board[i][j];
			}
		}
		// 到本节点前面一轮人走的
		int x = v->getX(), y = v->getY();

		double p = getProfit(tmp_chessman, x, y, tmp_board, tmp_top);	// 其实是看当前状态是否是终止状态
		// 若尚未结束，就执行循环
		while (p == UNTERMINAL) {
			// p是unterminal就说明了特殊情况只可能是对方有一个制胜位
			// 但是现在先不更改
			// 随机一列
			y = rand() % col;
			while (tmp_top[y] == 0) {
				y = rand() % col;
			}
			// 得到落子点x,y
			x = --tmp_top[y];
			// 落子，更新board和top数组
			tmp_board[x][y] = 3 - tmp_chessman;
			if (x - 1 == noX && y == noY) {
				--tmp_top[y];
			}
			tmp_chessman = 3 - tmp_chessman;
			p = getProfit(tmp_chessman, x, y, tmp_board, tmp_top);
		}
		if (p == MACHINE_WIN) {
			return MACHINE;
		}
		else if (p == USER_WIN) {
			return USER;
		}
		// 平局
		else
			return 0;
	}
	void backUp(Node* v, int who_win) {
		v->backUp(who_win);
	}
public:
	UCT(int _row, int _col, int lastX, int lastY, int _noX, int _noY, int** board, int* top) {
		startTime = clock();
		// cout << "startTime:" << startTime << endl;
		root = new Node(_row, _col, lastX, lastY, board, top, _noX, _noY, 0, 0, USER, nullptr);
		row = _row;
		col = _col;
		noX = _noX;
		noY = _noY;
	}
	Node* uctSearch() {
		srand((unsigned)time(NULL));
		tmp_top = new int[col];
		tmp_board = new int* [row];
		for (int i = 0; i < row; i++) {
			tmp_board[i] = new int[col];
		}
		while (clock() - startTime <= 2.3 * CLOCKS_PER_SEC) {
			Node* v = treePolicy(root);
			int who_win = defaultPolicy(v);
			backUp(v, who_win);
		}
		// cout << "Elapsed time: " << clock() - startTime << endl;
		delete[] tmp_top;
		for (int i = 0; i < row; i++)
			delete[] tmp_board[i];
		delete[] tmp_board;
		return root->bestChild(0);
	}
	~UCT() {
		// root->clear();
		delete root;
		// fprintf(stderr, "deleteAUCT in a function!");
		root = nullptr;
	}
};
