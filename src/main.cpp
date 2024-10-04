#include <iostream>
#include <vector>
#include "rclcpp/rclcpp.hpp"

class Player // 플레이어의 정보를 담고 있는 클래스
{
public:
  int player_num;   // 플레이어 번호
  int pieces_count; // 말의 개수
  int pre_yut_info; // 직전 윷 인포 (현재)
  int board[30];   // 보드 판
  int* pieces_location;//말의 위치
  int finish_pieces; //0부터 시작  이녀석과 말의 개수가 같아지면 게임이 끝남

  // 플레이어 보드의 사항
  //
  Player() : player_num(0), pieces_count(4)
  {
    //pieces reset
    pieces_location = new int[pieces_count];
    for(int i = 0; i<pieces_count;i++) pieces_location[i] = 0;
  }
  Player(int num) : player_num(num), pieces_count(4) {} // 기본 말 개수 4
  ~Player(){delete pieces_location;}


  //
  bool isend(){bool result = false; if(finish_pieces == pieces_count)result = true; return result;};




};

class Master : public rclcpp::Node
{
public:
  Player robot;   // my
  Player *player; // other
  int turn_count; // 0으로 리셋(0이 첫번째)
  int player_num; // 몇명의 플레이어가 있는지
  int current_yut;// 윷 상태// 도 :1,개: 2,걸 : 3, 윷 : 4, 모 : 5, 빽도 : -1


  Master() : Node("yutnori_master"), robot(0), player_num(2)
  {
    player = new Player[player_num];
    for (int i = 0; i < player_num; i++)
      player[i].player_num = i + 1;
  } // 노드 이름을 명시적으로 설정
  ~Master() {delete player;}

  // 추가적인 게임 로직이나 멤버 함수가 여기에 추가될 수 있음

  void starGame();  // 게임 셋팅을 위한 리셋
  void playGame();  // 전체적인 게임 플레이
  void gameReset(); // 게임 재시도 할 때 리셋해주는 거
  bool endGame();   // 게임 끝남 // 말이 다 들어온 플레이어가 있거나, 걍 강제종료 했을 때.

  bool isMyTurn(const Player &player); // "player" 턴인지 확인

  void ThrowYut();    // 윷 던지기
  void yutAnalysis(); // 윷 상태 분석

  // 움직임
  void arminit();                                             // 로봇 팔 제자리로 돌아오도록
  void playMotion();                                          // 잡거나 던지거나 등등의 모션 움직임을 위판 함수
  void targetCoor(double x = 0, double y= 0 , double z= 0, double seta = 0); // 그리퍼의 움직임 좌표
};

bool Master::isMyTurn(const Player &player)
{
  bool is = false;
  if (player.player_num == this->turn_count % this->player_num)
    is = true;

  return is;
}


bool Master::endGame()
{
  if(robot.isend()) return true;//myrobot
  for(int i = 0;i<player_num;i++)if(player[i].isend()) return true;//others

  return false;
}


void Master::playGame()
{

  while (endGame())
  {

    if (isMyTurn(robot)) // 내 턴인 경우
    {

      //윷 정리 //윷 위치 선택


      // 윷을 잡기
      targetCoor();
      playMotion();

      // 윷을 던지기
      playMotion();
      // 윷의 상태 확인

      yutAnalysis();
      // 말판의 상태 확인

      // 최적의 움직임 계산

      // 로봇 말 이동

      // 보드 상태 업데이트
      arminit();



    }
    else
    {




    }
  }
}

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  // Master 노드 인스턴스 생성 및 실행
  auto master_node = std::make_shared<Master>();
  rclcpp::spin(master_node);

  rclcpp::shutdown();
  return 0;
}
