#include <time.h>
#include <algorithm>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <iostream>
#include "Judge.h"

using namespace std;

const int USER = 1;
const int MACHINE = 2;
const int MACHINE_WIN = 1;
const int USER_WIN = -1;
const int TIE = 0;
const int UNTERMINAL = -2;
const double c = 0.8;


class Node {
private:
	int row, col;		// ���̶�Ӧ������
	int lastX, lastY;	// ��״̬��һ������
	int** board;		// ��״̬����
	int* top;			// ��״̬�����ӵ�
	int noX, noY;		// ��״̬�������ӵ�
	int winTimes, totTimes;	// ��һ��ִ�ӷ��Ļ�ʤ��������ģ�����
	int chessman;		// ��״̬��һ��������˭�ߵģ�����һ��ִ�ӷ�(������ߵı��)
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
	// �ڹ����ʱ��������ģ������board��top���ǲ�ȥ���������������⣬��Ϊ������������ڵ��Լ��ġ�
	// �����ڹ�����ǰ�����ֻ�������޸ģ���ֱ��ʹ��ԭ����ָ�뼴��
	// �����Ҫ�޸ģ���������ǣһ����ȫ��Ļ����������ȸ���һ�ݣ��޸ģ����ݣ�������
	Node(int _row, int _col, int _x, int _y, int** _board, int* _top, int _noX, int _noY,
		int _winTimes, int _totTimes, int _chessman, Node* _parent) :row(_row), col(_col), lastX(_x), lastY(_y),
		noX(_noX), noY(_noY), winTimes(_winTimes), totTimes(_totTimes), chessman(_chessman), parent(_parent) {
		top = copyTop(_top);
		board = copyBoard(_board);
		children = new Node * [col];		// �����ӽڵ����Ϣ��û��ȷ�������ȵ����ɺ��ӽڵ��ʱ���ٵ���
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
		// ֮ǰ�ߵ���Ӯ��
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
	Node* expand() {	// expand�����Ӻ��ӽڵ㣬���ٿ���չ�ڵ�
		// ���ȸ��ƣ�ΪʲôҪ����һ���أ���Ϊ���top��board��Ҫ���޸�,children��top��board�����ڴ˻����ϵ�
		int* newTop = copyTop(top);
		int** newBoard = copyBoard(board);
		int id = rand() % expandableNum;	// ���ѡ��һ�������ӵ�
		int y = expandCol[id], x = --newTop[y];		// �����ӵ�λ��
		newBoard[x][y] = 3 - chessman;		// �������ӵ�λ�÷��ü���������
		if (x - 1 == noX && y == noY) {		// ���������˲������ӵ�
			--newTop[y];
		}
		children[y] = new Node(row, col, x, y, newBoard, newTop, noX, noY, 0, 0, 3 - chessman, this);	// 3-chessman���൱����Ȩ�任��
		swap(expandCol[id], expandCol[--expandableNum]);
		if (newTop) {
			delete[] newTop;
			newTop = nullptr;
		}
		if (newBoard) {
			for (int i = 0; i < row; i++) {
				delete[] newBoard[i];
				newBoard[i] = nullptr;
			}
			delete[] newBoard;
			newBoard = nullptr;
		}
		return children[y];
	}
	Node* bestChild(double c) {			// ������ѡ��bestChild��������
		Node* bestC = nullptr;
		double maxval = -INT_MAX;
		for (int i = 0; i < col; i++) {
			if (children[i]) {		// �����Ϊ��
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
		// delete board, top, parent, children, expandCol
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
		//		fprintf(stderr, "delete expanCol!");
		if (children) {
			for (int i = 0; i < col; i++) {
				// children[i]->clear();
				delete children[i];
				children[i] = nullptr;
			}
			delete[] children;
			children = nullptr;
		}
		//		fprintf(stderr, "deleteAnode in a function!");
	}
};

// ÿһ����Ҫ���½�һ��UCT��
class UCT {
	Node* root;		// UCT���ĸ��ڵ� 
	int row, col;
	int noX, noY;
	int startTime;		// ��ʼ��ʱʱ�� 
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
	// ���ػ�ʤ���
	int getProfit(int chessman, int x, int y, int** board, int* top) {
		if (chessman == USER && userWin(x, y, row, col, board))
			return USER_WIN;
		if (chessman == MACHINE && machineWin(x, y, row, col, board))
			return MACHINE_WIN;
		if (isTie(col, top))
			return TIE;
		return UNTERMINAL;
	}
	// ����˭Ӯ��
	int defaultPolicy(Node* v) {		// ����v��ǰ״̬�����ж�
		// �Ȱ�v��״̬ȡ���������ǽڵ�v��top��board��Ϊ�����޸ģ����Եÿ���һ��
		int tmp_chessman = v->getChessman();

		int* tmp_top = new int[col];
		int** tmp_board = new int* [row];

		int* v_top = v->getTop();
		int** v_board = v->getBoard();
		// ��v��top��board״̬����һ��
		for (int i = 0; i < col; i++) {
			tmp_top[i] = v_top[i];
		}
		for (int i = 0; i < row; i++) {
			tmp_board[i] = new int[col];
			for (int j = 0; j < col; j++) {
				tmp_board[i][j] = v_board[i][j];
			}
		}
		// �����ڵ�ǰ��һ�����ߵ�
		int x = v->getX(), y = v->getY();

		double p = getProfit(tmp_chessman, x, y, tmp_board, tmp_top);	// ��ʵ�ǿ���ǰ״̬�Ƿ�����ֹ״̬
		// ����δ��������ִ��ѭ��
		while (p == UNTERMINAL) {
			// ���һ��
			y = rand() % col;
			while (tmp_top[y] == 0) {
				y = rand() % col;
			}
			// �õ����ӵ�x,y
			x = --tmp_top[y];
			// ���ӣ�����board��top����
			tmp_board[x][y] = 3 - tmp_chessman;
			if (x - 1 == noX && y == noY) {
				--tmp_top[y];
			}
			tmp_chessman = 3 - tmp_chessman;
			p = getProfit(tmp_chessman, x, y, tmp_board, tmp_top);
		}
		if (tmp_top) {
			delete[] tmp_top;
			tmp_top = nullptr;
		}
		if (tmp_board) {
			for (int i = 0; i < row; i++) {
				delete[] tmp_board[i];
				tmp_board[i] = nullptr;
			}
			delete[] tmp_board;
			tmp_board = nullptr;
		}
		v_board = nullptr;
		v_top = nullptr;
		if (p == MACHINE_WIN) {
			return MACHINE;
		}
		else if (p == USER_WIN) {
			return USER;
		}
		// ƽ��
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
		root = new Node(_row, _col, lastX, lastY, board, top, _noX, _noY, 0, 0, USER, nullptr);		// ���캯����new������������delete
		row = _row;
		col = _col;
		noX = _noX;
		noY = _noY;
	}
	Node* uctSearch() {
		srand((unsigned)time(NULL));
		Node* v = nullptr;
		int who_win = MACHINE;
		while (1) {
			v = treePolicy(root);		// v���ص���new������һ�����ӣ�Ҳ����˵�������������ͻ������ڴ�ɾ�����ټ��ϳ����˳����ֲ����������ͻ�����
			who_win = defaultPolicy(v);
			backUp(v, who_win);
			if (clock() - startTime >= 2.3 * CLOCKS_PER_SEC)
				break;
		}
		// cout << "Elapsed time: " << clock() - startTime << endl;
		return root->bestChild(0);
	}
	~UCT() {
		// root->clear();
		delete root;
		// fprintf(stderr, "deleteAUCT in a function!");
		root = nullptr;
	}
};
