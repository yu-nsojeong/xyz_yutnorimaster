#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include "rclcpp/rclcpp.hpp"

#include <rclcpp/executors/single_threaded_executor.hpp>

#include "xyz_interfaces/srv/yutnori_board_state.hpp"
#include "xyz_interfaces/srv/yutnori_yut_state.hpp"
#include "xyz_interfaces/srv/pick_place.hpp"
#include "xyz_interfaces/srv/yut_pick.hpp"
#include "xyz_interfaces/srv/yut_throw.hpp"
#include "xyz_interfaces/srv/yut_position.hpp" //
#include "xyz_interfaces/srv/yut_pick.hpp"

#include "xyz_interfaces/srv/next_turn.hpp"
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/int16.hpp"
#include "xyz_interfaces/msg/say_board_state.hpp"
#include "xyz_interfaces/msg/say_special_state.hpp"


class Player // 플레이어의 정보를 담고 있는 클래스
{
public:
  int player_num = -1;                                                              // 플레이어 번호
  int pieces_count = 4;                                                             // 말의 개수
  std::vector<std::pair<int, int>> location = {{-1, 1}, {-2, 1}, {-3, 1}, {-4, 1}}; //<index , tokens>
  // std::array<int,4> pieces_location = {0,0,0,0}; // 말의 위치와 토큰
  int finish_pieces = 0; // 0부터 시작 이녀석과 말의 개수가 같아지면 게임이 끝남
  std::string color = "red";



  const int short_cut_index[4] = {5, 10, 22, 27}; // 지름길 인덱스

  const float bc[4] = {0, 0, 0, 0};
  float robot_move_y = (37.5) * 4;

  // const std::map<int, std::tuple<float, float, float, float>> board_coor =
  //     {

  //       {0, {640.0, -305.0 + robot_move_y, 5, 0}}, {1, {640.0, -235.0 +robot_move_y , 5, 0}}, {2, {640.0, -180.0, 5, 0}}, {3, {640.0, -120.0, 5, 0}}, {4, {640.0, -60.0, 5, 0}},
  //       {5, {640.0, 20.0 + robot_move_y, 5, 0}},   {6, {570.0, 19.0 + robot_move_y, 3, 0}},   {7, {510.0, 18.0, 1, 0}},   {8, {450.0, 17.0, 0, 0}},   {9, {390.0, 16.0, 0, 0}},
  //       {10, {320.0, 15.0 + robot_move_y, 0, 0}},  {11, {320.0, -55.0, 3, 0}}, {12, {320.0, -115.0, 1, 0}},{13, {320.0, -175.0, 0, 0}},{14, {320.0, -235.0, 0, 0}},
  //       {15, {320.0, -305.0 + robot_move_y,0, 0}}, {16, {390.0, -305.0, 0, 0}},{17, {450.0, -305.0, 0, 0}},{18, {510.0, -305.0, 0, 0}},{19, {570.0, -305.0, 0, 0}},

  //       {20, {580.0, -40.0, 0, 0}}, {21, {540.0, -80.0, 0, 0}}, {22, {480.0, -145.0, 3, 0}},  {23,  {420.0, -205.0, 0, 0}}, {24, {380.0, -245.0, 0, 0}},
  //       {25, {380.0, -45.0, 0, 0}}, {26, {425.0, -85.0, 0, 0}}, {27, {480.0, -145.0, 3, 0}},  {28, {540.0, -205.0, 0, 0}},  {29, {580.0, -245.0, 0, 0}},

  //       {-1, {390.0, 76.0, 0, 90}},    {-2, {450.0, 77.0, 0, 90}},  {-3, {510.0, 80.0, 3, 90}},  {-4, {570.0, 80.0, 3, 90}},
  //       {-5, {390.0, -370.0, 0, 0}},  {-6, {450.0, -370.0, 0, 0}},{-7, {510.0, -370.0, 3, 0}},{-8, {570.0, -370.0, 3, 0}},

  //       {30, {100.0, -305.0, 0, 0}}

  //     };
  const std::map<int, std::tuple<float, float, float, float>> board_coor =
    {

      //       {0, {642.0, -105.0, 4, 0}}, {1, {637.0, -86.0, 4, 0}}, {2, {640.0, -32.0, 4, 0}}, {3, {640.0, 30.0, 4, 0}}, {4, {638.0, 88.0, 4, 0}}, {5, {641.0, 170.0, 5, 0}},
      // {6, {572.0, 169.0, 3, 0}}, {7, {516.0, 163.0, 2, 0}}, {8, {448.0, 167.0, 1, 0}}, {9, {394.0, 166.0, 0, 0}}, {10, {320.0, 165.0, 0, 0}},
      // {11, {322.0, 90, 0, 0}}, {12, {323.0, 37.0, 0, 0}}, {13, {323.0, -28.0, 0, 0}}, {14, {321.0, -82.0, 0, 0}}, {15, {333.0, -148.0, 0, 0}},
      // {16, {390.0, -158.0, 0, 0}}, {17, {449.0, -158.0, 0, 0}}, {18, {510.0, -159.0, 2, 0}}, {19, {575.0, -160.0, 3, 0}},

      // {20, {578.0, 99.0, 3, 0}}, {21, {539.0, 58, 2, 0}}, {22, {480.0, 11.0, 1, 0}},  {23, {417.0, -55.0, 0, 0}}, {24, {382.0, -90.0, 0, 0}},
      // {25, {380.0, 102.0, 0, 0}}, {26, {423.0, 60.0, 0, 0}}, {27, {480.0, 11.0, 1, 0}},  {28, {545.0, -55.0, 2, 0}},  {29, {579.0, -100.0, 3, 0}},

      // {-1, {455, 236, 1, 90}}, {-2, {513.0, 229, 2, 90}}, {-3, {570.0, 231, 3, 90}}, {-4, {629.0, 227.0, 5, 90}},
      // {-5, {394.0, -220.0, 0, 0}}, {-6, {437.0, -226, 0, 0}}, {-7, {510.0, -222.0, 2, 0}}, {-8, {565.0, -223.0, 3, 0}},

      //{30, {198.0, 180.0, 40, 0}}

      {0, {640.0, -305.0 + robot_move_y, 5, 0}}, {1, {640.0, -235.0 + robot_move_y, 5, 0}}, {2, {640.0, -180.0 + robot_move_y, 5, 0}}, {3, {640.0, -120.0 + robot_move_y, 5, 0}}, {4, {640.0, -60.0 + robot_move_y, 5, 0}},
      {5, {640.0, 20.0 + robot_move_y, 5, 0}},   {6, {570.0, 19.0 + robot_move_y, 3, 0}},   {7, {510.0, 18.0 + robot_move_y, 1, 0}},   {8, {450.0, 17.0 + robot_move_y, 0, 0}},   {9, {390.0, 16.0 + robot_move_y, 0, 0}},
      {10, {320.0, 15.0 + robot_move_y, 0, 0}},  {11, {320.0, -55.0 + robot_move_y, 3, 0}}, {12, {320.0, -115.0 + robot_move_y, 1, 0}}, {13, {320.0, -175.0 + robot_move_y, 0, 0}}, {14, {320.0, -235.0 + robot_move_y, 0, 0}},
      {15, {320.0, -305.0 + robot_move_y, 0, 0}}, {16, {390.0, -305.0 + robot_move_y, 0, 0}}, {17, {450.0, -305.0 + robot_move_y, 0, 0}}, {18, {510.0, -305.0 + robot_move_y, 0, 0}}, {19, {570.0, -305.0 + robot_move_y, 0, 0}},

      {20, {580.0, -40.0 + robot_move_y, 0, 0}}, {21, {540.0, -80.0 + robot_move_y, 0, 0}}, {22, {480.0, -145.0 + robot_move_y, 3, 0}},  {23, {420.0, -205.0 + robot_move_y, 0, 0}}, {24, {380.0, -245.0 + robot_move_y, 0, 0}},
      {25, {380.0, -45.0 + robot_move_y, 0, 0}}, {26, {425.0, -85.0 + robot_move_y, 0, 0}}, {27, {480.0, -145.0 + robot_move_y, 3, 0}},  {28, {540.0, -205.0 + robot_move_y, 0, 0}},  {29, {580.0, -245.0 + robot_move_y, 0, 0}},

      {-1, {390.0+60, 76.0 + robot_move_y, 0, 90}}, {-2, {450.0+60, 77.0 + robot_move_y, 0, 90}}, {-3, {510.0+60, 80.0 + robot_move_y, 3, 90}}, {-4, {570.0+60, 80.0 + robot_move_y, 3, 90}},
      {-5, {390.0, -370.0 + robot_move_y, 0, 0}}, {-6, {450.0, -370.0 + robot_move_y, 0, 0}}, {-7, {510.0, -370.0 + robot_move_y, 3, 0}}, {-8, {570.0, -370.0 + robot_move_y, 3, 0}},

      //{30, {100.0, -305.0 + robot_move_y, 0, 0}}
      {30, {198.0, 180.0, 40, 0}}

    };


  // 플레이어 보드의 사항
  //
  Player() {}
  Player(int playernum) { player_num = playernum; }
  ~Player() {}

  //
  bool isGmaeFinish()
  {
    bool result = false;
    if (finish_pieces == pieces_count)
      result = true;

    std::cout << "finish_pieces : " << finish_pieces << std::endl;
    return result;
  };
};







class Master : public rclcpp::Node
{
public:
  Player robot;       // my
  Player player;      // other
  int turn_count = -1; // -1으로 리셋(-1이 첫번째) 0이 되면 게임 시작
  int player_num = 2; // 몇명의 플레이어가 있는지
  int current_yut;    // 윷 상태// 도 :1,개: 2,걸 : 3, 윷 : 4, 모 : 5, 빽도 : -1
  bool kill_flag = false;

  Master() : Node("yutnori_master"), robot(1), player(0)
  {
    // 서비스 클라이언트 생성
    client_yut = this->create_client<xyz_interfaces::srv::YutnoriYutState>("yut_state");
    client_board = this->create_client<xyz_interfaces::srv::YutnoriBoardState>("board_state");
    client_pickplace = this->create_client<xyz_interfaces::srv::PickPlace>("pick_place");
    client_throw = this->create_client<xyz_interfaces::srv::YutThrow>("yut_throw");
    client_yut_pos = this->create_client<xyz_interfaces::srv::YutPosition>("yut_position");
    client_pick_put = this->create_client<xyz_interfaces::srv::YutPick>("yut_pick");
    Client_next_turn = this->create_client<xyz_interfaces::srv::NextTurn>("next_turn");
    Publisher_say_board_state = this->create_publisher<xyz_interfaces::msg::SayBoardState>("say_board_state_topic",100);

    publisher_resui = this->create_publisher<std_msgs::msg::Int16>("res",100);

    subscription_ = this->create_subscription<std_msgs::msg::Bool>(
        "topic", 10, std::bind(&Master::topic_callback, this, std::placeholders::_1));
    subscription_ = this->create_subscription<std_msgs::msg::Bool>(
        "start", 10, std::bind(&Master::start_callback, this, std::placeholders::_1));

    player.location.clear();


    while(turn_count == -1 )
    {

      RCLCPP_INFO(get_logger(),"GMAE LOADING... ...");
      send_request_next_turn();

    }



    while(1)
    {
      playGame();
    }

  } // 노드 이름을 명시적으로 설정
  ~Master() { /*delete player;*/ }

  // 추가적인 게임 로직이나 멤버 함수가 여기에 추가될 수 있음

  void playGame();  // 전체적인 게임 플레이
  void gameReset(); // 게임 재시도 할 때 리셋해주는 거 아직 구현 x
  bool endGame();   // ok // 게임 끝남 // 말이 다 들어온 플레이어가 있거나, 걍 강제종료 했을 때.

  bool isMyTurn(const Player &player); // "player" 턴인지 확인

  void throwYut();                                       // 윷 던지기
  int yutAnalysis();                                     // 윷 상태 분석
  int boardAnalysis(int current_state, int &next_index); // 리턴 값 옮길 말 인덱스, 어떤 움직임인지(상대 말을 치워야 하거나 쌓아야 하거나 하는 경우), 어디로 이동되는지 인덱스

  void cleanUpBoard();
  void movePieces(int piece_index, int next_index);
  void boardUpDate();
  void seeOtherBoard();
  void seeMyBoard();

  // 움직임
  void pickAndPlace(float x = 0, float y = 0, float z = 0, float rotate = 0, float x2 = 0, float y2 = 0, float z2 = 0, float rotate2 = 0); // 그리퍼의 움직임 좌표
  void pickAndPlace(int pick_index, int place_index, float place_z = 0);

  void timer_callback();

  // send request

  int send_request_yut_res();
  void send_request_pick_place(const std::array<float, 4> &pick_pos, const std::array<float, 4> &place_pos);
  void send_request_yut_pos(float (&pos)[4], int &left);
  void send_request_yut_throw();
  void send_request_board_state(std::vector<std::pair<int, int>> &player1,
                                std::vector<std::pair<int, int>> &player2);
  void send_request_yut_pick(float x, float y, float z, float r, int left);

  void send_request_next_turn();
  void send_msg_say_board_state(int state  = 0);


  void check_goal_tokens(std::vector<std::pair<int, int>> &befor, // pair <pos_index, token num>
                                      std::vector<std::pair<int, int>> &after);

  int kill_tokens = 0;

  int game_state_flag;
  bool start_flag = false;
  bool in_game_flag = false;
  bool end_game_flag = false;
  bool end_say_flag = true;

  #define GAME_START 0
  #define IN_GAME 1
  #define END_GAME 2

  #define YUT_THROW 3
  #define MY_TURN_RES 4
  #define BUTTON_PUSH 5
  #define KILL 6





  //void send_request_say_board_state();
  //void send_request_say_special_state(const std::string &say);
  void print_dbg()
  {

    std::cout << " robot : " << std::endl;
    for (size_t i = 0; i < robot.location.size(); i++)
    {
      std::cout << robot.location[i].first << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < robot.location.size(); i++)
    {
      std::cout << robot.location[i].second << " ";
    }
    std::cout << std::endl;
    std::cout << " player : " << std::endl;
    for (size_t i = 0; i < player.location.size(); i++)
    {
      std::cout << player.location[i].first << " ";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < player.location.size(); i++)
    {
      std::cout << player.location[i].second << " ";
    }
    std::cout << std::endl;
  }

private:
  const float c[4] = {2, 73, 10, 0}; // yut container centor
  float yut_container[4][4] = {
      {c[0] + 1.5f, c[1] + 1.5f, c[3] + 0.f, c[4] + 0.f},
      {c[0] - 1.5f, c[2] + 1.5f, c[3] + 0.f, c[4] + 0.f},
      {c[0] + 1.5f, c[2] - 1.5f, c[3] + 0.f, c[4] + 0.f},
      {c[0] - 1.5f, c[3] - 1.5f, c[3] + 0.f, c[4] + 0.f}};

  rclcpp::Client<xyz_interfaces::srv::YutnoriYutState>::SharedPtr client_yut;     // Yut 던지고 어떤 윷이 나왔는지 알게 해 주는 스마트 포인터
  rclcpp::Client<xyz_interfaces::srv::YutnoriBoardState>::SharedPtr client_board; // 현재 보드 상태가 어떤지(상태) 말 위치와 말 몇개 올려있는지
  rclcpp::Client<xyz_interfaces::srv::PickPlace>::SharedPtr client_pickplace;     // 두산으로 가는 메세지1
  rclcpp::Client<xyz_interfaces::srv::YutThrow>::SharedPtr client_throw;          // 던지게 하는 클라이언트
  rclcpp::Client<xyz_interfaces::srv::YutPosition>::SharedPtr client_yut_pos;     // 던진 후 YUt의 위치
  rclcpp::Client<xyz_interfaces::srv::YutPick>::SharedPtr client_pick_put;
  rclcpp::Client<xyz_interfaces::srv::NextTurn>::SharedPtr Client_next_turn;
  rclcpp::Publisher<xyz_interfaces::msg::SayBoardState>::SharedPtr Publisher_say_board_state;


  rclcpp::Publisher<std_msgs::msg::Int16>::SharedPtr publisher_resui;

  const std::map<int, int> vtom_coor = {
    {20,27},{21,25},
    {22,26},{23,20},
    {24,21},{25,24},
    {26,23},{27,29},
    {28,28}
  };

  int mapping(int input);

  void topic_callback(const std_msgs::msg::Bool::SharedPtr msg)
  {

    RCLCPP_WARN(get_logger(),"talk callback!!!");
    send_msg_say_board_state(BUTTON_PUSH);
    // this->turn_count++;
    // yourturn_active_ = false;

  }

  void start_callback(const std_msgs::msg::Bool::SharedPtr msg)
  {
      std :: cout<<"시작이 왔다."<<std::endl;

      this->turn_count++;
    // yourturn_active_ = false;
  }
  rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr timer_;

  bool yourturn_active_ = true;
};
