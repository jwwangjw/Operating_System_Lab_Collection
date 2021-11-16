#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<iostream>
#include<string>
#include<sstream>
#include<vector>


using namespace std;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;



int  BytsPerSec;
int  SecPerClus;
int  RsvdSecCnt;
int  NumFATs;
int  RootEntCnt;
int  FATSz;	

string str_print;
FILE* fat12;

#pragma pack (1) /*指定按1字节对齐*/

enum COLOR
{
	COLOR_NULL = 0,
	COLOR_RED = 1
};
struct BPB {
	u16  BPB_BytsPerSec;
	u8   BPB_SecPerClus;
	u16  BPB_RsvdSecCnt;
	u8   BPB_NumFATs;
	u16  BPB_RootEntCnt;
	u16  BPB_TotSec16;
	u8   BPB_Media;	
	u16  BPB_FATSz16;
	u16  BPB_SecPerTrk;
	u16  BPB_NumHeads;
	u32  BPB_HiddSec;
	u32  BPB_TotSec32;	
};

struct RootEntry {
	char DIR_Name[11];
	u8   DIR_Attr;	
	char reserved[10];
	u16  DIR_WrtTime;
	u16  DIR_WrtDate;
	u16  DIR_FstClus;
	u32  DIR_FileSize;
};

#pragma pack () 

class Node {
public:
	string name;
	vector<Node *> next;
	string path;
	u32 FileSize;
	bool isfile = false;
	bool isval = true;
	int dir_count = 0;
	int file_count = 0;	
	char content[10000] = "";
};



extern "C"
{
	/**
	 * 输出一个字符串
	 * @param c 颜色
	 * @param s 待输出字符串
	 */
	void colorPrint(COLOR c, const char *s);
}
void fillBPB(FILE* fat12, struct BPB* bpb_ptr) {
	int check;
	check = fseek(fat12, 11, SEEK_SET);
	if (check == -1)
		colorPrint(COLOR_NULL,"fseek in fillBPB failed!\n");

	check = fread(bpb_ptr, 1, 25, fat12);
	if (check != 25)
		colorPrint(COLOR_NULL,"fread in fillBPB failed!\n");
}
/**
 * 读取fat表
 * @return 
 */
int  getFATValue(FILE * fat12, int num) {
	int fatBase = RsvdSecCnt * BytsPerSec;
	int fatPos = fatBase + num * 3 / 2;
	int type = 0;
	if (num % 2 == 0) {
		type = 0;
	}
	else {
		type = 1;
	}
	u16 bytes;
	u16* bytes_ptr = &bytes;
	int check;
	check = fseek(fat12, fatPos, SEEK_SET);
	if (check == -1)
		colorPrint(COLOR_NULL,"fseek in getFATValue failed!");

	check = fread(bytes_ptr, 1, 2, fat12);
	if (check != 2)
		colorPrint(COLOR_NULL,"fread in getFATValue failed!");
	if (type == 0) {
		bytes = bytes << 4;
		return bytes >> 4;
	}
	else {
		return bytes >> 4;
	}
}
/**
 * 获取文件内容（cat）
 * @param startClus:开始扇区
 * @return 内容
 */
void getContent(FILE * fat12, int startClus, Node *son) {
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	int currentClus = startClus;
	int value = 0;
	char *p = son->content;
	if (startClus == 0) {
		return;
	}
	while (value < 0xFF8) {
		value = getFATValue(fat12, currentClus);
		if (value == 0xFF7
			) {
			colorPrint(COLOR_NULL,"坏簇!\n");
			break;
		}
		char* str = (char*)malloc(SecPerClus*BytsPerSec);
		char *content = str;
		int startByte = dataBase + (currentClus - 2)*SecPerClus*BytsPerSec;
		int check;
		check = fseek(fat12, startByte, SEEK_SET);
		if (check == -1)
			colorPrint(COLOR_NULL,"fseek in printChildren failed!");

		check = fread(content, 1, SecPerClus*BytsPerSec, fat12);
		if (check != SecPerClus * BytsPerSec)
			colorPrint(COLOR_NULL,"fread in printChildren failed!");

		int count = SecPerClus * BytsPerSec;
		int loop = 0;
		for (int i = 0; i < count; i++) {
			*p = content[i];
			p++;
		}
		free(str);
		currentClus = value;
	}
}
/**
 * 建立新节点（主要用来添加./..）
 * @return node*
 */
void creatNode(Node *p, Node *father) {
	Node *q = new Node();
	q->name = ".";
	q->isval = false;
	p->next.push_back(q);
	q = new Node();
	q->name = "..";
	q->isval = false;
	p->next.push_back(q);
}
/**
 * 读取子节点
 * @param startclus：开始簇
 * @param father: 父节点
 * @return 是 / 否
 */
void readChildren(FILE * fat12, int startClus, Node *father) {
	//数据区第一个簇
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	int currentClus = startClus;
	int value = 0;
	while (value < 0xFF8) {
		value = getFATValue(fat12, currentClus);
		if (value == 0xFF7) {
			colorPrint(COLOR_NULL,"坏簇!\n");
			break;
		}
		int startByte = dataBase + (currentClus - 2)*SecPerClus*BytsPerSec;
		int check;
		int count = SecPerClus * BytsPerSec;
		int loop = 0;
		while (loop < count) {
			int i;
			RootEntry sonEntry;
			RootEntry *sonEntryP = &sonEntry;
			check = fseek(fat12, startByte + loop, SEEK_SET);
			if (check == -1)
				colorPrint(COLOR_NULL,"fseek failed!\n");

			check = fread(sonEntryP, 1, 32, fat12);
			if (check != 32)
				colorPrint(COLOR_NULL,"fread failed!\n");
			loop += 32;
			if (sonEntryP->DIR_Name[0] == '\0') {
				continue;
			}
			int j;
			int boolean = 0;
			for (j = 0; j < 11; j++) {
				if (!(((sonEntryP->DIR_Name[j] >= 48) && (sonEntryP->DIR_Name[j] <= 57)) ||
					((sonEntryP->DIR_Name[j] >= 65) && (sonEntryP->DIR_Name[j] <= 90)) ||
					((sonEntryP->DIR_Name[j] >= 97) && (sonEntryP->DIR_Name[j] <= 122)) ||
					(sonEntryP->DIR_Name[j] == ' '))) {
					boolean = 1;
					break;
				}
			}
			if (boolean == 1) {
				continue;
			}
			if ((sonEntryP->DIR_Attr & 0x10) == 0) {
				char tempName[12];
				int k;
				int tempLong = -1;
				for (k = 0; k < 11; k++) {
					if (sonEntryP->DIR_Name[k] != ' ') {
						tempLong++;
						tempName[tempLong] = sonEntryP->DIR_Name[k];
					}
					else {
						tempLong++;
						tempName[tempLong] = '.';
						while (sonEntryP->DIR_Name[k] == ' ') k++;
						k--;
					}
				}
				tempLong++;
				tempName[tempLong] = '\0';
				Node *son = new Node();
				father->next.push_back(son);
				son->name = tempName;
				son->FileSize = sonEntryP->DIR_FileSize;
				son->isfile = true;
				son->path = father->path + tempName + "/";
				father->file_count++;
				getContent(fat12, sonEntryP->DIR_FstClus, son);
			}
			else {
				char tempName[12];
				int count = -1;
				for (int k = 0; k < 11; k++) {
					if (sonEntryP->DIR_Name[k] != ' ') {
						count++;
						tempName[count] = sonEntryP->DIR_Name[k];
					}
					else {
						count++;
						tempName[count] = '\0';
					}
				}
				Node *son = new Node();
				father->next.push_back(son);
				son->name = tempName;
				son->path = father->path + tempName + "/";
				father->dir_count++;
				creatNode(son, father);
				readChildren(fat12, sonEntryP->DIR_FstClus, son);
			}
		}
		currentClus = value;
	};
}
/**
 * 读取文件
 * @return 
 */
void ReadFiles(FILE * fat12, struct RootEntry* rootEntry_ptr, Node *father) {
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	int check;
	char realName[12];
	int i;
	for (i = 0; i < RootEntCnt; i++) {
		check = fseek(fat12, base, SEEK_SET);
		if (check == -1)
			colorPrint(COLOR_NULL,"fseek in printFiles failed!\n");
		check = fread(rootEntry_ptr, 1, 32, fat12);
		if (check != 32)
			colorPrint(COLOR_NULL,"fread in printFiles failed!\n");
		base += 32;
		if (rootEntry_ptr->DIR_Name[0] == '\0') continue;
		int j;
		int boolean = 0;
		for (j = 0; j < 11; j++) {
			if (!(((rootEntry_ptr->DIR_Name[j] >= 48) && (rootEntry_ptr->DIR_Name[j] <= 57)) ||
				((rootEntry_ptr->DIR_Name[j] >= 65) && (rootEntry_ptr->DIR_Name[j] <= 90)) ||
				((rootEntry_ptr->DIR_Name[j] >= 97) && (rootEntry_ptr->DIR_Name[j] <= 122)) ||
				(rootEntry_ptr->DIR_Name[j] == ' '))) {
				boolean = 1;
				break;
			}
		}
		if (boolean == 1) continue;

		int k;
		if ((rootEntry_ptr->DIR_Attr & 0x10) == 0) {
			int tempLong = -1;
			for (k = 0; k < 11; k++) {
				if (rootEntry_ptr->DIR_Name[k] != ' ') {
					tempLong++;
					realName[tempLong] = rootEntry_ptr->DIR_Name[k];
				}
				else {
					tempLong++;
					realName[tempLong] = '.';
					while (rootEntry_ptr->DIR_Name[k] == ' ') k++;
					k--;
				}
			}
			tempLong++;
			realName[tempLong] = '\0';
			Node *son = new Node();
			father->next.push_back(son);
			son->name = realName;
			son->FileSize = rootEntry_ptr->DIR_FileSize;
			son->isfile = true;
			son->path = father->path + realName + "/";
			father->file_count++;
			getContent(fat12, rootEntry_ptr->DIR_FstClus, son);
		}
		else {
			int tempLong = -1;
			for (k = 0; k < 11; k++) {
				if (rootEntry_ptr->DIR_Name[k] != ' ') {
					tempLong++;
					realName[tempLong] = rootEntry_ptr->DIR_Name[k];
				}
				else {
					tempLong++;
					realName[tempLong] = '\0';
					break;
				}
			}
			Node *son = new Node();
			father->next.push_back(son);
			son->name = realName;
			son->path = father->path + realName + "/";
			father->dir_count++;
			creatNode(son, father);
			readChildren(fat12, rootEntry_ptr->DIR_FstClus, son);
		}
	}
}
/**
 * ls命令
 * @return 
 */
void ls(Node *r) {
	Node *p = r;
	if (p->isfile == true) {
		return;
	}
	else {
		str_print = p->path + ":\n";
		colorPrint(COLOR_NULL,str_print.c_str());
		str_print.clear();
		Node *q;
		int leng = p->next.size();
		for (int i = 0; i < leng; i++) {
			q = p->next[i];
			if (q->isfile == false) {
				str_print = q->name+" ";
				colorPrint(COLOR_RED,str_print.c_str());
				str_print.clear();
			}
			else {
				str_print = q->name + "  ";
				colorPrint(COLOR_NULL,str_print.c_str());
				str_print.clear();
			}
		}
		str_print = "\n";
		colorPrint(COLOR_NULL,str_print.c_str());
		str_print.clear();
		for (int i = 0; i < leng; i++) {
			if (p->next[i]->isval == true) ls(p->next[i]);
		}
	}
}
/**
 * ll命令
 */
void ll(Node *root) {
	Node *p = root;
	if (p->isfile) {
		return;
	}
	else {
		str_print = p->path + " " + to_string(p->dir_count) + " " + to_string(p->file_count) + ":\n";
		colorPrint(COLOR_NULL,str_print.c_str());
		str_print.clear();
		Node *q;
		int leng = p->next.size();
		for (int i = 0; i < leng; i++) {
			q = p->next[i];
			if (q->isfile == false) {
				if (q->isval) {
					str_print = "  " + to_string(q->dir_count) + " " + to_string(q->file_count) + "\n";
					colorPrint(COLOR_RED,q->name.c_str());
					colorPrint(COLOR_NULL, str_print.c_str());
					str_print.clear();
				}
				else {
					str_print = q->name + "  \n";
					colorPrint(COLOR_RED,str_print.c_str());
					str_print.clear();
				}
			}
			else {
				str_print = q->name + "  " + to_string(q->FileSize) + "\n";
				colorPrint(COLOR_NULL,str_print.c_str());
				str_print.clear();
			}
		}
		colorPrint(COLOR_NULL,"\n");
		for (int i = 0; i < leng; i++) {
			if (p->next[i]->isval == true) ll(p->next[i]);
		}
	}
}
/**
 * ll+路径命令
 * @return 
 */
void ll_path(Node *root, string p, int & e_flag, bool hasL) {
	if (p.compare(root->path) == 0) { 
		if (root->isfile) {
			e_flag = 2;
			return;
		}
		else {
			e_flag = 1;
			if (hasL) {
				ll(root);
			}
			else {
				ls(root);
			}
		}
		return;
	}
	if (p.length() <= root->path.length()) {
		return;
	}
	string temp = p.substr(0, root->path.length());
	if (temp.compare(root->path) == 0) {
		for (Node *q : root->next) {
			ll_path(q, p, e_flag, hasL);
		}
	}
}
/**
 * cat命令查看文件
 * @return 
 */
void cat(Node *root, string p, int & e_flag) {
	if (p.compare(root->path) == 0) {
		if (root->isfile) {
			e_flag = 1;
			if (root->content[0] != 0) {
				colorPrint(COLOR_NULL,root->content);
				colorPrint(COLOR_NULL,"\n");
			}
			return;
		}
		else {
			e_flag = 2;
			return;
		}
	}
	if (p.length() <= root->path.length()) {
		return;
	}
	string temp = p.substr(0, root->path.length());
	if (temp.compare(root->path) == 0) {
		for (Node *q : root->next) {
			cat(q, p, e_flag);
		}
	}
}
/**
 * 处理输入命令
 * @return
 */
void handleCommand(Node* root) {
	bool flag=1;
	while (flag) {
		colorPrint(COLOR_NULL,">");
		string input;
		getline(cin, input);
		vector<string> input_list;
		input_list.clear();
		istringstream iss(input);
		string temp;
		while (getline(iss, temp, ' ')) {
			input_list.push_back(temp);
		}
		for (auto it = input_list.begin(); it != input_list.end();) {//strip
			if (*it == "") {
				it = input_list.erase(it);
			}
			else {
				it++;
			}
		}
		int command_judge;
		if (input_list[0].compare("exit") == 0) command_judge = 0;
		else if (input_list[0].compare("ls") == 0) command_judge = 1;
		else command_judge = 2;
		switch (command_judge) {
		case 0: {
			colorPrint(COLOR_NULL, "Bye!\n");
			fclose(fat12);
			flag = 0;
			break;
		}
		case 1: {
			if (input_list.size() == 1) {
				ls(root);
			}
			else {
				bool hasL = false;
				bool hasPath = false;
				bool error = false;
				string *path = NULL;
				for (int i = 1; i < input_list.size(); i++) {
					string s = input_list[i];
					if (s[0] != '-') {
						if (hasPath) {
							colorPrint(COLOR_NULL, "COMMAND INVALID!\n");
							error = true;
							break;
						}
						else {
							hasPath = true;
							if (input_list[i][0] != '/') {
								input_list[i] = "/" + input_list[i];
							}
							if (input_list[i][input_list[i].length() - 1] != '/') {
								input_list[i] += '/';
							}
							path = &input_list[i];
						}
					}
					else {
						if (s.length() == 1) {
							colorPrint(COLOR_NULL, "COMMAND INVALID!\n");
							error = true;
							break;
						}
						for (int j = 1; j < s.length(); j++) {
							if (s[j] != 'l') {
								error = true;
								colorPrint(COLOR_NULL, "COMMAND INVALID!\n");
								break;
							}
						}
						hasL = true;
					}
				}
				if (error) {
					continue;
				}
				int exist = 0;
				if (hasL && !hasPath) {
					exist = 1;
					ll(root);
				}
				else if (!hasL&&hasPath) {
					ll_path(root, *path, exist, false);
				}
				else if (hasL&&hasPath) {
					ll_path(root, *path, exist, true);
				}
				else {
					ls(root);
					continue;
				}
				if (exist == 0) {
					colorPrint(COLOR_NULL, "PATH INVALID!\n");
					continue;
				}
				else if (exist == 2) {
					colorPrint(COLOR_NULL, "FILE BROKEN(OR NOT EXIST)!\n");
					continue;
				}
			}
			break;
		}
		case 2: {
			if (input_list.size() == 2 && input_list[1][0] != '-') {
				int exist = 0;
				if (input_list[1][0] != '/') {
					input_list[1] = "/" + input_list[1];
				}
				if (input_list[1][input_list[1].length() - 1] != '/') {
					input_list[1] += '/';
				}
				cat(root, input_list[1], exist);
				if (exist == 0) {
					colorPrint(COLOR_NULL, "PATH INVALID!\n");
					continue;
				}
				else if (exist == 2) {
					colorPrint(COLOR_NULL, "FILE BROKEN(OR NOT EXIST)!\n");
					continue;
				}
			}
			else {
				colorPrint(COLOR_NULL, "COMMAND INVALID!\n");
				continue;
			}
			break;
		}
		default: {
			colorPrint(COLOR_NULL, "COMMAND INVALID!\n");
			continue;
		}
		}
	}
}

int main() {
	fat12 = fopen("a3.img", "rb");
	struct BPB bpb;
	struct BPB* bpb_ptr = &bpb;
	Node *root = new Node();
	root->name = "";
	root->path = "/";
	fillBPB(fat12, bpb_ptr);
	BytsPerSec = bpb_ptr->BPB_BytsPerSec;
	SecPerClus = bpb_ptr->BPB_SecPerClus;
	RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
	NumFATs = bpb_ptr->BPB_NumFATs;
	RootEntCnt = bpb_ptr->BPB_RootEntCnt;
	if (bpb_ptr->BPB_FATSz16 != 0) {
		FATSz = bpb_ptr->BPB_FATSz16;
	}
	else {
		FATSz = bpb_ptr->BPB_TotSec32;
	}
	struct RootEntry rootEntry;
	struct RootEntry* rootEntry_ptr = &rootEntry;
	ReadFiles(fat12, rootEntry_ptr, root);
	handleCommand(root);
	return 0;
}