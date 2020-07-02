#include<stdio.h>
#include<windows.h>
#include<conio.h>
#include <mmsystem.h>
#include<stdlib.h>
#include<assert.h>
#include<errors.h>
#include<random>
#include<chrono>

#pragma comment(lib, "winmm.lib")
#pragma warning(disable:4996)

#define FALLTHROUGH
#define ASSERT(condition, msg) \
    if(!(condition)){ \
        fprintf(stderr, "%s(%s: %d)\n", msg, __FILE__, __LINE__); \
        __asm{int 3} \
    } \
//    ASSERT(false, "error!!!");

enum class eDir:int
{
    LEFT =  75 , //좌로 이동    //키보드값들 
    RIGHT =  77 , //우로 이동 
    UP =  72 , //회전 
    DOWN =  80 , //soft drop
    SPACE =  32 , //hard drop
    p = 112, //일시정지 
    P =  80 , //일시정지
    ESC =  27, //게임종료 
    ROTATABLE_CRASH
    //블록이 바닥, 혹은 다른 블록과 닿은 상태에서 한칸위로 올려 회전이 가능한 경우 
};

enum class eBlockStatus:int
{
    ACTIVE_BLOCK = -2, // 게임판배열에 저장될 블록의 상태들 
    CEILLING = -1,     // 블록이 이동할 수 있는 공간은 0 또는 음의 정수료 표현 
    EMPTY = 0,        // 블록이 이동할 수 없는 공간은 양수로 표현 
    WALL = 1,
    INACTIVE_BLOCK = 2,// 이동이 완료된 블록값 
    NON_BLCOK = 100 // for copy
    
};

constexpr int MAIN_X =  11 ; //게임판 가로크기 
constexpr int MAIN_Y =  23 ; //게임판 세로크기 

constexpr int MAIN_X_ADJ =  3  ;//게임판 위치조정 
constexpr int MAIN_Y_ADJ =  1  ;//게임판 위치조정 
constexpr int STATUS_X_ADJ = MAIN_X_ADJ + MAIN_X + 1;//게임정보표시 위치조정 

constexpr int BLOCKS[7][4][4][4] = {
{{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
 {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0}},
{{0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},{0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
 {0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},{0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0}},
{{0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,0},
 {0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,0}},
{{0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0},{0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0},
 {0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0},{0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0}},
{{0,0,0,0,0,0,1,0,1,1,1,0,0,0,0,0},{0,0,0,0,1,1,0,0,0,1,0,0,0,1,0,0},
 {0,0,0,0,0,0,0,0,1,1,1,0,1,0,0,0},{0,0,0,0,0,1,0,0,0,1,0,0,0,1,1,0}},
{{0,0,0,0,1,0,0,0,1,1,1,0,0,0,0,0},{0,0,0,0,0,1,0,0,0,1,0,0,1,1,0,0},
 {0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,0},{0,0,0,0,0,1,1,0,0,1,0,0,0,1,0,0}},
{{0,0,0,0,0,1,0,0,1,1,1,0,0,0,0,0},{0,0,0,0,0,1,0,0,0,1,1,0,0,1,0,0},
 {0,0,0,0,0,0,0,0,1,1,1,0,0,1,0,0},{0,0,0,0,0,1,0,0,1,1,0,0,0,1,0,0}}
}; //블록모양 저장 4*4공간에 블록을 표현 blcoks[blockType][blockRotation][i][j]로 사용 

enum class eCursorType
{
    NO_CURSOR,
    SOLID_CURSOR,
    NOMAL_CURSOR
};


int blockType; //블록 종류를 저장 
int blockRotation; //블록 회전값 저장 
int blockTypeNext; //다음 블록값 저장 

//게임판의 정보를 저장하는 배열 모니터에 표시후에 main_cpy로 복사됨 
//즉 maincpy는 게임판이 모니터에 표시되기 전의 정보를 가지고 있음 
//main의 전체를 계속 모니터에 표시하지 않고(이렇게 하면 모니터가 깜빡거림) 
//main_cpy와 배열을 비교해서 값이 달라진 곳만 모니터에 고침 
eBlockStatus main_org[MAIN_Y][MAIN_X];
eBlockStatus main_cpy[MAIN_Y][MAIN_X];

int STATUS_Y_GOAL; //GOAL 정보표시위치 Y 좌표 저장 
int STATUS_Y_LEVEL; //LEVEL 정보표시위치 Y 좌표 저장
int STATUS_Y_SCORE; //SCORE 정보표시위치 Y 좌표 저장

int blockX;
int blockY; //이동중인 블록의 게임판상의 x,y좌표를 저장 

// todo menoBoard에 보여줄것 구조체로 묶기
/*
level
goal
present score
last score
bestscore
*/
int key; //키보드로 입력받은 키값을 저장 
int presentLevel; //현재 presentLevel 
int presentScore; //현재 점수 
int lastScore = 0; //마지막게임점수 
int bestScore = 0; //최고게임점수 
int ScoreNextLevel; //다음레벨로 넘어가기 위한 목표점수 

int deletedLineCount; //현재 레벨에서 제거한 줄 수를 저장  
int blockDownDelay; // 블록 내려오는 딜레이

bool bNeedNewBlock = false; //새로운 블럭이 필요함을 알리는 flag 
bool bCrash = false; //현재 이동중인 블록이 충돌상태인지 알려주는 flag 
bool bLevelUp = false; //다음레벨로 진행(현재 레벨목표가 완료되었음을) 알리는 flag 
bool bSpaceKey = false; //hard drop상태임을 알려주는 flag 

void drawTitle(void); //게임시작화면 
void resetBoard(void); //게임판 초기화 
void initialMainOrg(void); //메인 게임판(main_org[][]를 초기화)
void initialMainCpy(void); //copy 게임판(main_cpy[][]를 초기화)
void drawMap(void); //게임 전체 인터페이스를 표시 
void drawMain(void); //게임판을 그림 
void makeNewBlock(void); //새로운 블록을 하나 만듦 
void checkKey(void); //키보드로 키를 입력받음 
void dropBlock(void); //블록을 아래로 떨어트림 
bool isCrash(int blockX, int blockY, int rotation); //blockX, by위치에 rotation회전값을 같는 경우 충돌 판단 
void moveBlock(eDir dir); //dir방향으로 블록을 움직임 
void checkLine(void); //줄이 가득찼는지를 판단하고 지움 
void checkLevelUp(void); //레벨목표가 달성되었는지를 판단하고 levelup시킴 
void checkGameOver(void); //게임오버인지 판단하고 게임오버를 진행 
void pauseGame(void);//게임을 일시정지시킴 
void eraseBlock();

void setActiveBlock(int X, int Y);


void gotoxy(int x, int y) { //gotoxy함수 
    COORD pos = { 2 * (short)x,(short)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int getRandom(const int min, const int max)
{
    static std::random_device      rn;
    static int                     seed = rn();
    static std::mt19937            rnd(seed);

    std::uniform_int_distribution<int> range(min, max);

    return range(rnd);
}


void setCursorType(eCursorType c) {
    CONSOLE_CURSOR_INFO curInfo;
    switch (c) 
    {
        case eCursorType::NO_CURSOR:
            curInfo.dwSize = 1;
            curInfo.bVisible = FALSE;
            break;
        case eCursorType::SOLID_CURSOR:
            curInfo.dwSize = 100;
            curInfo.bVisible = TRUE;
            break;
        case eCursorType::NOMAL_CURSOR:
            curInfo.dwSize = 20;
            curInfo.bVisible = TRUE;
            break;
        default:
            ASSERT(false,"eCursorType input error");
            break;
    }
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
}

int main() {
    static_assert(sizeof(main_org) == 23*11*sizeof(eBlockStatus), "error");

    srand((unsigned)time(NULL));
    setCursorType(eCursorType::NO_CURSOR);
    drawTitle();
    resetBoard(); //게임판 리셋 

    while (1) 
    {
        for (int i = 0; i < 5; i++) 
        { 
            //블록이 한칸떨어지는동안 5번 키입력받을 수 있음 
            checkKey(); //키입력확인 
            drawMain();
            Sleep(blockDownDelay);
            
            if (bCrash && isCrash(blockX, blockY + 1, blockRotation))
            {
                Sleep(100);
            }
            
            //블록이 충돌중인경우 추가로 이동및 회전할 시간을 갖음 
            if (bSpaceKey) { //스페이스바를 누른경우(hard drop) 추가로 이동및 회전할수 없음 break; 
                bSpaceKey = false;
                break;
            }
        }
        dropBlock(); // 블록을 한칸 내림 
        checkLevelUp(); // 레벨업을 체크 
        checkGameOver(); //게임오버를 체크 
        if (bNeedNewBlock)
        {
            makeNewBlock(); // 뉴 블럭 flag가 있는 경우 새로운 블럭 생성 
        }
    }
}

void drawTitle(void) {
    const int x = 5; //타이틀화면이 표시되는 x좌표 
    const int y = 4; //타이틀화면이 표시되는 y좌표 
    
    gotoxy(x, y + 0); printf("■□□□■■■□□■■□□■■"); Sleep(100);
    gotoxy(x, y + 1); printf("■■■□  ■□□    ■■□□■"); Sleep(100);
    gotoxy(x, y + 2); printf("□□□■              □■  ■"); Sleep(100);
    gotoxy(x, y + 3); printf("■■□■■  □  ■  □□■□□"); Sleep(100);
    gotoxy(x, y + 4); printf("■■  ■□□□■■■□■■□□"); Sleep(100);
    gotoxy(x, y + 5); printf("      blog.naver.com/azure0777"); Sleep(100);
    gotoxy(x + 5, y + 2); printf("T E T R I S"); Sleep(100);
    gotoxy(x, y + 7); printf("Please Enter Any Key to Start..");
    gotoxy(x, y + 9); printf("  △   : Shift");
    gotoxy(x, y + 10); printf("◁  ▷ : Left / Right");
    gotoxy(x, y + 11); printf("  ▽   : Soft Drop");
    gotoxy(x, y + 12); printf(" SPACE : Hard Drop");
    gotoxy(x, y + 13); printf("   P   : Pause");
    gotoxy(x, y + 14); printf("  ESC  : Quit");
    gotoxy(x, y + 16); printf("BONUS FOR HARD DROPS / COMBOS");

    //타이틀 프레임을 세는 변수  
    for (int i = 0; 1; i++) 
    {   //하나도 안중요한 별 반짝이는 애니메이션효과 
        if (kbhit()) 
        {
            break;
        }
        else if (i % 200 == 0) 
        { 
			gotoxy(x + 4, y + 1);
			printf("★");
		}//cnt가 200으로 나누어 떨어질때 별을 표시 
        else if ((i % 200 - 100) == 0)
        {
            gotoxy(x + 4, y + 1);
            printf("  "); 
        } //위 카운트에서 100카운트 간격으로 별을 지움 
        else if ((i % 350) == 0)
        { 
			gotoxy(x + 13, y + 2);
            printf("☆"); 
        } //윗별과 같지만 시간차를 뒀음 
        else if ((i % 350 - 100) == 0)
        { 
            gotoxy(x + 13, y + 2); 
            printf("  "); 
        }
        Sleep(10); // 00.1초 딜레이  
    }


    //todo need this code?
    while (kbhit())
    {
        getch(); //버퍼에 기록된 키값을 버림 
    }

}

void resetBoard(void) {

    FILE* file = fopen("score.dat", "rt"); // presentScore.dat파일을 연결 
    if (file == NULL) 
    { 
        bestScore = 0; 
    }
    else 
    {
        fscanf(file, "%d", &bestScore); // 파일이 열리면 최고점수를 불러옴 
        fclose(file); //파일 닫음 
    }

    presentLevel = 1; //각종변수 초기화 
    presentScore = 0;
    ScoreNextLevel = 1000;
    key = 0;
    bCrash = false;
    deletedLineCount = 0;
    blockDownDelay = 100;

    system("cls"); //화면지움 
    initialMainOrg(); // main_org를 초기화 
    initialMainCpy();
    drawMap(); // 게임화면을 그림
    drawMain(); // 게임판을 그림 

    blockTypeNext = rand() % 7; //다음번에 나올 블록 종류를 랜덤하게 생성 
    makeNewBlock(); //새로운 블록을 하나 만듦  

}

void initialMainOrg(void) 
{ 
    memset(main_org, 0, sizeof(main_org));
    for (int j = 1; j < MAIN_X; j++) 
    { //y값이 3인 위치에 천장을 만듦 
        main_org[3][j] = eBlockStatus::CEILLING;
    }
    for (int i = 1; i < MAIN_Y - 1; i++) 
    { //좌우 벽을 만듦  
        main_org[i][0] = eBlockStatus::WALL;
        main_org[i][MAIN_X - 1] = eBlockStatus::WALL;
    }
    for (int j = 0; j < MAIN_X; j++) 
    { //바닥벽을 만듦 
        main_org[MAIN_Y - 1][j] = eBlockStatus::WALL;
    }
}

//main_org와 같은 숫자가 없게 하기 위함  게임판에 게임에 사용되지 않는 숫자를 넣음 
void initialMainCpy(void) {
    for (int i = 0; i < MAIN_Y; i++) 
    {         
        for (int j = 0; j < MAIN_X; j++) 
        {  
            main_cpy[i][j] = eBlockStatus::NON_BLCOK;
        }
    }
}

void drawMap(void) 
{ //게임 상태 표시를 나타내는 함수  
    int y = 3;// presentLevel, goal, score만 게임중에 값이 바뀔수 도 있음 그 y값을 따로 저장해둠 
 // 그래서 혹시 게임 상태 표시 위치가 바뀌어도 그 함수에서 안바꿔도 되게.. 
    gotoxy(STATUS_X_ADJ, STATUS_Y_LEVEL = y); printf(" LEVEL : %5d", presentLevel);
    gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL = y + 1); printf(" GOAL  : %5d", 10 - deletedLineCount);
    gotoxy(STATUS_X_ADJ, y + 2); printf("+-  N E X T  -+ ");
    gotoxy(STATUS_X_ADJ, y + 3); printf("|             | ");
    gotoxy(STATUS_X_ADJ, y + 4); printf("|             | ");
    gotoxy(STATUS_X_ADJ, y + 5); printf("|             | ");
    gotoxy(STATUS_X_ADJ, y + 6); printf("|             | ");
    gotoxy(STATUS_X_ADJ, y + 7); printf("+-- -  -  - --+ ");
    gotoxy(STATUS_X_ADJ, y + 8); printf(" YOUR SCORE :");
    gotoxy(STATUS_X_ADJ, STATUS_Y_SCORE = y + 9); printf("        %6d", presentScore);
    gotoxy(STATUS_X_ADJ, y + 10); printf(" LAST SCORE :");
    gotoxy(STATUS_X_ADJ, y + 11); printf("        %6d", lastScore);
    gotoxy(STATUS_X_ADJ, y + 12); printf(" BEST SCORE :");
    gotoxy(STATUS_X_ADJ, y + 13); printf("        %6d", bestScore);
    gotoxy(STATUS_X_ADJ, y + 15); printf("  △   : Shift        SPACE : Hard Drop");
    gotoxy(STATUS_X_ADJ, y + 16); printf("◁  ▷ : Left / Right   P   : Pause");
    gotoxy(STATUS_X_ADJ, y + 17); printf("  ▽   : Soft Drop     ESC  : Quit");
    gotoxy(STATUS_X_ADJ, y + 20); printf("blog.naver.com/azure0777");
}

void drawMain(void) 
{ //게임판 그리는 함수 
    for (int j = 1; j < MAIN_X - 1; j++) 
    { //천장은 계속 새로운블럭이 지나가서 지워지면 새로 그려줌 
        if (main_org[3][j] == eBlockStatus::EMPTY)
        {
            main_org[3][j] = eBlockStatus::CEILLING;
        }
    }
    for (int i = 0; i < MAIN_Y; i++) 
    {
        for (int j = 0; j < MAIN_X; j++) 
        {
            if (main_cpy[i][j] != main_org[i][j]) 
            { //cpy랑 비교해서 값이 달라진 부분만 새로 그려줌.
			 //이게 없으면 게임판전체를 계속 그려서 느려지고 반짝거림
                gotoxy(MAIN_X_ADJ + j, MAIN_Y_ADJ + i);
                switch (main_org[i][j]) 
                {
                    case eBlockStatus::EMPTY: //빈칸모양 
                        printf("  ");
                        break;
                    case eBlockStatus::CEILLING: //천장모양 
                        printf(". ");
                        break;
                    case eBlockStatus::WALL: //벽모양 
                        printf("▩");
                        break;
                    case eBlockStatus::INACTIVE_BLOCK: //굳은 블럭 모양  
                        printf("□");
                        break;
                    case eBlockStatus::ACTIVE_BLOCK: //움직이고있는 블럭 모양  
                        printf("■");
                        break;
                    default:
                        ASSERT(false, "erorr?")
                            break;
                }
            }
        }
    }

    memcpy(main_cpy, main_org, sizeof(main_org));
}

void makeNewBlock(void) 
{ //새로운 블록 생성  
    blockX = (MAIN_X / 2) - 1; //블록 생성 위치x좌표(게임판의 가운데) 
    blockY = 0;  //블록 생성위치 y좌표(제일 위) 
    blockType = blockTypeNext; //다음블럭값을 가져옴 
    blockTypeNext = rand() % 7; //다음 블럭을 만듦 
    blockRotation = 0;  //회전은 0번으로 가져옴 

    bNeedNewBlock = false; //makeNewBlock flag를 끔  

    for (int i = 0; i < 4; i++) 
    { //게임판 blockX, by위치에 블럭생성  
        for (int j = 0; j < 4; j++) 
        {
            if (BLOCKS[blockType][blockRotation][i][j] == 1)
            {
                main_org[blockY + i][blockX + j] = eBlockStatus::ACTIVE_BLOCK;
            }
        }
    }
    for (int i = 1; i < 3; i++) 
    { //게임상태표시에 다음에 나올블럭을 그림 
        for (int j = 0; j < 4; j++) 
        {
            if (BLOCKS[blockTypeNext][0][i][j] == 1) 
            {
                gotoxy(STATUS_X_ADJ + 2 + j, i + 6);
                printf("■");
            }
            else 
            {
                gotoxy(STATUS_X_ADJ + 2 + j, i + 6);
                printf("  ");
            }
        }
    }
}

void checkKey(void) 
{
    int key = 0; //키값 초기화  

    if (kbhit()) 
    { //키입력이 있는 경우  
        key = getch(); //키값을 받음
        eDir dir = static_cast<eDir>(key);
        if (key == 224) 
        { //방향키인경우 
            do 
            {
                key = getch(); 
            }while (key == 224);//방향키지시값을 버림 

            dir = static_cast<eDir>(key);
            switch (dir)
            {
            case eDir::LEFT: //왼쪽키 눌렀을때
                if (!isCrash(blockX - 1, blockY, blockRotation))
                {
                    moveBlock(dir);
                }
                break;
            case eDir::RIGHT: //오른쪽 방향키 눌렀을때- 위와 동일하게 처리됨 
                if (!isCrash(blockX + 1, blockY, blockRotation))
                {
                    moveBlock(dir);
                }
                break;
            case eDir::DOWN: //아래쪽 방향키 눌렀을때-위와 동일하게 처리됨 
                if (!isCrash(blockX, blockY + 1, blockRotation))
                {
                    moveBlock(dir);
                }
                break;
            case eDir::UP: //위쪽 방향키 눌렀을때 
                if (!isCrash(blockX, blockY, (blockRotation + 1) % 4))
                {
                    moveBlock(dir);
                }
                //회전할 수 있는지 체크 후 가능하면 회전
                else if (bCrash == 1 && !isCrash(blockX, blockY - 1, (blockRotation + 1) % 4))
                {
                    //바닥에 닿은 경우 위쪽으로 한칸띄워서 회전이 가능하면 그렇게 함(특수동작)
                    moveBlock(eDir::ROTATABLE_CRASH);
                }
            default:
                break;
            }
            
        }
        else 
        { //방향키가 아닌경우 
            switch (dir) 
            {
            case eDir::SPACE: //스페이스키 눌렀을때 
                bSpaceKey = true; //스페이스키 flag를 띄움 
                while (bCrash == 0) 
                { //바닥에 닿을때까지 이동시킴 
                    dropBlock();
                    presentScore += presentLevel; // hard drop 보너스
                    gotoxy(STATUS_X_ADJ, STATUS_Y_SCORE); printf("        %6d", presentScore); //점수 표시  
                }
                break;
            case eDir::P: //P(대문자) 눌렀을때 
            case eDir::p: //p(소문자) 눌렀을때 
                pauseGame(); //일시정지 
                break;
            case eDir::ESC: //ESC눌렀을때 
                system("cls"); //화면을 지우고 
                exit(0); //게임종료 
            }
        }
    }
    while (kbhit()) 
    {
        getch(); //키버퍼를 비움 
    }
}

void dropBlock(void) 
{
if (bCrash && !isCrash(blockX, blockY + 1, blockRotation))
    {
        bCrash = false; //밑이 비어있으면 crush flag 끔 
    }
    if (bCrash && isCrash(blockX, blockY + 1, blockRotation)) { //밑이 비어있지않고 crush flag가 켜저있으면 
        for (int i = 0; i < MAIN_Y; i++) 
        { //현재 조작중인 블럭을 굳힘 
            for (int j = 0; j < MAIN_X; j++) 
            {
                if (main_org[i][j] == eBlockStatus::ACTIVE_BLOCK)
                {
                    main_org[i][j] = eBlockStatus::INACTIVE_BLOCK;
                }
            }
        }
        bCrash = false;
        checkLine(); 
        bNeedNewBlock = true; 
        return; //함수 종료 
    }

    if (!isCrash(blockX, blockY + 1, blockRotation))
    {
        moveBlock(eDir::DOWN); //밑이 비어있으면 밑으로 한칸 이동 
    }
    else
    {
        bCrash = true; //밑으로 이동이 안되면  crush flag를 켬
    }
}

bool isCrash(int blockX, int blockY, int blockRotation) 
{   //지정된 좌표와 회전값으로 충돌이 있는지 검사 
    for (int i = 0; i < 4; i++) 
    {
        for (int j = 0; j < 4; j++) 
        { //지정된 위치의 게임판과 블럭모양을 비교해서 겹치면 false를 리턴 
            if (BLOCKS[blockType][blockRotation][i][j] == 1 && static_cast<int>(main_org[blockY + i][blockX + j]) > 0)
            {
                return true;
            }
        }
    }
    return false; //하나도 안겹치면 true리턴 
};

void moveBlock(eDir dir) 
{ //블록을 이동시킴 
    eraseBlock();
    switch (dir) 
    {
    case eDir::LEFT: //왼쪽방향 
        setActiveBlock(0, -1);
        blockX--; //좌표값 이동 
        break;

    case eDir::RIGHT:    //오른쪽 방향. 왼쪽방향이랑 같은 원리로 동작 
        setActiveBlock(0, 1);
        blockX++;
        break;

    case eDir::DOWN:    //아래쪽 방향. 왼쪽방향이랑 같은 원리로 동작
        setActiveBlock(1, 0);
        blockY++;
        break;

    case eDir::UP: //키보드 위쪽 눌렀을때 회전시킴. 
        blockRotation = (blockRotation + 1) % 4; //회전값을 1증가시킴(3에서 4가 되는 경우는 0으로 되돌림) 
        setActiveBlock(0, 0);
        break;

    case eDir::ROTATABLE_CRASH: //블록이 바닥, 혹은 다른 블록과 닿은 상태에서 한칸위로 올려 회전이 가능한 경우 
              //이를 동작시키는 특수동작 
        blockRotation = (blockRotation + 1) % 4;
        setActiveBlock(-1, 0);
        blockY--;
        break;
    }
}

void checkLine(void) 
{
    int combo = 0; //콤보갯수 저장하는 변수 지정및 초기화 

    for (int i = MAIN_Y - 2; i > 3;) 
    { //i=MAIN_Y-2 : 밑쪽벽의 윗칸부터,  i>3 : 천장(3)아래까지 검사 
        int lineBlockNum = 0; //한줄의 블록갯수를 저장하는 변수  
        for (int j = 1; j < MAIN_X - 1; j++) 
        { //벽과 벽사이의 블록갯루를 셈 
            if (static_cast<int> (main_org[i][j]) > 0)
            {
                lineBlockNum++;
            }
        }
        if (lineBlockNum == MAIN_X - 2) 
        { //블록이 가득 찬 경우 
            if (bLevelUp == 0) 
            { //레벨업상태가 아닌 경우에(레벨업이 되면 자동 줄삭제가 있음) 
                presentScore += 100 * presentLevel; //점수추가 
                deletedLineCount++; //지운 줄 갯수 카운트 증가 
                combo++; //콤보수 증가  
            }
            for (int k = i; k > 1; k--) 
            { //윗줄을 한칸씩 모두 내림(윗줄이 천장이 아닌 경우에만) 
                for (int l = 1; l < MAIN_X - 1; l++) 
                {
                    if (main_org[k - 1][l] != eBlockStatus::CEILLING)
                    {
                        main_org[k][l] = main_org[k - 1][l];
                    }
                    if (main_org[k - 1][l] == eBlockStatus::CEILLING)
                    {
                        main_org[k][l] = eBlockStatus::EMPTY;
					}
                    //윗줄이 천장인 경우에는 천장을 한칸 내리면 안되니까 빈칸을 넣음 
                }
            }
        }
        else
        {
            i--;
        }
    }

	if (combo > 0)
    { //줄 삭제가 있는 경우 점수와 레벨 목표를 새로 표시함  
        if (combo > 1) 
        { //2콤보이상인 경우 경우 보너스및 메세지를 게임판에 띄웠다가 지움 
            gotoxy(MAIN_X_ADJ + (MAIN_X / 2) - 1, MAIN_Y_ADJ + blockY - 2); 
            printf("%d COMBO!", combo);
            Sleep(500);
            presentScore += (combo * presentLevel * 100);
            initialMainCpy(); //텍스트를 지우기 위해 main_cpy을 초기화.
//(main_cpy와 main_org가 전부 다르므로 다음번 draw()호출시 게임판 전체를 새로 그리게 됨) 
        }

        gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL); printf(" GOAL  : %5d", (deletedLineCount <= 10) ? 10 - deletedLineCount : 0);
        gotoxy(STATUS_X_ADJ, STATUS_Y_SCORE); printf("        %6d", presentScore);
    }
}

void checkLevelUp(void) 
{
    if (deletedLineCount >= 10) 
    { //레벨별로 10줄씩 없애야함. 10줄이상 없앤 경우 
        drawMain();
        bLevelUp = true; //레벨업 flag를 띄움 
        presentLevel += 1; //레벨을 1 올림 
        deletedLineCount = 0; //지운 줄수 초기화   

        for (int i = 0; i < 4; i++) 
        {
            gotoxy(MAIN_X_ADJ + (MAIN_X / 2) - 3, MAIN_Y_ADJ + 4);
            printf("             ");
            gotoxy(MAIN_X_ADJ + (MAIN_X / 2) - 2, MAIN_Y_ADJ + 6);
            printf("             ");
            Sleep(200);

            gotoxy(MAIN_X_ADJ + (MAIN_X / 2) - 3, MAIN_Y_ADJ + 4);
            printf("☆LEVEL UP!☆");
            gotoxy(MAIN_X_ADJ + (MAIN_X / 2) - 2, MAIN_Y_ADJ + 6);
            printf("☆SPEED UP!☆");
            Sleep(200);
        }
        initialMainCpy(); //텍스트를 지우기 위해 main_cpy을 초기화.
//(main_cpy와 main_org가 전부 다르므로 다음번 draw()호출시 게임판 전체를 새로 그리게 됨) 

        for (int i = MAIN_Y - 2; i > MAIN_Y - 2 - (presentLevel - 1); i--) 
        { //레벨업보상으로 각 레벨-1의 수만큼 아랫쪽 줄을 지워줌 
            for (int j = 1; j < MAIN_X - 1; j++) {
                main_org[i][j] = eBlockStatus::INACTIVE_BLOCK; // 줄을 블록으로 모두 채우고 
                gotoxy(MAIN_X_ADJ + j, MAIN_Y_ADJ + i); // 별을 찍어줌.. 이뻐보이게 
                printf("★");
                Sleep(20);
            }
        }
        Sleep(100); //별찍은거 보여주기 위해 delay 
        checkLine(); //블록으로 모두 채운것 지우기
//.checkLine()함수 내부에서 presentLevel up flag가 켜져있는 경우 점수는 없음.         
        switch (presentLevel) 
        { //레벨별로 속도를 조절해줌. 
            case 2:
                blockDownDelay = 50;
                break;
            case 3:
                blockDownDelay = 25;
                break;
            case 4:
                blockDownDelay = 10;
                break;
            case 5:
                blockDownDelay = 5;
                break;
            case 6:
                blockDownDelay = 4;
                break;
            case 7:
                blockDownDelay = 3;
                break;
            case 8:
                blockDownDelay = 2;
                break;
            case 9:
                blockDownDelay = 1;
                break;
            case 10:
                blockDownDelay = 0;
                break;
            default:
                ASSERT(false, "고려하지않은 레벨");
                break;
        }
        bLevelUp = false; //레벨업 flag꺼줌

        gotoxy(STATUS_X_ADJ, STATUS_Y_LEVEL); printf(" LEVEL : %5d", presentLevel); //레벨표시 
        gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL); printf(" GOAL  : %5d", 10 - deletedLineCount); // 레벨목표 표시 

    }
}

void checkGameOver(void) {
    const int x = 5;
    const int y = 5;

    for (int i = 1; i < MAIN_X - 2; i++) 
    {
        if (static_cast<int>(main_org[3][i]) > 0) 
        { //천장(위에서 세번째 줄)에 inactive가 생성되면 게임 오버 
            gotoxy(x, y + 0); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤"); //게임오버 메세지 
            gotoxy(x, y + 1); printf("▤                              ▤");
            gotoxy(x, y + 2); printf("▤  +-----------------------+   ▤");
            gotoxy(x, y + 3); printf("▤  |  G A M E  O V E R..   |   ▤");
            gotoxy(x, y + 4); printf("▤  +-----------------------+   ▤");
            gotoxy(x, y + 5); printf("▤   YOUR SCORE: %6d         ▤", presentScore);
            gotoxy(x, y + 6); printf("▤                              ▤");
            gotoxy(x, y + 7); printf("▤  Press any key to restart..  ▤");
            gotoxy(x, y + 8); printf("▤                              ▤");
            gotoxy(x, y + 9); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
            lastScore = presentScore; //게임점수를 옮김 

            if (presentScore > bestScore) 
            { //최고기록 갱신시 
                FILE* file = fopen("score.dat", "wt"); //presentScore.dat에 점수 저장

                gotoxy(x, y + 6); printf("▤  ★★★ BEST SCORE! ★★★   ▤  ");

                if (file == NULL) 
                { //파일 에러메세지  
                    gotoxy(0, 0);
                    printf("FILE ERROR: SYSTEM CANNOT WRITE BEST SCORE ON \"SCORE.DAT\"");
                }
                else
                {
                    fprintf(file, "%d", presentScore);
                    fclose(file);
                }
            }
            Sleep(1000);
            while (kbhit())
            {
                getch();
            }
            key = getch();
            resetBoard();
        }
    }
}

void pauseGame(void) { //게임 일시정지 함수 
    const int x = 5;
    const int y = 5;

    for (int i = 1; i < MAIN_X - 2; i++) 
    { //게임 일시정지 메세지 
        gotoxy(x, y + 0); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
        gotoxy(x, y + 1); printf("▤                              ▤");
        gotoxy(x, y + 2); printf("▤  +-----------------------+   ▤");
        gotoxy(x, y + 3); printf("▤  |       P A U S E       |   ▤");
        gotoxy(x, y + 4); printf("▤  +-----------------------+   ▤");
		gotoxy(x, y + 5); printf("▤  Press any key to resume..   ▤");
        gotoxy(x, y + 6); printf("▤                              ▤");
        gotoxy(x, y + 7); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
    }
    getch(); //키입력시까지 대기 

    system("cls"); //화면 지우고 새로 그림 
    initialMainCpy();
    drawMain();
    drawMap();

    for (int i = 1; i < 3; i++) 
    { // 다음블록 그림 
        for (int j = 0; j < 4; j++) 
        {
            if (BLOCKS[blockTypeNext][0][i][j] == 1) 
            {
                gotoxy(MAIN_X + MAIN_X_ADJ + 3 + j, i + 6);
                printf("■");
            }
            else 
            {
                gotoxy(MAIN_X + MAIN_X_ADJ + 3 + j, i + 6);
                printf("  ");
            }
        }
    }
} //끝! 

void eraseBlock()
{
    for (int i = 0; i < 4; i++)
    { //현재좌표의 블럭을 지움 
        for (int j = 0; j < 4; j++)
        {
            if (BLOCKS[blockType][blockRotation][i][j] == 1)
            {
                main_org[blockY + i][blockX + j] = eBlockStatus::EMPTY;
            }
        }
    }
}


void setActiveBlock(int X, int Y)
{
    for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                if (BLOCKS[blockType][blockRotation][i][j] == 1)
                {
                    main_org[blockY + i + X][blockX + j + Y] = eBlockStatus::ACTIVE_BLOCK;
                }
            }
        }
}