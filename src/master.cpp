#include "yutnori_master/master.hpp"

bool Master::isMyTurn(const Player &player)
{
  bool is = false;
  if (player.player_num == this->turn_count % this->player_num)
    is = true;

  std::cout << "this->turn_count" << this->turn_count << std::endl;
  return is;
}

void Master::playGame()
{

  // this->timer_->cancel();

#define testmode 1

  RCLCPP_INFO(this->get_logger(), "Game Player");
  if (!endGame())
  {
    if (isMyTurn(robot)) // 내 턴인 경우
    {
      RCLCPP_INFO(this->get_logger(), "myturn...");
      // 윷 정리 //윷 위치 받아오기.(4개 다? 부딪히면 난리나니까 걍 4번 반복이 나을 듯)//위치 이동, 모션 실행 //
      // cleanUpBoard();
      // 윷을 던지기 (잡기)

      RCLCPP_INFO(this->get_logger(), "Throw Yut!");
      throwYut();

      // 윷의 상태 확인(결과)
      int yut_state;
      if (testmode)
      {
        std::cout << "yut state : " << std::endl;
        std::cin >> yut_state;
      }
      else
      {

        // RCLCPP_INFO(this->get_logger(), "yutAnalysis!");
        // yut_state = yutAnalysis(); // 도개걸윷모 어떤 것이 나왔는지
      }

      // 상대 말판의 상태 확인 //player board update
      RCLCPP_INFO(this->get_logger(), "seeOtherBoard!");
      seeOtherBoard();
      // 내 보드 보고 업데이트 잡혔을 때를 대비
      RCLCPP_INFO(this->get_logger(), "seeMyBoard!");
      seeMyBoard();

      // 최적의 움직임 계산
      int move_index;
      int next_pos;
      move_index = boardAnalysis(yut_state, next_pos);

      // 로봇 말 이동 내 보드 상태 업데이트
      RCLCPP_INFO(this->get_logger(), "movePieces");
      movePieces(move_index, next_pos);

      // 윷과 모가 나오지 않았다면 턴을 종료
      if ((yut_state != 5 && yut_state != 4)) //! 캐치 플래그 추가
      {
        // send_request_say_board_state();
        // send_request_say_special_state(std::string("I'm done with my turn. Send me something to say"));
        this->turn_count++;
      }
    }
    else
    {

      RCLCPP_INFO(this->get_logger(), "Your turn!");
      // 게임 종료 대기 (콜백에서 처리)
      send_request_next_turn();
    }

    print_dbg();
  }
  else
  {

    RCLCPP_INFO(this->get_logger(), "game end!");
  }

  // this->timer_->reset();
}

// void Master::playGame()
// {
// }

void Master::gameReset()
{
}
bool Master::endGame()
{
  if (robot.isGmaeFinish())
  {
    // send_request_say_special_state(std::string("When I won"));
    return true;
  }

  if (player.isGmaeFinish())
  {
    // send_request_say_special_state(std::string("When the opponent wins"));
    return true;
  }
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

    send_request_yut_pick(pos[0], pos[1], pos[2], pos[3]);
  }
}

void Master::seeOtherBoard()
{
  std::vector<std::pair<int, int>> redPlayer;
  std::vector<std::pair<int, int>> bluePlayer;
  send_request_board_state(redPlayer, bluePlayer);

  if (this->robot.color == "red") // robot이 red면 상대는 blue 는 2
  {
    this->player.location = bluePlayer;
  }
  else
  { // robot이 blue면 상대는 red red는 1
    this->player.location = redPlayer;
  }
}

void Master::seeMyBoard()
{

  std::vector<std::pair<int, int>> redPlayer;
  std::vector<std::pair<int, int>> bluePlayer;
  send_request_board_state(redPlayer, bluePlayer);

  bool index[robot.location.size()] = {
      false,
  };

  if (this->robot.color == "red") // robot이 red면 상대는 blue 는 2
  {
    // 인덱스 맞는거 잡고 안 맞으면 안 맞은 만큼 업뎃 해줘야 겠지...
    for (size_t i = 0; i < redPlayer.size(); i++)
    {
      for (size_t j = 0; j < robot.location.size(); j++)
      {
        if (redPlayer[i].first == robot.location[j].first)
        {
          index[j] = true;
          break;
        }
      }
    }
    // flase 되어있으면 삭제된거임
    int delete_tokens = 0;
    for (size_t i = 0; i < robot.location.size(); i++)
    {
      if (index[i] == false)
      {
        delete_tokens = delete_tokens + robot.location[i].second;
        robot.location.erase(robot.location.begin() + i);
      }
    }
    // 삭제된 만큼 채워주기
    for (size_t i = 0; i < delete_tokens; i++)
    {
      for (int j = -1; j >= -4; j--)
      {
        auto it = std::find_if(robot.location.begin(), robot.location.end(), [j](const std::pair<int, int> &element)
                               { return element.first == j; });
        if (it == robot.location.end()) // 끝까지 안 나옴
        {
          robot.location.push_back(std::make_pair(j, 1));
        }
      }
    }
  }
  else
  { // robot이 blue면 상대는 red red는 1
    // 인덱스 맞는거 잡고 안 맞으면 안 맞은 만큼 업뎃 해줘야 겠지...
    for (size_t i = 0; i < bluePlayer.size(); i++)
    {
      for (size_t j = 0; j < robot.location.size(); j++)
      {
        if (bluePlayer[i].first == robot.location[j].first)
        {
          index[j] = true;
          break;
        }
      }
    }
    // flase 되어있으면 삭제된거임
    int delete_tokens = 0;
    for (size_t i = 0; i < robot.location.size(); i++)
    {
      if (index[i] == false)
      {
        delete_tokens = delete_tokens + robot.location[i].second;
        robot.location.erase(robot.location.begin() + i);
      }
    }
    // 삭제된 만큼 채워주기
    for (size_t i = 0; i < delete_tokens; i++)
    {
      for (int j = -1; j >= -4; j--)
      {
        auto it = std::find_if(robot.location.begin(), robot.location.end(), [j](const std::pair<int, int> &element)
                               { return element.first == j; });
        if (it == robot.location.end()) // 끝까지 안 나옴
        {
          robot.location.push_back(std::make_pair(j, 1));
        }
      }
    }
  }
}

void Master::throwYut()
{
  RCLCPP_INFO(this->get_logger(), "in throwYut");
  send_request_yut_throw();
}

int Master::yutAnalysis()
{

  int yut_state = -2;
  // 비전하고 서비스 통신
  yut_state = send_request_yut_res();

  return yut_state;
}

// 말을 움직이고 움직인 말을 내 보드에 업데이트
void Master::movePieces(int piece_index, int next_pos)
{
  int current_pos = robot.location[piece_index].first;
  int current_tokens = robot.location[piece_index].second;
  robot.location.erase(robot.location.begin() + piece_index);
  // print_dbg();

  std::cout << " move : " << current_pos << "next : " << next_pos << std::endl;
  float place_z = 0;
  const float height = 10;
  bool overlap = false;
  for (size_t i = 0; i < robot.location.size(); i++)
  { // 겹치는 값이 있는지 확인 하여 z 값 변화.

    if (robot.location[i].first == next_pos)
    {

      std::cout << "nononononononoon" << std::endl;

      robot.location[i].second++;
      place_z = (robot.location[i].second - 1) * height; //
      overlap = true;
      break;
    }
  }

  if (!overlap)
  {
    std::cout << "lakdjfklasdjfkljaslkdfjlasf" << std::endl;
    robot.location.push_back(std::make_pair(next_pos, current_tokens));
  }

  if (next_pos == 30)
  { // 도착했다면
    robot.finish_pieces += current_tokens;
  }
  for (size_t i = 0; i < robot.location.size(); i++) // 30 지워버려
  {
    if (robot.location[i].first == 30)
      robot.location.erase(robot.location.begin() + i);
  }

  pickAndPlace(current_pos, next_pos, place_z);
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
  int move_steps = current_yut;       // 윷 결과로 몇 칸 이동할지 결정
  int new_pos[robot.location.size()]; // newpos
  const int new_pos_size = robot.location.size();

  for (size_t i = 0; i < robot.location.size(); ++i) // 말의 다음위치 구하기 현재 위치가 지름길 위치인지? 지름길 위치라면 들어간 걸로 숫자 바꿔주기 아니라면 그냥 더하기
  {
    int current_pos = robot.location[i].first;   // index
    if (robot.short_cut_index[0] == current_pos) // 5
    {
      new_pos[i] = 19 + move_steps;
    }
    else if (robot.short_cut_index[1] == current_pos) // 10
    {
      new_pos[i] = 24 + move_steps;
    }
    else if (robot.short_cut_index[2] == current_pos) // 22
    {
      new_pos[i] = 27 + move_steps;
    }
    else if (current_pos < 0)
    {
      new_pos[i] = 0 + move_steps;
    }
    else
    {

      new_pos[i] = current_pos + move_steps;

      if (20 < current_pos && current_pos < 24) // 지름길에서 나오는 경우
      {
        if (new_pos[i] > 24)
          new_pos[i] -= 10;
      }
    }
  }
  std::cout << "aaaa" << std::endl;

  int max_tokens = -1;
  int min_left = 100;
  int index;
  bool Arrivalflag = false;
  // 1. 골 지점으로 들어올 수 있는 말이 있는지? 토큰큰게 있으면 그것부터
  for (int i = 0; i < new_pos_size; ++i)
  {
    int left = 100;

    if (new_pos[i] >= 30) // 무조건 들어간 겨
    {
      left = new_pos[i] - 30;
      if (max_tokens <= robot.location[i].second && left <= min_left)
      { // update
        index = i;
        max_tokens = robot.location[i].second;
        min_left = left;
        next_pos = new_pos[i];
        Arrivalflag = true;
      }
    }
    else if ((15 < robot.location[i].first && robot.location[i].first <= 19) && 20 <= new_pos[i]) // 바깥 들어간 겨
    {
      left = new_pos[i] - 20;
      if (max_tokens <= robot.location[i].second && left <= min_left)
      { // update
        index = i;
        max_tokens = robot.location[i].second;
        min_left = left;
        next_pos = new_pos[i];
        Arrivalflag = true;
      }
    }
  }
  std::cout << "bbbb" << std::endl;
  if (Arrivalflag)
  {
    next_pos = 30;
    //! send_request_say_special_state(std::string("My token has reached the destination, and I earned points!"));
    return index;
  };

  // 2. 겹칠 수 있는지 확인 (같은 말이 같은 위치에 있는지)
  for (int i = 0; i < new_pos_size; ++i)
  {
    for (int j = 0; j < robot.location.size(); ++j)
    {
      if (i != j && robot.location[j].first == new_pos[i])
      {
        RCLCPP_INFO(this->get_logger(), "Can overlap with piece %d at position %d", i, robot.location[j]);
        next_pos = new_pos[i];
        return i;
      }
    }
  }

  // 3. 잡을 수 있는 말이 있는지 확인 (현재 위치에 상대방 말이 있는지)
  for (size_t i = 0; i < robot.location.size(); i++)
  {
    for (size_t k = 0; k < player.location.size(); ++k)
    {
      if (player.location[k].first == new_pos[i]) // 같은 위치에 상대 말이 있는지 확인
      {
        RCLCPP_INFO(this->get_logger(), "Captured opponent's piece at position %d!", new_pos);
        // send_request_say_special_state(std::string("Write down what you want to say when you catch the opponent's token"));
        pickAndPlace(new_pos[i], 0); // 겹치는 부분 치우는 부분
        // player[j].pieces_location[k] = 0; // 상대 말 다시 시작 위치로
        kill_flag = true;
        next_pos = new_pos[i];
        return i;
      }
    }
  }

  // 4. 지금길로 들어갈 수 있는지 확인
  for (size_t i = 0; i < robot.location.size(); ++i)
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
  for (size_t i = 0; i < robot.location.size(); ++i)
  {
    if (farmax < robot.location[i].first)
    {
      farindex = i;
      next_pos = new_pos[i];
      farmax = robot.location[i].first;
    }
  }
  return farindex;
}

// pick place 시키는 함수
void Master::pickAndPlace(float x, float y, float z, float rotate, float x2, float y2, float z2, float rotate2)
{

  std::array<float, 4> pick_pos = {x, y, z, rotate};
  std::array<float, 4> place_pos = {x2, y2, z2, rotate2};

  send_request_pick_place(pick_pos, place_pos);
}

void Master::pickAndPlace(int pick_index, int place_index, float place_z)
{

  if (place_index > 30)
    place_index = 30;

  auto pick_iter = robot.board_coor.find(pick_index);
  auto place_iter = robot.board_coor.find(place_index);

  // 맵에서 찾았는지 확인
  // if (pick_iter != robot.board_coor.end() && place_iter != robot.board_coor.end())
  // {
  std::tuple<float, float, float, float> pick = pick_iter->second;
  std::tuple<float, float, float, float> place = place_iter->second;

  // 이후 pick과 place 사용
  //}

  // std::tuple<float, float, float, float> pick = robot.board_coor.find(pick_index);
  // std::tuple<float, float, float, float> place = robot.board_coor.find(place_index);

  pickAndPlace(std::get<0>(pick), std::get<1>(pick), std::get<2>(pick), std::get<3>(pick),
               std::get<0>(place), std::get<1>(place), std::get<2>(place) + place_z, std::get<3>(place));
}

// 윷 던진 결과 알려달라고 요청 보내는 함수
int Master::send_request_yut_res()
{
  RCLCPP_INFO(this->get_logger(), "send_request_yut_res");
  int result = -2;
  auto request = std::make_shared<xyz_interfaces::srv::YutnoriYutState::Request>();
  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_yut->async_send_request(request);

  RCLCPP_INFO(this->get_logger(), "send_request_yut_res1");

  auto response = result_future.get();
  result = response->yut_info;

  RCLCPP_INFO(this->get_logger(), "send_request_yut_res2");
  RCLCPP_INFO(this->get_logger(), "rrrrrrrresult%d", result);

  // if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
  //     rclcpp::FutureReturnCode::SUCCESS)
  // {
  //   // 응답 받음
  //   auto response = result_future.get();

  //   // 결과 확인 및 처리
  //   result = response->yut_info;
  // }

  return result;
}

// 윷 위치 알려달라고 요청 보내는 함수
void Master::send_request_yut_pos(float (&pos)[4], int &left)
{
  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutPosition::Request>();

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
void Master::send_request_pick_place(const std::array<float, 4> &pick_pos, const std::array<float, 4> &place_pos)
{
  // 요청 객체 생성

  RCLCPP_INFO(this->get_logger(), "Service.  send_request_pick_place");
  // 서비스가 준비될 때까지 대기
  while (!client_pickplace->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_INFO(this->get_logger(), "Service not available, waiting...");
  }

  // 요청 메시지 생성
  auto request = std::make_shared<xyz_interfaces::srv::PickPlace::Request>();
  request->pick_pos = pick_pos;   // pick 위치 설정
  request->place_pos = place_pos; // place 위치 설정

  // 서비스 요청
  auto future = client_pickplace->async_send_request(request);

  // 응답 대기 및 처리
  if (rclcpp::spin_until_future_complete(get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_INFO(this->get_logger(), "Service call successful.");
  }
  else
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to call service.");
  }

  // auto response = result_future.get();
  // std::cout<<"request??????????????? : <"<<response->result<<">"<<std::endl;

  // // 응답이 올 때까지 대기
  // if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
  //     rclcpp::FutureReturnCode::SUCCESS)
  // {
  //   // 응답 받음
  //   auto response = result_future.get();

  //   // 결과 확인 및 처리
  //   if (response->result)
  //   {
  //     RCLCPP_INFO(this->get_logger(), "Pick and place operation succeeded.");
  //   }
  //   else
  //   {
  //     RCLCPP_WARN(this->get_logger(), "Pick and place operation failed. Failure code: %d", response->failure_code);
  //     // 실패 코드에 따른 처리 (예시)
  //     if (response->failure_code == 1)
  //     {
  //       RCLCPP_WARN(this->get_logger(), "Impossible to pick.");
  //     }
  //     else if (response->failure_code == 2)
  //     {
  //       RCLCPP_WARN(this->get_logger(), "Impossible to place.");
  //     }
  //     else if (response->failure_code == 3)
  //     {
  //       RCLCPP_WARN(this->get_logger(), "No object to pick.");
  //     }
  //   }
  // }
  // else
  // {
  //   // 서비스 호출 실패
  //   RCLCPP_ERROR(this->get_logger(), "Failed to call service pick_and_place.");
  // }
}

//
void Master::send_request_board_state(std::vector<std::pair<int, int>> &player1, // pair <pos_index, token num>
                                      std::vector<std::pair<int, int>> &player2)
{

  RCLCPP_INFO(this->get_logger(), "Service.  send_request_board_state");
  // 서비스가 준비될 때까지 대기
  while (!client_board->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_INFO(this->get_logger(), "Service not available, waiting...");
  }

  // 요청 메시지 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutnoriBoardState::Request>();

  // 서비스 요청
  auto future = client_board->async_send_request(request);

  // 응답 대기 및 처리
  if (rclcpp::spin_until_future_complete(get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_INFO(this->get_logger(), "Service call successful.");

    auto response = future.get();
    // 응답 처리
    if (response != nullptr)
    {
      for (size_t i = 0; i < response->positions_p1.size(); i++)
      {
        RCLCPP_INFO(this->get_logger(), "player 1 %d , %d", response->positions_p1[i], response->tokens_p1[i]);
        player1.push_back(std::make_pair(response->positions_p1[i], response->tokens_p1[i]));
      }
      for (size_t i = 0; i < response->positions_p2.size(); i++)
      {
        RCLCPP_INFO(this->get_logger(), "player 2 %d , %d", response->positions_p2[i], response->tokens_p2[i]);
        player2.push_back(std::make_pair(response->positions_p2[i], response->tokens_p2[i]));
      }
      RCLCPP_INFO(this->get_logger(), "보드 상태 업데이트 완료");
    }
    else
    {
      RCLCPP_ERROR(this->get_logger(), "응답이 비어있습니다.");
    }
  }
  else
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to call service.");
  }
}

void Master::send_request_yut_throw()
{
  RCLCPP_INFO(this->get_logger(), "Service.  send_request_yut_throw");
  // 서비스가 준비될 때까지 대기
  while (!client_throw->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_INFO(this->get_logger(), "Service not available, waiting...");
  }

  // 요청 메시지 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutThrow::Request>();

  // 서비스 요청
  auto future = client_throw->async_send_request(request);

  // 응답 대기 및 처리
  if (rclcpp::spin_until_future_complete(get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_INFO(this->get_logger(), "Service call successful.");
  }
  else
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to call service.");
  }
}

void Master::send_request_yut_pick(float x, float y, float z, float r)
{

  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutPick::Request>();
  std::array<float, 4> pickpos = {x, y, z, r};
  request->pick_pos = pickpos;
  request->place_num = 0;

  // 비동기 요청 보내고 응답을 기다림
  auto result_future = client_pick_put->async_send_request(request);

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
  }
  else
  {
    // 서비스 호출 실패
    RCLCPP_ERROR(this->get_logger(), "Failed to call service pick_and_place.");
  }
}

void Master::send_request_next_turn()
{
  RCLCPP_WARN(this->get_logger(), "send_request_next_turn.");
  auto request = std::make_shared<xyz_interfaces::srv::NextTurn::Request>();
  // request->data = true; // 요청할 데이터 설정

  // 서비스가 준비될 때까지 대기
  if (Client_next_turn->wait_for_service(std::chrono::seconds(1)))
  {
    auto result_future = Client_next_turn->async_send_request(request);

    // 응답이 올 때까지 대기
    if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
        rclcpp::FutureReturnCode::SUCCESS)
    {
      // 응답 받음
      auto response = result_future.get();

      // 결과 확인 및 처리
      if (response->next)
      {
        RCLCPP_INFO(this->get_logger(), "send_request_next_turn operation succeeded.");
      }
    }
    else
    {
      // 서비스 호출 실패
      RCLCPP_ERROR(this->get_logger(), "Failed to call ssend_request_next_turn.");
    }
  }
}

  // void Master::send_request_say_board_state()
  // {

  //   // 요청 객체 생성
  //   auto request = std::make_shared<xyz_interfaces::srv::SayBoardState::Request>();

  //   for (size_t i = 0; i < robot.location.size(); i++)
  //   {
  //     request->positions_p1.push_back(robot.location[i].first);
  //     request->tokens_p1.push_back(robot.location[i].second);
  //   }

  //   for (size_t i = 0; i < player.location.size(); i++)
  //   {
  //     request->positions_p2.push_back(player.location[i].first);
  //     request->tokens_p2.push_back(player.location[i].second);
  //   }

  //   // 비동기 요청 보내고 응답을 기다림
  //   auto result_future = client_say_board->async_send_request(request);

  // }
  // void Master::send_request_say_special_state(const std::string &say)
  // {

  //   // 요청 객체 생성
  //   auto request = std::make_shared<xyz_interfaces::srv::SaySpecialState::Request>();
  //   request->script = say;

  //   // 비동기 요청 보내고 응답을 기다림
  //   auto result_future = client_say_special->async_send_request(request);

  //   // 응답이 올 때까지 대기
  //   if (rclcpp::spin_until_future_complete(this->get_node_base_interface(), result_future) ==
  //       rclcpp::FutureReturnCode::SUCCESS)
  //   {
  //     // 응답 받음
  //     auto response = result_future.get();

  //     // 결과 확인 및 처리
  //     // if (response->success)
  //     // {
  //     //   RCLCPP_INFO(this->get_logger(), "Pick and place operation succeeded.");
  //     // }
  //   }
  //   else
  //   {
  //     // 서비스 호출 실패
  //     RCLCPP_ERROR(this->get_logger(), "Failed to call service send_request_say_special_state.");
  //   }
  // }
