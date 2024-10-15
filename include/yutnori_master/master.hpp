#include <iostream>
#include <vector>
#include <map>
#include "rclcpp/rclcpp.hpp"




#include "xyz_interfaces/srv/yutnori_board_state.hpp"
#include "xyz_interfaces/srv/yutnori_yut_state.hpp"
#include "xyz_interfaces/srv/pick_place.hpp"
#include "xyz_interfaces/srv/yut_pick.hpp"
#include "xyz_interfaces/srv/yut_throw.hpp"
#include "xyz_interfaces/srv/yut_pieces_poision.hpp" //

using YutState = xyz_interfaces::srv::YutnoriYutState;
using YutBoardState = xyz_interfaces::srv::YutnoriBoardState;

class Player // 플레이어의 정보를 담고 있는 클래스
{
public:
  int player_num;                   // 플레이어 번호
  int pieces_count;                 // 말의 개수
  std::array<int,4> pieces_location; // 말의 위치
  int finish_pieces;                // 0부터 시작 이녀석과 말의 개수가 같아지면 게임이 끝남

  const int short_cut_index[4] = {5, 10, 33, 48}; // 지름길 인덱스

  const float bc[4] = {0,0,0,0};
  const std::map<int,std::tuple<float,float,float,float>> board_coor =
  {
        {0, {1., 2., 0, 0}},{1, {1., 2., 0, 0}},{2, {1., 2., 0, 0}},{3, {1., 2., 0, 0}},{4, {1., 2., 0, 0}},
        {5, {1., 2., 0, 0}},{6, {1., 2., 0, 0}},{7, {1., 2., 0, 0}},{8, {1., 2., 0, 0}},{9, {1., 2., 0, 0}},
        {10, {1., 2., 0, 0}},{11, {1., 2., 0, 0}},{12, {1., 2., 0, 0}},{13, {1., 2., 0, 0}},{14, {1., 2., 0, 0}},
        {15, {1., 2., 0, 0}},{16, {1., 2., 0, 0}},{17, {1., 2., 0, 0}},{18, {1., 2., 0, 0}},{19, {1., 2., 0, 0}},{20, {1., 2., 0, 0}},

        {30, {1., 2., 0, 0}},{31, {1., 2., 0, 0}},{32, {1., 2., 0, 0}},{33, {1., 2., 0, 0}},{34, {1., 2., 0, 0}},
        {35, {1., 2., 0, 0}},{36, {1., 2., 0, 0}},{37, {1., 2., 0, 0}},{38, {1., 2., 0, 0}},{39, {1., 2., 0, 0}},
        {40, {1., 2., 0, 0}},{41, {1., 2., 0, 0}},//첫번째 지름길 대각선 지름길

        {45, {1., 2., 0, 0}},{46, {1., 2., 0, 0}},{47, {1., 2., 0, 0}},{48, {1., 2., 0, 0}},{49, {1., 2., 0, 0}},
        {50, {1., 2., 0, 0}},{51, {1., 2., 0, 0}}//두번째 지름길 대각선 지름길
  };

  // 플레이어 보드의 사항
  //
  Player() : player_num(0), pieces_count(4), finish_pieces(4){}
  ~Player() {}

  //
  bool isend()
  {
    bool result = false;
    if (finish_pieces == pieces_count)
      result = true;
    return result;
  };
};

class Master : public rclcpp::Node
{
public:
  Player robot;    // my
  Player player;  // other
  int turn_count;  // 0으로 리셋(0이 첫번째)
  int player_num;  // 몇명의 플레이어가 있는지
  int current_yut; // 윷 상태// 도 :1,개: 2,걸 : 3, 윷 : 4, 모 : 5, 빽도 : -1

  Master() : Node("yutnori_master"), player_num(2)
  {
    // player = new Player[player_num];
    // for (int i = 0; i < player_num; i++)
    //   player[i].player_num = i + 1;

    // 서비스 클라이언트 생성
    client_yut      = this->create_client<xyz_interfaces::srv::YutnoriYutState>("see_yutnori_state");
    client_position = this->create_client<xyz_interfaces::srv::PickPlace>("pick_place");
    client_yut_pos  = this->create_client<xyz_interfaces::srv::YutPiecesPoision>("yut_pieces_position");
    client_board = this->create_client<xyz_interfaces::srv::YutnoriBoardState>("pieces_location");

    RCLCPP_INFO(this->get_logger(), "Wait...44");
    playGame();
    // 서비스가 준비될 때까지 대기
    // while (!client_yut->wait_for_service(std::chrono::seconds(1)))
    // {
    //   RCLCPP_INFO(this->get_logger(), "Waiting for service1 to appear...");
    // }

    // // picplace

    // // 서비스가 준비될 때까지 대기
    // while (!client_position->wait_for_service(std::chrono::seconds(1)))
    // {
    //   RCLCPP_INFO(this->get_logger(), "Waiting for service2 to appear...");
    // }

  } // 노드 이름을 명시적으로 설정
  ~Master() { /*delete player;*/ }

  // 추가적인 게임 로직이나 멤버 함수가 여기에 추가될 수 있음

  void starGame();  // 게임 셋팅을 위한 리셋
  void playGame();  // 전체적인 게임 플레이
  void gameReset(); // 게임 재시도 할 때 리셋해주는 거
  bool endGame();   // 게임 끝남 // 말이 다 들어온 플레이어가 있거나, 걍 강제종료 했을 때.

  bool isMyTurn(const Player &player); // "player" 턴인지 확인

  void throwYut();                      // 윷 던지기
  int yutAnalysis();                    // 윷 상태 분석
  int boardAnalysis(int current_state, int & next_index); // 리턴 값 옮길 말 인덱스, 어떤 움직임인지(상대 말을 치워야 하거나 쌓아야 하거나 하는 경우), 어디로 이동되는지 인덱스

  void cleanUpBoard();
  void movePieces(int piece_index, int next_index);
  void boardUpDate();
  void seeOtherBoard();


  // 움직임
  void pickAndPlace(float x = 0, float y = 0, float z = 0, float rotate = 0, float x2 = 0, float y2 = 0, float z2 = 0, float rotate2 = 0); // 그리퍼의 움직임 좌표
  void send_request(const std::array<float, 4> &pick_pos, const std::array<float, 4> &place_pos);
  void send_request_yut_pos(float (& pos)[4],int & left);
  int send_request_yut_res();



private:

  const int c[4] = {2, 73, 10, 0};//yut container centor
  float yut_container[4][4] = {
    {c[0] + 1.5, c[1] + 1.5, c[3] + 0, c[4] + 0},
    {c[0] - 1.5, c[2] + 1.5, c[3] + 0, c[4] + 0},
    {c[0] + 1.5, c[2] - 1.5, c[3] + 0, c[4] + 0},
    {c[0] - 1.5, c[3] - 1.5, c[3] + 0, c[4] + 0}
  };





  rclcpp::Client<xyz_interfaces::srv::YutnoriYutState>::SharedPtr     client_yut;       // Yut 던지고 어떤 윷이 나왔는지 알게 해 주는 스마트 포인터
  rclcpp::Client<xyz_interfaces::srv::YutnoriBoardState>::SharedPtr   client_board;     // 현재 보드 상태가 어떤지(상대)
  rclcpp::Client<xyz_interfaces::srv::PickPlace>::SharedPtr         client_position;  // 두산으로 가는 메세지1
  rclcpp::Client<xyz_interfaces::srv::YutThrow>::SharedPtr          client_throw;     //던지게 하는 클라이언트
  rclcpp::Client<xyz_interfaces::srv::YutPiecesPoision>::SharedPtr  client_yut_pos;   //던진 후 YUt의 위치
  //rclcpp::Client<xyz_interfaces::srv::YutnoriBoardState>::SharedPtr client_pieces_location; //
};



