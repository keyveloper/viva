#include <curses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define CTRL(x) ((x) & 0x1f)

typedef struct {
    char* fileName;
    bool isModified; // save, quit을 관리 
    bool realQuit;
} FileMetaData;

FileMetaData fileMetaData = {NULL, false, false};

typedef struct {
    char task[10];
    char shortCut[10];
} Command;

typedef struct {
    Command commands[10];
    int count;
} MessageBar;

// Node 구조체 정의
typedef struct Node {
    char data;
    struct Node* prev;
    struct Node* next;
} Node;


typedef struct LineNode {
    Node* first;    // for up down key  last에서 거꾸로 조회
    struct LineNode* up;  // line관리 용
    struct LineNode* down;
    int lastColIdx;
    int currentRow;
} LineNode;

typedef struct {
    LineNode* head;
    int size;      
} LineInfo;
LineInfo lineInfo = {NULL, 0};

// 커서 정보를 관리하는 구조체
typedef struct {
    Node* current;
    LineNode* currentLine;
    int row;
    int col;
} Cursor;

typedef struct {
    Node** matches;         // 매칭된 노드들의 배열
    int matchCount;         // 매칭된 결과의 수
    int currentIndex;       // 현재 선택된 매칭 인덱스
    char keyword[256];      // 검색어
    Cursor originalCursor;  // 검색 시작 전의 커서 위치
} FoundState;

FoundState foundState = {NULL, 0, -1, "", {NULL, NULL, 0, 0}};
bool isSearchMode = false;

int scrollOffset = 0; // 화면에 표시할 첫 번째 라인의 인덱스

// DoubleLinkedList의 시작과 끝을 관리하는 구조체
typedef struct {
    Node* head;
    Node* tail;
    Cursor cursor;
} DoubleLinkedList;

// 리스트 초기화 함수
void initList(DoubleLinkedList* list) {
    list->head = NULL;
    list->tail = NULL;
    list->cursor.current = NULL;
    list->cursor.currentLine = NULL;
    list->cursor.row = 1;
    list->cursor.col = 1;
}


// 노드를 리스트 끝에 추가하는 함수
void insertNode(DoubleLinkedList* list, char ch) {
    char currentData;
    // new Node 생성 및 리스트 연결 관리 
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "newLineNode: memory allocation failed\n");
        return;
    }
    newNode->data = ch;
    newNode->prev = list->tail;
    newNode->next = NULL;

    if (list->cursor.current == NULL) { // if list head is null
        // 초기 상태
        list->head = newNode;
        list->tail = newNode;
        list->cursor.current = newNode;
    } else {
        currentData = list->cursor.current->data;
        newNode->prev = list->cursor.current;
        newNode->next = list->cursor.current->next;

        if (list->cursor.current->next) {
            list->cursor.current->next->prev = newNode;
        } else {
            list->tail = newNode;
        }

        list->cursor.current->next = newNode;
        list->cursor.current = newNode;
    }

    // '\n처리'
    if (ch == '\n') {
        if (list->cursor.current->next == NULL) {
            LineNode* newLineNode = (LineNode*)malloc(sizeof(LineNode));
            if (newNode == NULL) {
                fprintf(stderr, "newLineNode: memory allocation failed\n");
                return;
            }

             // newLineNode 생성 및 연결 처리 
        
             // make newLineNode
            newLineNode->first = NULL;
            newLineNode->currentRow = list->cursor.row + 1;
            newLineNode->lastColIdx = 1;
            if (list->cursor.currentLine) {
                
                newLineNode->up = list->cursor.currentLine;
                newLineNode->down = list->cursor.currentLine->down; // NULL이어도 상관 x

                // check currentDownLine
                if (list->cursor.currentLine->down) {
                    list->cursor.currentLine->down->up = newLineNode;
                } 
                
                list->cursor.currentLine->down = newLineNode;

            } else {
                // 첫번째 라인을 절대 비우지 않으나, 혹시 모르니
                list->cursor.currentLine = newLineNode;
                lineInfo.head = newLineNode;
            }  

            // **never delete line head**

            // '\n'도 total에 포함하기
            list->cursor.currentLine->lastColIdx++;

            // cursor좌표처리
            list->cursor.row++;
            list->cursor.col = 1;
            
            // 현재 cursor lineNode를 변경
            list->cursor.currentLine = newLineNode;

            // lineSize 증가
            lineInfo.size++;
        } else { // row 중간에 '\n'가 들어오는 경우
            LineNode* newLineNode = (LineNode*)malloc(sizeof(LineNode));
            if (newNode == NULL) {
                fprintf(stderr, "newLineNode: memory allocation failed\n");
                return;
            }
            // newLineNode 연결 처리 
            newLineNode->first = list->cursor.current->next; //
            newLineNode->currentRow = list->cursor.currentLine->currentRow + 1;  
            
            // check current Line has 
            newLineNode->lastColIdx = list->cursor.currentLine->lastColIdx - list->cursor.col + 1;

            if (list->cursor.currentLine) {
                // change currentLine - lastColIdx
                list->cursor.currentLine->lastColIdx = list->cursor.col + 1;
                
                // chaining
                LineNode* currentDownLine = list->cursor.currentLine->down;

                newLineNode->up = list->cursor.currentLine;
                newLineNode->down = currentDownLine;
                
                if (currentDownLine) {
                    list->cursor.currentLine->down->up = newLineNode;
                }

                list->cursor.currentLine->down = newLineNode;

            }  
            // move to newLineNode;
            list->cursor.currentLine = newLineNode;
            list->cursor.current = list->cursor.currentLine->first;
            // down의 모든 line row값을 모두 변경해줘야 함 
            LineNode* lineNodeDown = list->cursor.currentLine->down;
            while (lineNodeDown != NULL) {
                lineNodeDown->currentRow++;
                lineNodeDown = lineNodeDown->down;
            }

            lineInfo.size++;
            // 현재 커서 처리
            list->cursor.row = list->cursor.currentLine->currentRow;
            list->cursor.col = 2;
        }
    } else {  // '\n'이 아닌 경우
        // 현재 라인이 NULL인 경우 처리
        if (list->cursor.currentLine == NULL) {
            // 새로운 라인 노드 생성
            LineNode* newLineNode = (LineNode*)malloc(sizeof(LineNode));
            if (newLineNode == NULL) {
                fprintf(stderr, "Error: Memory allocation failed for currentLine\n");
                return;
            }
            newLineNode->first = newNode;
            newLineNode->up = NULL;
            newLineNode->down = NULL;
            newLineNode->lastColIdx = 1;
            newLineNode->currentRow = 1;

            list->cursor.currentLine = newLineNode;
            lineInfo.head = newLineNode;
            lineInfo.size = 1;
            list->cursor.row = 1;
            list->cursor.col = 1;
        } else {
            // 지우고나서 '\n'이 된 경우 아직 아랫 라인이 남아잇다.
            if (list->cursor.currentLine->down && currentData == '\n' && list->cursor.currentLine->down->first == NULL) {
                list->cursor.currentLine->down->first = newNode;
                list->cursor.currentLine = list->cursor.currentLine->down;
                list->cursor.row = list->cursor.currentLine->currentRow;
                list->cursor.currentLine->lastColIdx++;
                list->cursor.col = 2;
            } else {
                // line total Col 증가
                list->cursor.currentLine->lastColIdx++;
                // line cursor 증가
                list->cursor.col++;
                // first 없는 경우 설정
            }
            
           
            if (!list->cursor.currentLine->first) {
                list->cursor.currentLine->first = newNode;
            }
        }
    }
}


void deleteNode(DoubleLinkedList* list) { // '\n'를 삭제할 때로 고려하기 - LineNode 처리 할 때
    // Node 연결 처리부터 관리
    Node* currentNode = list->cursor.current;
    char currentData = currentNode ->data;
    Node* globalPrevNode;
    if (currentNode && list->cursor.currentLine) { // not null
        if (currentNode != list->head && currentNode->prev) { // not head 
            currentNode->prev->next = currentNode->next; 
            if (currentNode->next) { // next가 존재
                currentNode->next->prev = currentNode->prev; 
            } else {
                list->tail = currentNode->prev; // tail 조정 
            }
            list->cursor.current = currentNode->prev; 
            // 무조건 앞으로 커서 이동시키기 
            free(currentNode);
        } else { // head인 경우
            // 초기로 set
            if (list->cursor.currentLine == lineInfo.head) {
                list->cursor.currentLine->first = NULL;
                list->cursor.current = NULL;
                free(currentNode);
                
            } else {
                printf("node = head, but line is not head error!!");
            }
        }
    }

    // line관리
    if (list->cursor.current && list->cursor.current->data == '\n') { // after delete data = '\n'
        // line chane
        if (list->cursor.currentLine->up) {
            // do not dlete currentLine -> 삭제하면 '\n'입력이 없는 이상, 새로운 lineNode가 생성 xx 
            // reset currentLine -> row, col? 
            list->cursor.currentLine->first = NULL;
            list->cursor.currentLine->lastColIdx--;
            
            list->cursor.currentLine = list->cursor.currentLine->up; // line up
            list->cursor.col = list->cursor.currentLine->lastColIdx; // current col set
            list->cursor.row = list->cursor.currentLine->currentRow; // current row set 
        } else {
            printf("currentLine is head but '\n' left...");// 라인 삭제 오류
        }
    } else if (currentData == '\n') { // '\n'를 지워버린 경우 -> line합쳐야 해
        if (list->cursor.currentLine == lineInfo.head) { //headline에서 지운 경우
            // have case 2 
            
            // case 1 
            if (list->cursor.currentLine->down) { // 아래 line이 있는 경우
                // line 통합
                list->cursor.col = list->cursor.currentLine->lastColIdx--;
                list->cursor.currentLine->lastColIdx = list->cursor.currentLine->lastColIdx + list->cursor.currentLine->down->lastColIdx;

                if (list->cursor.currentLine->down->down) {
                    list->cursor.currentLine->down->down->up = list->cursor.currentLine;
                }
                LineNode* deleteNode = list->cursor.currentLine->down;
                list->cursor.currentLine->down = deleteNode->down;

                free(deleteNode); // 아래 라잇 해제

                // 모든 라인 row값 변경
                LineNode* change = list->cursor.currentLine->down;
                while (change!=NULL) {
                    change->currentRow--;
                    change = change->down;
                }
                
            } else { //case2 
               list->cursor.currentLine->lastColIdx--; // just minus lastColIdx...
            }
        } else { // not head line case 1, 2
            // case 1
            // case 1 
            if (list->cursor.currentLine->down) { // 아래 line이 있는 경우
                // line 통합
                list->cursor.col = list->cursor.currentLine->lastColIdx--;
                list->cursor.currentLine->lastColIdx = list->cursor.currentLine->lastColIdx + list->cursor.currentLine->down->lastColIdx;

                if (list->cursor.currentLine->down->down) {
                    list->cursor.currentLine->down->down->up = list->cursor.currentLine;
                }
                LineNode* deleteNode = list->cursor.currentLine->down;
                list->cursor.currentLine->down = deleteNode->down;

                free(deleteNode); // 아래 라잇 해제

                // 모든 라인 row값 변경
                LineNode* change = list->cursor.currentLine->down;
                while (change!=NULL) {
                    change->currentRow--;
                    change = change->down;
                }
                
            } else { //case2 
               list->cursor.currentLine->lastColIdx--; // just minus lastColIdx...
            }
        }
    } else {
        list->cursor.currentLine->lastColIdx--;
    }

    

    if (list && list->cursor.currentLine) {
        refresh();
    }
}


// 리스트의 모든 데이터를 화면에 출력하는 함수
void displayList(DoubleLinkedList* list) {
    Node* current = list->head;
    move(0, 0); // 화면의 첫 번째 줄로 이동
    curs_set(1);
    
    while (current) {
        if (current == list->cursor.current) {
            if (current->data == '\n') {
                //addstr("<NL>");
                addch(current->data);
            } else {
                attron(A_REVERSE); // 커서 위치 강조
                addch(current->data);
                attroff(A_REVERSE);
            }
            
        } else {
            if (current->data == '\n') {
                //addstr("<NL>"); // 줄바꿈 문자를 시각적으로 표시
                addch(current->data);
            } else {
                addch(current->data);
            }
        }
        current = current->next;
    }
}

void displayLine() {
    int row, col;
    getmaxyx(stdscr, row, col);
    move(row - 3, 0);
    clrtoeol();
    int i = 1;
    LineNode* tmp = lineInfo.head;
    while(tmp) {
        if (tmp->first) {
            printw("row: %d: Linefirst: %c, lastColIdx: %d | ", tmp->currentRow, tmp->first->data, tmp->lastColIdx);

        } else {
            printw("row: %d: Linefirst: NULL, lastColIdx: %d ", tmp->currentRow, tmp->lastColIdx);
        }
        tmp=tmp->down;
    }
    refresh();
}

// 현재 노드 디버깅용 
// void displayCurrentInfo(DoubleLinkedList* list) {
//     if (list->cursor.current == NULL) {
//         return;
//     }
//     int row, col;
//     getmaxyx(stdscr, row, col);
//     move(row - 4, 0);
//     clrtoeol();
//     printw("current: %c, row: %d, col : %d, line: %d", 
//         list->cursor.current->data, list->cursor.row, list->cursor.col, list->cursor.currentLine->currentRow);
// }

// // line debug용
// void displayLineStatusBar(DoubleLinkedList* list) {
//     int row, col;
//     getmaxyx(stdscr, row, col); // 현재 터미널의 크기 가져오기
//     move(row - 3, 0);           // 화면 아래에서 3번째 줄로 이동하여 상태바 표시
//     clrtoeol();                 // 현재 줄을 지움

//     attron(A_REVERSE); // 반전된 색으로 상태바 강조

//     char lineText[100];
//     LineNode* currentLine = list->cursor.currentLine;

//     // 현재 라인의 정보를 출력
//     if (currentLine && currentLine->first) {
//         snprintf(lineText, sizeof(lineText), "Current Line: First Node Data: %c, LastColIdx: %d, CurrentRow: %d",
//                  currentLine->first->data,
//                  currentLine->lastColIdx,
//                  currentLine->currentRow);
//     } else if (currentLine) {
//         snprintf(lineText, sizeof(lineText), "Current Line: First Node: NULL, LastColIdx: %d, CurrentRow: %d",
//                  currentLine->lastColIdx,
//                  currentLine->currentRow);
//     } else {
//         snprintf(lineText, sizeof(lineText), "Current Line: NULL");
//     }
    
//     printw("%s", lineText);

//     attroff(A_REVERSE); // 반전된 색 효과 끄기
//     refresh(); // 화면 갱신
// }

// 상태바를 화면에 출력하는 함수
void displayStatusBar(DoubleLinkedList* list) {
    int row, col;
    getmaxyx(stdscr, row, col); 
    move(row - 2, 0); // last line - 2           
    clrtoeol();                   
    attron(A_REVERSE);

    char leftText[276]; // filename 크기 최대 255 최대 라인수 : 4자리 수 이내

    snprintf(leftText, sizeof(leftText), "[%s] - %d lines", fileMetaData.fileName, lineInfo.size);
    printw("%s", leftText);

    // 우측 텍스트 생성
    char rightText[25]; // row col을 최대 4자리로 계산해서 
    snprintf(rightText, sizeof(rightText), "Row: %d, Col: %d", list->cursor.row, list->cursor.col);

    int leftLength = strlen(leftText);
    int rightStart = col - strlen(rightText);

    // insert ' '
    for (int i = leftLength; i < rightStart; i++) {
        addch(' ');
    }

    move(row - 2, rightStart);
    printw("%s", rightText);

    attroff(A_REVERSE);
    refresh(); // 화면 갱신
}

void displayMessageBar(MessageBar* messageBar) {
    
    int row, col;
    getmaxyx(stdscr, row, col);

    move(row - 1, 0);
    clrtoeol();

    printw("HELP: ");

    for (int i = 0; i < messageBar->count; i++) {
        printw("%s : %s", messageBar->commands[i].shortCut, messageBar->commands[i].task);

        if (i < messageBar->count - 1) {
            printw(" | ");
        }
    }
}


void addCommand(MessageBar* messageBar, const char* task, const char* shortCut) {
    if (messageBar->count >= 10 ) {
        return;
    }

    strcpy(messageBar->commands[messageBar->count].task, task);
    strcpy(messageBar->commands[messageBar->count].shortCut, shortCut);
    messageBar->count++;
}

void moveCursorLeft(DoubleLinkedList* list) {
    if (list->cursor.current) {
        if (list->cursor.current->prev) { // node가 존재해야해
            // 일단 left
            list->cursor.current = list->cursor.current->prev;

            if (list->cursor.current->data == '\n') {
                if (list->cursor.currentLine && list->cursor.currentLine->up) {
                    list->cursor.currentLine = list->cursor.currentLine->up;
                    list->cursor.col = list->cursor.currentLine->lastColIdx;
                    list->cursor.row = list->cursor.currentLine->currentRow;
                }
            } else {
                list->cursor.col--;
            }
        }
    }
}

void moveCursorRight(DoubleLinkedList* list) {
    // ** '\n'까지가 하나의 row이다.
    if (list->cursor.current && list->cursor.currentLine) {
        if (list->cursor.current->data == '\n') { // '\n' 먼저 체크
            if (list->cursor.current->next) { // 다음줄로 이동 -> 없는 경우 이동 x 
                list->cursor.currentLine = list->cursor.currentLine->down;
                list->cursor.current = list->cursor.current->next;
                list->cursor.row = list->cursor.currentLine->currentRow;
                list->cursor.col = 2;
            }
        } else {
            // '\n'이 아니다.
            if (list->cursor.current->next) {
                list->cursor.current = list->cursor.current->next;
                list->cursor.col = list->cursor.col + 1;
            }
        }
    } 
}

void moveCursorUp(DoubleLinkedList* list) {
    if (list->cursor.current && list->cursor.currentLine) {
        // Check if not the head line
        if (list->cursor.currentLine->up) {
            // Move up
            list->cursor.currentLine = list->cursor.currentLine->up;

            // Adjust row
            list->cursor.row = list->cursor.currentLine->currentRow;

            if (list->cursor.col > list->cursor.currentLine->lastColIdx) {
                Node* endNode = list->cursor.currentLine->first;

                // Check if first node is NULL
                if (!endNode) {
                    printf("Error: currentLine->first is NULL\n");
                    return;
                }

                // Traverse to the end of the line
                while (endNode && endNode->data != '\n') {
                    endNode = endNode->next;
                }

                // Move one step back
                if (endNode && endNode->prev) {
                    endNode = endNode->prev;
                }

                list->cursor.current = endNode;
                list->cursor.col = list->cursor.currentLine->lastColIdx - 1;

            } else {
                // Find the node at the same column position
                Node* sameColNode = list->cursor.currentLine->first;

                // Check if first node is NULL
                if (!sameColNode) {
                    printf("Error: currentLine->first is NULL\n");
                    return;
                }

                // Traverse to the same column position
                for (int i = 2; i < list->cursor.col; i++) {
                    if (!sameColNode || !sameColNode->next) {
                        printf("Error: Reached NULL before finding column %d\n", list->cursor.col);
                        return;
                    }
                    sameColNode = sameColNode->next;
                }

                list->cursor.current = sameColNode;
            }
        }
    }
}

void moveCursorDown(DoubleLinkedList* list) {
    if (list->cursor.current) {
        // check tail 
        if (list->cursor.currentLine->down) {
            // move down
            list->cursor.currentLine = list->cursor.currentLine->down;

            // adjust row 
            list->cursor.row = list->cursor.currentLine->currentRow;

            // "abcddd(d)d\n"
            // "abc\n"
            if (list->cursor.col > list->cursor.currentLine->lastColIdx) {
                Node* endNode = list->cursor.currentLine->first;
                for (int i = 2; i< list->cursor.currentLine->lastColIdx; i++) {
                    endNode = endNode->next;
                }
                list->cursor.current = endNode;
                list->cursor.col = list->cursor.currentLine->lastColIdx;
            } else {
                Node* sameColNode = list->cursor.currentLine->first;
                for (int i = 2; i< list->cursor.col; i++) {
                    sameColNode = sameColNode->next;
                }
                list->cursor.current = sameColNode;
            }
        }
    }
}


void moveCursorHome(DoubleLinkedList* list) {
    list->cursor.current = list->cursor.currentLine->first;
    list->cursor.col = 2;
}

void moveCursorEnd(DoubleLinkedList* list) {
    Node* endNode = list->cursor.currentLine->first;
    for (int i = 2; i < list->cursor.currentLine->lastColIdx; i++) {
        endNode = endNode->next;
    }
    if (endNode->data == '\n') {
        list->cursor.current = endNode->prev;
        list->cursor.col = list->cursor.currentLine->lastColIdx -1;
    } else {
        list->cursor.current = endNode;
        list->cursor.col = list->cursor.currentLine->lastColIdx;
    }
}

void saveToFile(DoubleLinkedList* list) {
    if (!fileMetaData.isModified) {
        return;
    }
    int row, col;
    getmaxyx(stdscr, row, col);
    if (!fileMetaData.fileName) {
        char buffer[256];    
        mvprintw(row - 3, 0, "Enter file name to save: ");
        refresh();
        echo();
        getnstr(buffer, sizeof(buffer) - 1); // 사용자로부터 파일 이름 입력 받기
        noecho(); // 입력 표시 끄기

        if (strlen(buffer) == 0) {
            mvprintw(row - 3, 0, "Error: No file name entered.");
            refresh();
            return; // 파일 이름이 없으면 저장하지 않음
        }

        fileMetaData.fileName = (char*)malloc(strlen(buffer) + 1);
        if (!fileMetaData.fileName) {
            mvprintw(row -3, 0, "Error: Memory allocation failed.");
            refresh();
            return;
        }
        strcpy(fileMetaData.fileName, buffer);
    }
    FILE* file = fopen(fileMetaData.fileName, "w");
    if (!file) {
        mvprintw(0,0, "Error: unable to open file for writing.");
        refresh();
        return;
    }

    Node* current = list->head;
    while (current) {
        fputc(current->data, file);
        current = current->next;
    }

    fclose(file);
    mvprintw(row - 3, 0, "File saved successfully to %s", fileMetaData.fileName);
    refresh();
    fileMetaData.isModified = false;
    fileMetaData.realQuit = false;
}

void readFile(DoubleLinkedList* list, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char c;
    while ((c = fgetc(file)) != EOF) {
        addch(c);
        if (c>=32 || c == '\n') {
            insertNode(list, c);
        }
    }

    fclose(file);
}

void setFileName(const char* fileName) {
    fileMetaData.fileName = (char*)malloc(strlen(fileName) + 1);
    if (!fileMetaData.fileName) {
        perror("Failed to allocate memoery!!");
        exit(1);
    }
    strcpy(fileMetaData.fileName, fileName);
}

void alertQuit() {
    if (fileMetaData.realQuit) {
        int row, col;
        getmaxyx(stdscr, row, col);
        mvprintw(row - 4, 0, "Unsaved changes! Press Ctrl+Q again to quit.");
        refresh();
    }
}

// 슬라이딩 윈도우를 사용한 검색 함수
void search(DoubleLinkedList* list, const char* keyword) {
    // 기존 검색 결과가 있으면 해제
    if (foundState.matches) {
        free(foundState.matches);
        foundState.matches = NULL;
    }
    foundState.matchCount = 0;
    foundState.currentIndex = -1;

    // 키워드 길이와 초기 용량 설정
    int keywordLength = strlen(keyword);
    int capacity = 10;
    foundState.matches = (Node**)malloc(sizeof(Node*) * capacity);

    // 슬라이딩 윈도우를 사용하여 텍스트 버퍼를 탐색
    Node* windowStart = list->head;
    while (windowStart) {
        Node* temp = windowStart;
        int i;
        for (i = 0; i < keywordLength; i++) {
            if (!temp || temp->data != keyword[i]) {
                break;
            }
            temp = temp->next;
        }

        if (i == keywordLength) {
            // 매칭됨
            if (foundState.matchCount >= capacity) {
                capacity *= 2;
                foundState.matches = (Node**)realloc(foundState.matches, sizeof(Node*) * capacity);
            }
            foundState.matches[foundState.matchCount++] = windowStart;
        }

        windowStart = windowStart->next;
    }

    if (foundState.matchCount > 0) {
        foundState.currentIndex = 0;
        // 매칭된 첫 번째 위치로 커서를 이동
        list->cursor.current = foundState.matches[0];

        // 커서의 라인과 위치를 업데이트
        LineNode* lineNode = lineInfo.head;
        int row = 1;
        int col = 1;
        Node* node = list->head;
        while (node && node != foundState.matches[0]) {
            if (node->data == '\n') {
                lineNode = lineNode->down;
                row++;
                col = 1;
            } else {
                col++;
            }
            node = node->next;
        }
        list->cursor.currentLine = lineNode;
        list->cursor.row = row;
        list->cursor.col = col;
    } else {
        // 매칭 결과가 없음
        int row, col;
        getmaxyx(stdscr, row, col);
        mvprintw(row - 3, 0, "No matches found for '%s'", keyword);
        refresh();
    }
}


// 검색 모드에서 다음 매칭으로 이동
void moveToNextMatch(DoubleLinkedList* list) {
    if (foundState.matchCount > 0) {
        foundState.currentIndex = (foundState.currentIndex + 1) % foundState.matchCount;
        Node* matchNode = foundState.matches[foundState.currentIndex];

        // 커서 이동 로직
        list->cursor.current = matchNode;

        LineNode* lineNode = lineInfo.head;
        int row = 1;
        int col = 1;
        Node* node = list->head;
        while (node && node != matchNode) {
            if (node->data == '\n') {
                lineNode = lineNode->down;
                row++;
                col = 1;
            } else {
                col++;
            }
            node = node->next;
        }
        list->cursor.currentLine = lineNode;
        list->cursor.row = row;
        list->cursor.col = col;
    }
}

// 검색 모드에서 이전 매칭으로 이동
void moveToPreviousMatch(DoubleLinkedList* list) {
    if (foundState.matchCount > 0) {
        foundState.currentIndex = (foundState.currentIndex - 1 + foundState.matchCount) % foundState.matchCount;
        Node* matchNode = foundState.matches[foundState.currentIndex];

        // 커서 이동 로직
        list->cursor.current = matchNode;

        LineNode* lineNode = lineInfo.head;
        int row = 1;
        int col = 1;
        Node* node = list->head;
        while (node && node != matchNode) {
            if (node->data == '\n') {
                lineNode = lineNode->down;
                row++;
                col = 1;
            } else {
                col++;
            }
            node = node->next;
        }
        list->cursor.currentLine = lineNode;
        list->cursor.row = row;
        list->cursor.col = col;
    }
}

int main(int argc, char* argv[]) {
    system("stty -ixon");

    // Buffer
    DoubleLinkedList list;
    initList(&list);

    // set currentLine 
    LineNode* currentLine = (LineNode*)malloc(sizeof(LineNode));
    currentLine->first = NULL;
    currentLine->up = NULL;
    currentLine->down = NULL;
    currentLine->lastColIdx = 1;
    currentLine->currentRow = 1;

    // headLine을 Lineinfo에 등록
    lineInfo.head = currentLine;
    lineInfo.size = 1;

    // list에 line 등록 
    list.cursor.currentLine = lineInfo.head;

    // input filename 
    if (argc == 2) {
        setFileName(argv[1]); //setfileName;
        readFile(&list, fileMetaData.fileName); // check file and read!
    } else if (argc > 2) {
        perror("Invalid argc");
        exit(1);
    }

    // message bar
    MessageBar messageBar;
    messageBar.count = 0;
    addCommand(&messageBar, "save", "Ctrl+S");
    addCommand(&messageBar, "quit", "Ctrl+Q");
    addCommand(&messageBar, "find", "Ctrl+F");

    initscr();            // curses 모드 시작
    cbreak();             // 입력 버퍼 없이 문자 즉시 입력
    noecho();             // 입력된 문자를 화면에 표시하지 않음
    keypad(stdscr, TRUE); // 특수 키 입력 허용

    displayList(&list);
    displayMessageBar(&messageBar);
    displayStatusBar(&list);  // 상태바 표시
    curs_set(0); // 커서 숨김

    int ch;
    while ((ch = getch())) {
        if (isSearchMode) {
            // 검색 모드에서의 입력 처리
            if (ch == KEY_LEFT || ch == KEY_UP) {
                moveToPreviousMatch(&list);
            } else if (ch == KEY_RIGHT || ch == KEY_DOWN) {
                moveToNextMatch(&list);
            } else if (ch == 10) { // Enter 키
                isSearchMode = false;
                // 검색 결과에서 편집 가능하도록 설정
            } else if (ch == 27) { // Esc 키
                // 검색 취소하고 커서를 원래 위치로 복원
                list.cursor.col = foundState.originalCursor.col;
                list.cursor.row = foundState.originalCursor.row;
                list.cursor.current = foundState.originalCursor.current;
                list.cursor.currentLine = foundState.originalCursor.currentLine;
                isSearchMode = false;
            }
        } else {
            // 일반 모드에서의 입력 처리
            if (ch >= 32 && ch <= 126 || ch == 10) {
                insertNode(&list, ch);
                if (!fileMetaData.isModified) {
                    fileMetaData.isModified = true;   
                }
                fileMetaData.realQuit = false;
            } 
            switch (ch) {
                case KEY_LEFT:
                    moveCursorLeft(&list);
                    break;
                case KEY_RIGHT:
                    moveCursorRight(&list);
                    break;
                case KEY_UP:
                    moveCursorUp(&list);
                    break;
                case KEY_DOWN:
                    moveCursorDown(&list);
                    break;
                case KEY_HOME:
                    moveCursorHome(&list);
                    break;
                case KEY_END:
                    moveCursorEnd(&list);
                    break;
                case KEY_RESIZE:
                    resize_term(0, 0); 
                    clear();         
                    displayStatusBar(&list);
                    displayList(&list);
                    break;
                case 8:
                case 127:
                case KEY_BACKSPACE:
                    deleteNode(&list);
                    if (!fileMetaData.isModified) {
                        fileMetaData.isModified = true;
                    }
                    fileMetaData.realQuit = false;
                    break;
                case CTRL('s'):
                    saveToFile(&list);
                
                    break;
                case CTRL('q'):
                    if (fileMetaData.isModified && !fileMetaData.realQuit) {
                        fileMetaData.realQuit = true;
                    } else if (fileMetaData.realQuit) {
                        return 0;
                    } else {
                        return 0;
                    }
                    break;
                case CTRL('f'):
                    {
                        // 검색 모드 진입
                        isSearchMode = true;
                        // 현재 커서 위치 저장
                        foundState.originalCursor = list.cursor;

                        // 검색어 입력 받기
                        int row, col;
                        getmaxyx(stdscr, row, col);
                        echo();
                        mvprintw(row - 3, 0, "Enter search term: ");
                        char buffer[256];
                        getnstr(buffer, sizeof(buffer) - 1);
                        noecho();

                        strcpy(foundState.keyword, buffer);
                        search(&list, foundState.keyword);
                    }
                    break;
            }
        }

        clear();                  // 화면 지우기
        displayList(&list);
        displayMessageBar(&messageBar);
        displayStatusBar(&list);  // 상태바 표시
        // displayCurrentInfo(&list);
        // displayLineStatusBar(&list);
        alertQuit();
        refresh();                // 화면 갱신
    }

    endwin(); // curses 모드 종료

    return 0;
}