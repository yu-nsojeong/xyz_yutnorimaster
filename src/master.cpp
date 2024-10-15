#include "yutnori_master/master.hpp"

bool Master::isMyTurn(const Player &player)
{
  bool is = false;
  if (player.player_num == this->turn_count % this->player_num)
    is = true;

  return is;
}

void Master::starGame()
{
}

void Master::playGame()
{

#define testmode 1

  RCLCPP_INFO(this->get_logger(), "Game Player11111...");
  while (endGame())
  {
    if (isMyTurn(robot)) // 내 턴인 경우
    {
      RCLCPP_INFO(this->get_logger(), "myturn...");

      // 윷 정리 //윷 위치 받아오기.(4개 다? 부딪히면 난리나니까 걍 4번 반복이 나을 듯)//위치 이동, 모션 실행 //
      cleanUpBoard();
      // 윷을 던지기 (잡기)
      throwYut();
      // 윷의 상태 확인(결과)
      int yut_state;
#if 0
    yut_state = yutAnalysis();
#endif
#if testmode

      std::cout << "yut state : " << std::endl;
      std::cin >> yut_state;
#endif

      // 상대 말판의 상태 확인 //player board update
      seeOtherBoard();

      // 최적의 움직임 계산
      int move_index;
      int next_pos;
      move_index = boardAnalysis(yut_state, next_pos);

      // 로봇 말 이동 내 보드 상태 업데이트

      movePieces(move_index, next_pos);

      // 윷과 모가 나오지 않았다면 턴을 종료
      if (yut_state != 5 && yut_state != 4)
        this->turn_count++;
    }
    else
    {
    }
  }
}
void Master::gameReset()
{
}
bool Master::endGame()
{
  if (robot.isend())
    return true; // myrobot
                 // for (int i = 0; i < player_num; i++)
                 //   if (player[i].isend())
                 //     return true; // others
  if (player.isend())
    return true;

  return false;
}

void Master::cleanUpBoard()
{

  for (int i = 0; i < 4; i++)
  {
    // 윷 좌표 받아와

    float pos[4] = {
        0,
    }; // x,y,z, rotate
    int left;
    send_request_yut_pos(pos, left);

    if (!left)
      break; // 남은게 없으면 끝내기

    // 좌표가지고 픽엔 플레이스 하는데 , 플레이스는 이미 지정 위치 있으니까 디파인 해놔야겠다.
    pickAndPlace(pos[0], pos[1], pos[2], pos[3], yut_container[i][0], yut_container[i][1], yut_container[i][2], yut_container[i][3]);
  }
}

void Master::seeOtherBoard()
{

  auto request = std::make_shared<xyz_interfaces::srv::YutnoriBoardState::Request>();
  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_board->async_send_request(request);
  if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    // 응답 받음
    auto response = result_future.get();

    // 결과 확인 및 처리
    for (int i = 0; i < 4; i++)
    {
      player.pieces_location[i] = response->pieces_state[i];
    }
  }
}

void Master::throwYut()
{
  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutThrow::Request>();
  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_throw->async_send_request(request);

  // 응답이 올 때까지 대기
  if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    // 응답 받음
    auto response = result_future.get();

    // 결과 확인 및 처리
    if (response->success)
    {
      RCLCPP_INFO(this->get_logger(), "Pick and place operation succeeded.");
    }
    else
    {
      RCLCPP_WARN(this->get_logger(), "throwYut operation failed. Failure");
    }
  }
  else
  { // 서비스 호출 실패
    RCLCPP_ERROR(this->get_logger(), "Failed to call service throwYut.");
  }
}

int Master::yutAnalysis()
{

  int yut_state = -2;
  // 비전하고 서비스 통신
  yut_state = send_request_yut_res();

  return yut_state;
}

void Master::movePieces(int piece_index, int next_pos)
{

  auto current_coor = robot.board_coor.find(robot.pieces_location[piece_index]);
  auto [x, y, z, r] = current_coor->second;

  auto next_coor = robot.board_coor.find(next_pos);
  auto [x2, y2, z2, r2] = next_coor->second;

  pickAndPlace(x, y, z, r, x, y2, z2, r2);

  robot.pieces_location[piece_index] = next_pos;
}
void Master::boardUpDate()
{
}

/**
 * @param 현재 던진 윷 나온 결과
 * @return 움직일 말의 인덱슨
 *
 */
int Master::boardAnalysis(int current_yut, int &next_pos)
{
  // 1. 골 지점으로 들어올 수 있는 말이 있는지?
  // 2. 겹칠 수 있는 말이 있는지?
  // 3. 잡을 수 있는 말이 있는지
  // 4. 지금길로 들어갈 수 있는 말이있는지?

  RCLCPP_INFO(this->get_logger(), "Analyzing Yut state...");

  // 윷 상태에 따른 움직임 계산 (current_yut: 도(1), 개(2), 걸(3), 윷(4), 모(5), 빽도(-1))
  int move_steps = current_yut; // 윷 결과로 몇 칸 이동할지 결정

  int new_pos[robot.pieces_location.size()];

  // 1. 골 지점으로 들어올 수 있는 말이 있는지?
  for (int i = 0; i < robot.pieces_location.size(); ++i)
  {
    int current_pos = robot.pieces_location[i];
    new_pos[i] = current_pos + move_steps;

    if (robot.short_cut_index[0] == current_pos)
    {
      new_pos[i] = current_pos + 25 + move_steps; // 30
    }
    else if (robot.short_cut_index[1] == current_pos)
    {
      new_pos[i] = current_pos + 35 + move_steps; // 45
    }
    else if (robot.short_cut_index[2] == current_pos)
    {
      new_pos[i] = current_pos + 15 + move_steps; //
    }

    if ((20 < new_pos[i] && new_pos[i] < 30) || (41 < new_pos[i] && new_pos[i] < 45) || (51 < new_pos[i])) // 골 지점 도착 했는지?
    {
      next_pos = new_pos[i];
      return i;
    }
  }

  // 2. 겹칠 수 있는지 확인 (같은 말이 같은 위치에 있는지)
  for (int i = 0; i < robot.pieces_location.size(); ++i)
  {
    for (int j = 0; j < robot.pieces_location.size(); ++j)
    {
      if (i != j && robot.pieces_location[j] == new_pos[i])
      {

        RCLCPP_INFO(this->get_logger(), "Can overlap with piece %d at position %d", i, robot.pieces_location[j]);
        next_pos = new_pos[i];
        return i;
      }
    }
  }

  // 3. 잡을 수 있는 말이 있는지 확인 (현재 위치에 상대방 말이 있는지)
  for (int i = 0; i < robot.pieces_location.size(); i++)
  {
    // for (int j = 0; j < player_num; ++j)
    // {
    for (int k = 0; k < player.pieces_location.size(); ++k)
    {
      if (player.pieces_location[k] == new_pos[i]) // 같은 위치에 상대 말이 있는지 확인
      {
        RCLCPP_INFO(this->get_logger(), "Captured opponent's piece at position %d!", new_pos);
        // player[j].pieces_location[k] = 0; // 상대 말 다시 시작 위치로
        next_pos = new_pos[i];
        return i;
      }
    }
    //}
  }

  // 4. 지금길로 들어갈 수 있는지 확인
  for (int i = 0; i < robot.pieces_location.size(); ++i)
  {
    const int shortcutsize = 4;
    for (int j = 0; j < shortcutsize; j++)
    {
      if (new_pos[i] == robot.short_cut_index[j])
      {
        next_pos = new_pos[i];
        return i;
      }
    }
  }

  // 5. 가장 멀리 가 있는 애 보내기
  int farindex = -10;
  int farmax = -10;
  for (int i = 0; i < robot.pieces_location.size(); ++i)
  {
    if (farmax < robot.short_cut_index[i])
    {
      farindex = i;
      next_pos = new_pos[i];
      farmax = robot.short_cut_index[i];
    }
  }
  return farindex;
}

// pick place 시키는 함수
void Master::pickAndPlace(float x, float y, float z, float rotate, float x2, float y2, float z2, float rotate2)
{
  std::array<float, 4> pick_pos = {x, y, z, rotate};
  std::array<float, 4> place_pos = {x2, y2, z2, rotate2};
  send_request(pick_pos, place_pos);
}

int Master::send_request_yut_res()
{

  int result = -2;
  auto request = std::make_shared<xyz_interfaces::srv::YutnoriYutState::Request>();
  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_yut->async_send_request(request);
  if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    // 응답 받음
    auto response = result_future.get();

    // 결과 확인 및 처리
    result = response->yut_info;
  }

  return result;
}

// 윷 위치 알려달라고 요청 보내는 함수
void Master::send_request_yut_pos(float (&pos)[4], int &left)
{
  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutPiecesPoision::Request>();

  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_yut_pos->async_send_request(request);

  // 응답이 올 때까지 대기
  if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    // 응답 받음
    auto response = result_future.get();

    // 결과 확인 및 처리
    left = response->left_num;
    pos[0] = response->pos[0];
    pos[1] = response->pos[1];
    pos[2] = response->pos[2];
    pos[3] = response->pos[3];
  }
  else
  {
    // 서비스 호출 실패
    RCLCPP_ERROR(this->get_logger(), "Failed to call service pick_and_place.");
  }
}

// pick and place 요청 보내는 함수
void Master::send_request(const std::array<float, 4> &pick_pos, const std::array<float, 4> &place_pos)
{
  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::PickPlace::Request>();
  request->pick_pos = pick_pos;   // pick 위치 설정
  request->place_pos = place_pos; // place 위치 설정

  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_position->async_send_request(request);

  // 응답이 올 때까지 대기
  if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
      rclcpp::FutureReturnCode::SUCCESS)
  {
    // 응답 받음
    auto response = result_future.get();

    // 결과 확인 및 처리
    if (response->result)
    {
      RCLCPP_INFO(this->get_logger(), "Pick and place operation succeeded.");
    }
    else
    {
      RCLCPP_WARN(this->get_logger(), "Pick and place operation failed. Failure code: %d", response->failure_code);
      // 실패 코드에 따른 처리 (예시)
      if (response->failure_code == 1)
      {
        RCLCPP_WARN(this->get_logger(), "Impossible to pick.");
      }
      else if (response->failure_code == 2)
      {
        RCLCPP_WARN(this->get_logger(), "Impossible to place.");
      }
      else if (response->failure_code == 3)
      {
        RCLCPP_WARN(this->get_logger(), "No object to pick.");
      }
    }
  }
  else
  {
    // 서비스 호출 실패
    RCLCPP_ERROR(this->get_logger(), "Failed to call service pick_and_place.");
  }
}
