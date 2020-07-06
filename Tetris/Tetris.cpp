#include"consolCommon.h"

#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<errors.h>
#include<random>

/*
TODO:
1. 전역변수 집어넣기
2. linux테스트
3. class?
핸재 블럭을 하나의 개체로 보자
*/

#define FALLTHROUGH


#define ASSERT(condition, msg) \
    if(!(condition)){ \
        fprintf(stderr, "%s(%s: %d)\n", msg, __FILE__, __LINE__); \
        __asm{int 3} \
    } \
//    ASSERT(false, "error!!!");

const char GAP[] = "  ";

enum class eKeyInput
{
    NON,
    LEFT = 75, //좌로 이동    
    RIGHT = 77, //우로 이동 
    UP = 72, //회전 
    DOWN = 80, //soft drop
    SPACE = 32, //hard drop
    p = 112, //일시정지 
    P = 80, //일시정지
    ESC = 27, //게임종료 
    ROTATABLE_CRASH
    //블록이 바닥, 혹은 다른 블록과 닿은 상태에서 한칸위로 올려 회전이 가능한 경우
};

enum class eBlockStatus
{
    EMPTY,
    ACTIVE_BLOCK, // 게임판배열에 저장될 블록의 상태들 
    CEILLING,
    WALL,
    INACTIVE_BLOCK, // 이동이 완료된 블록값 
    NON_BLOCK  // for copy GAMEBLOARD에는 존재하면안됨
};


struct Info
{
    int presentLevel;
    int presentScore;
    int lastScore;
    int bestScore;
    int scoreNextLevel;
    int deletedLineCount;
};

constexpr int BLOCKS[7][4][4][4] = {
{{0,0,0,0, 0,1,1,0, 0,1,1,0, 0,0,0,0},{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},
 {0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,1,1,0,0,1,1,0,0,0,0,0}}, // 정사각형
{{0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},{0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0},
 {0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0},{0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,0}},// 일자
{{0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,0},
 {0,0,0,0,1,1,0,0,0,1,1,0,0,0,0,0},{0,0,0,0,0,0,1,0,0,1,1,0,0,1,0,0}}, // ㄹ
{{0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0},{0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0},
 {0,0,0,0,0,1,1,0,1,1,0,0,0,0,0,0},{0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0}},
{
    {0,0,0,0,    0,0,1,0,    1,1,1,0,    0,0,0,0},{0,0,0,0,    0,1,0,0,    0,1,0,0,    0,1,1,0},
    {0,0,0,0,    0,0,0,0,    1,1,1,0,    1,0,0,0},{0,0,0,0,    1,1,0,0,    0,1,0,0,    0,1,0,0}},
{
    {0,0,0,0,   1,0,0,0,    1,1,1,0,    0,0,0,0},{0,0,0,0,  0,1,1,0,    0,1,0,0,    0,1,0,0},
    {0,0,0,0,   0,0,0,0,    1,1,1,0,    0,0,1,0},{0,0,0,0,  0,1,0,0,    0,1,0,0,    1,1,0,0}
},
{{0,0,0,0,0,1,0,0,1,1,1,0,0,0,0,0},{0,0,0,0,0,1,0,0,0,1,1,0,0,1,0,0}, // ㅗ
 {0,0,0,0,0,0,0,0,1,1,1,0,0,1,0,0},{0,0,0,0,0,1,0,0,1,1,0,0,0,1,0,0}}
}; //블록모양 저장 4*4공간에 블록을 표현 blcoks[blockType][blockRotation][i][j]로 사용 

constexpr int GAME_Board_X_SIZE = 11; //게임판 가로크기 
constexpr int GAME_Board_Y_SIZE = 23; //게임판 세로크기 

constexpr int MAIN_X_ADJ = 3;//게임판 위치조정 
constexpr int MAIN_Y_ADJ = 1;//게임판 위치조정 
constexpr int STATUS_X_ADJ = MAIN_X_ADJ + GAME_Board_X_SIZE + 1;//게임정보표시 위치조정 

constexpr int STATUS_Y = 3;
constexpr int STATUS_Y_GOAL = STATUS_Y; //GOAL 정보표시위치 Y 좌표 저장 
constexpr int STATUS_Y_LEVEL = STATUS_Y + 1; //LEVEL 정보표시위치 Y 좌표 저장
constexpr int STATUS_Y_SCORE = STATUS_Y + 9; //SCORE 정보표시위치 Y 좌표 저장


//todo 전역변수 모두 지역변수로 넣기
int blockType; //블록 종류를 저장 
int blockRotation; //블록 회전값 저장 
int blockTypeNext; //다음 블록값 저장 

//게임판의 정보를 저장하는 배열    모니터에 표시후에 gameBoardCpy로 복사됨 
//gameBoardCpy는 게임판이 모니터에 표시되기 전의 정보를 가지고 있음 
//전체를 계속 모니터에 표시하지 않고(이렇게 하면 모니터가 깜빡거림) 
//gameBoardCpy와 배열을 비교해서 값이 달라진 곳만 모니터에 고침 
// 배열 특성상 X,Y순서가 뒤집어짐
eBlockStatus gameBoard[GAME_Board_Y_SIZE][GAME_Board_X_SIZE];
eBlockStatus gameBoardCpy[GAME_Board_Y_SIZE][GAME_Board_X_SIZE];

int blockX;
int blockY; //이동중인 블록의 게임판상의 x,y좌표를 저장 

int blockDownDelay; // 블록 내려오는 딜레이

bool bNeedNewBlock = false; //새로운 블럭이 필요함을 알리는 flag 
bool bBlockFloorCrash = false; //현재 블록이 바닥에 닿았는지 체크
bool bLevelUp = false; //다음레벨로 진행(현재 레벨목표가 완료되었음을) 알리는 flag 

void drawTitle(void); //게임시작화면 
void drawGameBoard(void); //게임판을 그림 
void drawInfoBoard(const Info& info); //게임 전체 인터페이스를 표시 
void drawUpdateInfoBoard(const Info& info);

void initialBoard(Info* info); //게임판 초기화 
void initialMainOrg(void); //메인 게임판(main_org[][]를 초기화)
void initialMainCpy(void); //copy 게임판(main_cpy[][]를 초기화)
void makeNewBlock(void); //새로운 블록을 하나 만듦 
eKeyInput inputKeyMoveBlock(Info* info); //키보드로 키를 입력받음 
void dropBlock(Info* info); //블록을 아래로 떨어트림 
bool isCrash(int blockX, int blockY, int rotation); //blockX, by위치에 rotation회전값을 같는 경우 충돌 판단 
void moveBlock(eKeyInput dir); //dir방향으로 블록을 움직임 
void checkFullLine(Info* info); //줄이 가득찼는지를 판단하고 지움 
void checkLevelUp(Info* info); //레벨목표가 달성되었는지를 판단하고 levelup시킴 
void checkGameOver(Info* info); //게임오버인지 판단하고 게임오버를 진행 
void pauseGame(Info* info);//게임을 일시정지시킴 
void setEraseBlock();
void clearBuffer();

void setActiveBlock(int X, int Y);
int getRandom(const int min, const int max);

int main()
{
    static_assert(static_cast<int>(eBlockStatus::EMPTY) == 0, "EMPTY is not 0");
    static_assert(sizeof(gameBoard) == 23 * 11 * sizeof(eBlockStatus), "error");
    Info info;
    consolCommon::setCursorType(eCursorType::NO_CURSOR);
    drawTitle(); 
    
    //시작시, 사망후 재시작시 무조건 실행하는 네줄
    initialBoard(&info); //게임판 리셋 
    drawInfoBoard(info); // 정보화면을 그림
    makeNewBlock(); //새로운 블록을 하나 만듦  
    drawUpdateInfoBoard(info);

    while (1)
    {
        for (int i = 0; i < 5; i++)
        {//블록이 한칸떨어지는동안 5번 키입력받을 수 있음 
            eKeyInput keyInput = inputKeyMoveBlock(&info);
            drawGameBoard();
            if (keyInput == eKeyInput::SPACE)
            { //스페이스바를 누른경우(hard drop) 추가로 이동및 회전할수 없음 break; 
                break;
            }
            
            consolCommon::__sleep(blockDownDelay);
            //블록이 충돌중인경우 추가로 이동및 회전할 시간을 갖음 
            if (bBlockFloorCrash && isCrash(blockX, blockY + 1, blockRotation))
            {
                consolCommon::__sleep(100);
            }
        }

        dropBlock(&info); // 블록을 한칸 내림 
        checkLevelUp(&info); // 레벨업을 체크 
        checkGameOver(&info); //게임오버를 체크 

        if (bNeedNewBlock)
        {
            makeNewBlock(); // 뉴 블럭 flag가 있는 경우 새로운 블럭 생성 
            drawUpdateInfoBoard(info);
        }
    }
}

void drawTitle(void) {
    const int x = 5; //타이틀화면이 표시되는 x좌표 
    const int y = 4; //타이틀화면이 표시되는 y좌표 

    consolCommon::gotoxy(x, y + 0); printf("■□□□■■■□□■■□□■■"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 1); printf("■■■□  ■□□    ■■□□■"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 2); printf("□□□■              □■  ■"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 3); printf("■■□■■  □  ■  □□■□□"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 4); printf("■■  ■□□□■■■□■■□□"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 5); printf("      blog.naver.com/azure0777"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x + 5, y + 2); printf("T E T R I S"); consolCommon::__sleep(100);
    consolCommon::gotoxy(x, y + 7); printf("Please Enter Any Key to Start..");
    consolCommon::gotoxy(x, y + 9); printf("  △   : Shift");
    consolCommon::gotoxy(x, y + 10); printf("◁  ▷ : Left / Right");
    consolCommon::gotoxy(x, y + 11); printf("  ▽   : Soft Drop");
    consolCommon::gotoxy(x, y + 12); printf(" SPACE : Hard Drop");
    consolCommon::gotoxy(x, y + 13); printf("   P   : Pause");
    consolCommon::gotoxy(x, y + 14); printf("  ESC  : Quit");
    consolCommon::gotoxy(x, y + 16); printf("BONUS FOR HARD DROPS / COMBOS");

    //타이틀 프레임을 세는 변수  
    for (int i = 0; 1; i++)
    {   //하나도 안중요한 별 반짝이는 애니메이션효과 
        if (consolCommon::__kbhit())
        {
            break;
        }
        else if (i % 200 == 0)
        {
            consolCommon::gotoxy(x + 4, y + 1);
            printf("★");
        }//cnt가 200으로 나누어 떨어질때 별을 표시 
        else if ((i % 200 - 100) == 0)
        {
            consolCommon::gotoxy(x + 4, y + 1);
            printf(GAP);
        } //위 카운트에서 100카운트 간격으로 별을 지움 
        else if ((i % 350) == 0)
        {
            consolCommon::gotoxy(x + 13, y + 2);
            printf("☆");
        } //윗별과 같지만 시간차를 뒀음 
        else if ((i % 350 - 100) == 0)
        {
            consolCommon::gotoxy(x + 13, y + 2);
            printf(GAP);
        }
        consolCommon::__sleep(10); // 00.1초 딜레이  
    }

}

void initialBoard(Info* info) {

    FILE* file = fopen("score.dat", "rt"); // Score.dat파일을 연결 
    if (file == NULL)
    {
        info->bestScore = 0;
    }
    else
    {
        fscanf(file, "%d", &(info->bestScore)); // 파일이 열리면 최고점수를 불러옴 
        fclose(file); //파일 닫음 
    }

    info->lastScore = 0;
    info->presentLevel = 1; //각종변수 초기화 
    info->presentScore = 0;
    info->scoreNextLevel = 1000;
    info->deletedLineCount = 0;

    blockDownDelay = 100;
    bBlockFloorCrash = false;
    blockTypeNext = getRandom(0, 6); //다음번에 나올 블록 종류를 랜덤하게 생성 

    consolCommon::clear(); //화면지움 
    initialMainOrg(); // main_org를 초기화 
    initialMainCpy();

}

void initialMainOrg(void)
{
    memset(gameBoard, 0, sizeof(gameBoard));
    for (int j = 1; j < GAME_Board_X_SIZE; j++)
    { //y값이 3인 위치에 천장을 만듦 
        gameBoard[3][j] = eBlockStatus::CEILLING;
    }
    for (int i = 1; i < GAME_Board_Y_SIZE - 1; i++)
    { //좌우 벽을 만듦  
        gameBoard[i][0] = eBlockStatus::WALL;
        gameBoard[i][GAME_Board_X_SIZE - 1] = eBlockStatus::WALL;
    }
    for (int j = 0; j < GAME_Board_X_SIZE; j++)
    { //바닥벽을 만듦 
        gameBoard[GAME_Board_Y_SIZE - 1][j] = eBlockStatus::WALL;
    }
    clearBuffer();
}




//main_org와 같은 숫자가 없게 하기 위해  게임판에 게임에 사용되지 않는 숫자를 넣음 
void initialMainCpy(void) {
    for (int i = 0; i < GAME_Board_Y_SIZE; i++)
    {
        for (int j = 0; j < GAME_Board_X_SIZE; j++)
        {
            gameBoardCpy[i][j] = eBlockStatus::NON_BLOCK;
        }
    }
}

void drawInfoBoard(const Info &info)
{ //게임 상태 표시를 나타내는 함수  
// presentLevel, goal, score만 게임중에 값이 바뀔수 도 있음 그 y값을 따로 저장해둠 
 // 그래서 혹시 게임 상태 표시 위치가 바뀌어도 그 함수에서 안바꿔도 되게.. 
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_LEVEL); printf(" LEVEL : %5d", info.presentLevel);
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL); printf(" GOAL  : %5d", 10 - info.deletedLineCount);
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 2); printf("+-  N E X T  -+ ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 3); printf("|             | ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 4); printf("|             | ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 5); printf("|             | ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 6); printf("|             | ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 7); printf("+-- -  -  - --+ ");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 8); printf(" YOUR SCORE :");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_SCORE); printf("        %6d", info.presentScore);
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 10); printf(" LAST SCORE :");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 11); printf("        %6d", info.lastScore);
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 12); printf(" BEST SCORE :");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 13); printf("        %6d", info.bestScore);
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 15); printf("  △   : Shift        SPACE : Hard Drop");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 16); printf("◁  ▷ : Left / Right   P   : Pause");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 17); printf("  ▽   : Soft Drop     ESC  : Quit");
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y + 20); printf("blog.naver.com/azure0777");
}

void drawGameBoard(void)
{ //게임판 그리는 함수 
    for (int j = 1; j < GAME_Board_X_SIZE - 1; j++)
    { //천장은 계속 새로운블럭이 지나가서 지워지면 새로 그려줌 
        if (gameBoard[3][j] == eBlockStatus::EMPTY)
        {
            gameBoard[3][j] = eBlockStatus::CEILLING;
        }
    }
    for (int i = 0; i < GAME_Board_Y_SIZE; i++)
    {
        for (int j = 0; j < GAME_Board_X_SIZE; j++)
        {
            if (gameBoardCpy[i][j] != gameBoard[i][j])
            { //cpy랑 비교해서 값이 달라진 부분만 새로 그려줌.
             //이게 없으면 게임판전체를 계속 그려서 느려지고 반짝거림
                consolCommon::gotoxy(MAIN_X_ADJ + j, MAIN_Y_ADJ + i);
                switch (gameBoard[i][j])
                {
                case eBlockStatus::EMPTY: //빈칸모양 
                    printf(GAP);
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
    memcpy(gameBoardCpy, gameBoard, sizeof(gameBoard));
}

void makeNewBlock(void)
{
    blockX = (GAME_Board_X_SIZE / 2) - 1; //블록 생성 위치x좌표(게임판의 가운데) 
    blockY = 0;  //블록 생성위치 y좌표(제일 위) 
    blockType = blockTypeNext; //다음블럭값을 가져옴 
    blockTypeNext = getRandom(0, 6);
    blockRotation = 0;  //회전은 0번으로 가져옴 

    bNeedNewBlock = false; //makeNewBlock flag를 끔  

    //게임판 blockX, blocky위치에 블럭생성  
    setActiveBlock(0, 0);
}

void drawUpdateInfoBoard(const Info& info)
{
    consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_SCORE); printf("        %6d", info.presentScore); //점수 표시  
    //게임상태표시에 다음에 나올블럭을 그림 
    for (int i = 1; i < 3; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (BLOCKS[blockTypeNext][0][i][j] == 1)
            {
                consolCommon::gotoxy(STATUS_X_ADJ + 2 + j, i + 6);
                printf("■");
            }
            else
            {
                consolCommon::gotoxy(STATUS_X_ADJ + 2 + j, i + 6);
                printf(GAP);
            }
        }
    }
}

eKeyInput inputKeyMoveBlock(Info* info)
{
    int key = 0;
    eKeyInput inputKey = eKeyInput::NON;

    if (consolCommon::__kbhit())
    {
        key = consolCommon::__getch(); //키값을 받음
        inputKey = static_cast<eKeyInput>(key);
        //방향키인경우 
        if (key == 224)
        {
            key = consolCommon::__getch();
            inputKey = static_cast<eKeyInput>(key);
            switch (inputKey)
            {
            case eKeyInput::LEFT:
                if (!isCrash(blockX - 1, blockY, blockRotation))
                {
                    moveBlock(inputKey);
                }
                break;
            case eKeyInput::RIGHT:
                if (!isCrash(blockX + 1, blockY, blockRotation))
                {
                    moveBlock(inputKey);
                }
                break;
            case eKeyInput::DOWN:
                if (!isCrash(blockX, blockY + 1, blockRotation))
                {
                    moveBlock(inputKey);
                }
                break;
            case eKeyInput::UP:
                if (!isCrash(blockX, blockY, (blockRotation + 1) % 4))
                {
                    moveBlock(inputKey);
                }
                //회전할 수 있는지 체크 후 가능하면 회전
                else if (bBlockFloorCrash && !isCrash(blockX, blockY - 1, (blockRotation + 1) % 4))
                {
                    //바닥에 닿은 경우 위쪽으로 한칸띄워서 회전이 가능하면 그렇게 함(특수동작)
                    moveBlock(eKeyInput::ROTATABLE_CRASH);
                }
            default:
                break;
            }
        }
        else
        { //방향키가 아닌경우 
            switch (inputKey)
            {
            case eKeyInput::SPACE: //스페이스키 눌렀을때 

                while (!bBlockFloorCrash)
                { //바닥에 닿을때까지 이동시킴 
                    dropBlock(info);
                    info->presentScore += info->presentLevel; // hard drop 보너스

                }
                break;
            case eKeyInput::P: //P(대문자) 눌렀을때 
//#ifdef _HAS_CXX17
//                //only msvc c++17
//                [[fallthrough]];
//#endif
                FALLTHROUGH
            case eKeyInput::p: //p(소문자) 눌렀을때 
                pauseGame(info); //일시정지 
                break;
            case eKeyInput::ESC: //ESC눌렀을때 
                consolCommon::clear(); //화면을 지우고 
                exit(0); //게임종료 
            default:
                break;
            }
        }
    }
    clearBuffer();
    return inputKey;
}

void dropBlock(Info* info)
{
    bool bIsCrash = isCrash(blockX, blockY + 1, blockRotation);
    if (bBlockFloorCrash && bIsCrash) // 11
    { //밑이 비어있지않고 crush flag가 켜저있으면 
        for (int i = 0; i < GAME_Board_Y_SIZE; i++)
        { //현재 조작중인 블럭을 굳힘 
            for (int j = 0; j < GAME_Board_X_SIZE; j++)
            {
                if (gameBoard[i][j] == eBlockStatus::ACTIVE_BLOCK)
                {
                    gameBoard[i][j] = eBlockStatus::INACTIVE_BLOCK;
                }
            }
        }
        bBlockFloorCrash = false;
        checkFullLine(info);
        bNeedNewBlock = true;
        return;
    }
    if (!bIsCrash) //  0 1/0
    {
        moveBlock(eKeyInput::DOWN); //밑이 비어있으면 밑으로 한칸 이동 
    }
    else // 0 1/0
    {
        bBlockFloorCrash = true; //밑으로 이동이 안되면  crush flag를 켬
    }
}

bool isCrash(int blockX, int blockY, int blockRotation)
{   //지정된 좌표와 회전값으로 충돌이 있는지 검사 
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        { //지정된 위치의 게임판과 블럭모양을 비교해서 겹치면 false를 리턴 
            if (BLOCKS[blockType][blockRotation][i][j] == 1 &&
                (gameBoard[blockY + i][blockX + j] == eBlockStatus::INACTIVE_BLOCK
                    || gameBoard[blockY + i][blockX + j] == eBlockStatus::WALL))
            {
                return true;
            }
        }
    }
    return false; //하나도 안겹치면 true리턴 
}

void moveBlock(eKeyInput key)
{
    setEraseBlock();
    switch (key)
    {
    case eKeyInput::LEFT:
        setActiveBlock(0, -1);
        blockX--; //좌표값 이동 
        break;
    case eKeyInput::RIGHT:
        setActiveBlock(0, 1);
        blockX++;
        break;
    case eKeyInput::DOWN:
        setActiveBlock(1, 0);
        blockY++;
        break;
    case eKeyInput::UP: //키보드 위쪽 눌렀을때 회전시킴. 
        blockRotation = (blockRotation + 1) % 4; //회전값을 1증가시킴(3에서 4가 되는 경우는 0으로 되돌림) 
        setActiveBlock(0, 0);
        break;
    case eKeyInput::ROTATABLE_CRASH: //블록이 바닥, 혹은 다른 블록과 닿은 상태에서 한칸위로 올려 회전이 가능한 경우 
        blockRotation = (blockRotation + 1) % 4;
        setActiveBlock(-1, 0);
        blockY--;
        break;
    }
}

void checkFullLine(Info* info)
{
    int fullLineCount = 0;
    for (int i = GAME_Board_Y_SIZE - 2; i > 3; )
    { //i=GAME_Board_Y_SIZE-2 : 밑쪽벽의 윗칸부터,  i>3 : 천장(3)아래까지 검사 
        int lineBlockNum = 0; //한줄의 블록갯수를 저장하는 변수  
        for (int j = 1; j < GAME_Board_X_SIZE - 1; j++)
        {   //벽과 벽사이의 블록갯루를 셈 
            if (gameBoard[i][j] == eBlockStatus::INACTIVE_BLOCK
                || gameBoard[i][j] == eBlockStatus::WALL)
            {
                lineBlockNum++;
            }
        }

        if (lineBlockNum == GAME_Board_X_SIZE - 2)
        { //블록이 가득 찬 경우 
            if (!bLevelUp)
            { //레벨업상태가 아닌 경우에(레벨업이 되면 자동 줄삭제가 있음) 
                info->presentScore += 100 * info->presentLevel; //점수추가 
                info->deletedLineCount++; //지운 줄 갯수 카운트 증가 
                fullLineCount++; //콤보수 증가  
            }

            for (int k = i; k > 1; k--)
            { //윗줄을 한칸씩 모두 내림(윗줄이 천장이 아닌 경우에만) 
                for (int l = 1; l < GAME_Board_X_SIZE - 1; l++)
                {
                    //윗줄이 천장인 경우에는 천장을 한칸 내리면 안되니까 빈칸을 넣음 
                    if (gameBoard[k - 1][l] == eBlockStatus::CEILLING)
                    {
                        gameBoard[k][l] = eBlockStatus::EMPTY;
                    }
                    else
                    {
                        gameBoard[k][l] = gameBoard[k - 1][l];
                    }

                }
            }
        }
        else
        {
            i--;
        }
    }

    if (fullLineCount > 0)
    { //줄 삭제가 있는 경우 점수와 레벨 목표를 새로 표시함  
        if (fullLineCount > 1)
        { //2콤보이상인 경우 경우 보너스및 메세지를 게임판에 띄웠다가 지움 
            consolCommon::gotoxy(MAIN_X_ADJ + (GAME_Board_X_SIZE / 2) - 1, MAIN_Y_ADJ + blockY - 2);printf("%d COMBO!", fullLineCount);
            consolCommon::__sleep(500);
            info->presentScore += (fullLineCount * info->presentLevel * 100);
            initialMainCpy(); 
            //텍스트를 지우기 위해 main_cpy을 초기화(main_cpy와 main_org가 전부 다르므로 다음번 draw()호출시 게임판 전체를 새로 그리게 됨) 
        }
        consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL); printf(" GOAL  : %5d", (info->deletedLineCount <= 10) ? 10 - info->deletedLineCount : 0);
    }
}

void checkLevelUp(Info* info)
{
    if (info->deletedLineCount >= 10)
    { //레벨별로 10줄씩 없애야함 10줄이상 없앤 경우 
        drawGameBoard();
        bLevelUp = true; //레벨업 flag를 띄움 
        info->presentLevel += 1; //레벨을 1 올림 
        info->deletedLineCount = 0; //지운 줄수 초기화   

        for (int i = 0; i < 4; i++)
        {
            consolCommon::gotoxy(MAIN_X_ADJ + (GAME_Board_X_SIZE / 2) - 3, MAIN_Y_ADJ + 4);
            printf("             ");
            consolCommon::gotoxy(MAIN_X_ADJ + (GAME_Board_X_SIZE / 2) - 2, MAIN_Y_ADJ + 6);
            printf("             ");
            consolCommon::__sleep(200);

            consolCommon::gotoxy(MAIN_X_ADJ + (GAME_Board_X_SIZE / 2) - 3, MAIN_Y_ADJ + 4);
            printf("☆LEVEL UP!☆");
            consolCommon::gotoxy(MAIN_X_ADJ + (GAME_Board_X_SIZE / 2) - 2, MAIN_Y_ADJ + 6);
            printf("☆SPEED UP!☆");
            consolCommon::__sleep(200);
        }
        initialMainCpy(); //텍스트를 지우기 위해 main_cpy을 초기화.
        //(main_cpy와 main_org가 전부 다르므로 다음번 draw()호출시 게임판 전체를 새로 그리게 됨) 

        for (int i = GAME_Board_Y_SIZE - 2; i > GAME_Board_Y_SIZE - 2 - (info->presentLevel - 1); i--)
        { //레벨업보상으로 각 레벨-1의 수만큼 아랫쪽 줄을 지워줌 
            for (int j = 1; j < GAME_Board_X_SIZE - 1; j++) {
                gameBoard[i][j] = eBlockStatus::INACTIVE_BLOCK; // 줄을 블록으로 모두 채우고 
                consolCommon::gotoxy(MAIN_X_ADJ + j, MAIN_Y_ADJ + i); // 별을 찍어줌.. 이뻐보이게 
                printf("★");
                consolCommon::__sleep(20);
            }
        }
        consolCommon::__sleep(100); //별찍은거 보여주기 위해 delay 
        checkFullLine(info); //블록으로 모두 채운것 지우기
        //.checkFullLine()함수 내부에서 presentLevel up flag가 켜져있는 경우 점수는 없음.         
        switch (info->presentLevel)
        {
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
            //ASSERT(false, "고려하지않은 레벨");
            // CASE 10을 유지
            break;
        }

        bLevelUp = false;

        consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_LEVEL); printf(" LEVEL : %5d", info->presentLevel);
        consolCommon::gotoxy(STATUS_X_ADJ, STATUS_Y_GOAL); printf(" GOAL  : %5d", 10 - info->deletedLineCount);
    }
}

void checkGameOver(Info* info)
{
    const int x = 5;
    const int y = 5;

    for (int i = 1; i < GAME_Board_X_SIZE - 2; i++)
    {
        ASSERT(gameBoard[3][i] != eBlockStatus::NON_BLOCK, "non_block");

        //gameover 검사 기준
        if (gameBoard[3][i] == eBlockStatus::INACTIVE_BLOCK
            || gameBoard[3][i] == eBlockStatus::WALL)
        { //천장(위에서 세번째 줄)에 inactive가 생성되면 게임 오버 
            consolCommon::gotoxy(x, y + 0); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
            consolCommon::gotoxy(x, y + 1); printf("▤                              ▤");
            consolCommon::gotoxy(x, y + 2); printf("▤  +-----------------------+   ▤");
            consolCommon::gotoxy(x, y + 3); printf("▤  |  G A M E  O V E R..   |   ▤");
            consolCommon::gotoxy(x, y + 4); printf("▤  +-----------------------+   ▤");
            consolCommon::gotoxy(x, y + 5); printf("▤   YOUR SCORE: %6d         ▤", info->presentScore);
            consolCommon::gotoxy(x, y + 6); printf("▤                              ▤");
            consolCommon::gotoxy(x, y + 7); printf("▤  Press any key to restart..  ▤");
            consolCommon::gotoxy(x, y + 8); printf("▤                              ▤");
            consolCommon::gotoxy(x, y + 9); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
            info->lastScore = info->presentScore;



            if (info->presentScore > info->bestScore)
            { //최고기록 갱신시 
                FILE* file = fopen("score.dat", "wt");

                consolCommon::gotoxy(x, y + 6); printf("▤  ★★★ BEST SCORE! ★★★   ▤  ");

                if (file == NULL)
                { //파일 에러메세지  
                    consolCommon::gotoxy(0, 0);
                    printf("FILE ERROR: SYSTEM CANNOT WRITE BEST SCORE ON \"SCORE.DAT\"");
                }
                else
                {
                    fprintf(file, "%d", info->presentScore);
                    fclose(file);
                }
            }
            consolCommon::__sleep(1000);
            consolCommon::__getch();

            clearBuffer();
            
            initialBoard(info);
            drawInfoBoard(*info);
            drawGameBoard();
        }
    }
}

void pauseGame(Info* info)
{
    const int x = 5;
    const int y = 5;

    for (int i = 1; i < GAME_Board_X_SIZE - 2; i++)
    {
        consolCommon::gotoxy(x, y + 0); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
        consolCommon::gotoxy(x, y + 1); printf("▤                              ▤");
        consolCommon::gotoxy(x, y + 2); printf("▤  +-----------------------+   ▤");
        consolCommon::gotoxy(x, y + 3); printf("▤  |       P A U S E       |   ▤");
        consolCommon::gotoxy(x, y + 4); printf("▤  +-----------------------+   ▤");
        consolCommon::gotoxy(x, y + 5); printf("▤  Press any key to resume..   ▤");
        consolCommon::gotoxy(x, y + 6); printf("▤                              ▤");
        consolCommon::gotoxy(x, y + 7); printf("▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤▤");
    }

    consolCommon::__getch(); //키입력시까지 대기 

    consolCommon::clear(); //화면 지우고 새로 그림 

    initialMainCpy();
    drawGameBoard();
    drawInfoBoard(*info);
    drawUpdateInfoBoard(*info);
}


int getRandom(const int min, const int max)
{
    static std::random_device      rn;
    static int                     seed = rn();
    static std::mt19937            rnd(seed);

    std::uniform_int_distribution<int> range(min, max);

    return range(rnd);
}

//현재좌표의 블럭을 지움 
void setEraseBlock()
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (BLOCKS[blockType][blockRotation][i][j] == 1)
            {
                gameBoard[blockY + i][blockX + j] = eBlockStatus::EMPTY;
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
                gameBoard[blockY + i + X][blockX + j + Y] = eBlockStatus::ACTIVE_BLOCK;
            }
        }
    }
}


//getch의 버퍼들 모두 비움.
void clearBuffer()
{
    while (consolCommon::__kbhit())
    {
        consolCommon::__getch();
    }
}
