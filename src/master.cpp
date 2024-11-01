#include "yutnori_master/master.hpp"

bool Master::isMyTurn(const Player &player)
{
  bool is = false;
  if (player.player_num == this->turn_count % this->player_num)
    is = true;

  RCLCPP_ERROR(this->get_logger(), "TURN COUNT : %d", this->turn_count);
  return is;
}

void Master::playGame()
{

#define testmode 0

  RCLCPP_WARN(this->get_logger(), "Game Player");
  if (!endGame())
  {
    if (isMyTurn(robot)) // 내 턴인 경우
    {

      RCLCPP_WARN(this->get_logger(), "seeOtherBoard!");
      seeOtherBoard();
      // 내 보드 보고 업데이트 잡혔을 때를 대비
      print_dbg();
      RCLCPP_WARN(this->get_logger(), "seeMyBoard!");
      seeMyBoard();

      RCLCPP_INFO(this->get_logger(), "myturn...");

      // 윷 정리
      RCLCPP_INFO(this->get_logger(), "cleanUpBoard...!!!");
      cleanUpBoard();
      if (endGame()){return;}


      // 윷을 던지기 (잡기)
      send_msg_say_board_state(YUT_THROW);
      RCLCPP_WARN(this->get_logger(), "Throw Yut!");
      throwYut();

      // 윷의 상태 확인(결과)
      int yut_state;
      if (testmode)
      {

        std::cout << "yut state : ";
        std::cin >> yut_state;
      }
      else
      {
        RCLCPP_WARN(this->get_logger(), "yutAnalysis!");
        yut_state = yutAnalysis(); // 도개걸윷모 어떤 것이 나왔는지

        if (yut_state == 0)
        {

          RCLCPP_WARN(this->get_logger(), "낙!!!!!!!");
          this->turn_count++;
          return;
        }
      }

      // // 상대 말판의 상태 확인 //player board update
      // RCLCPP_WARN(this->get_logger(), "seeOtherBoard!");
      // seeOtherBoard();
      // // 내 보드 보고 업데이트 잡혔을 때를 대비
      // print_dbg();
      // RCLCPP_WARN(this->get_logger(), "seeMyBoard!");
      // seeMyBoard(); //! 수정요함
      // print_dbg();

      // 최적의 움직임 계산
      int move_index;
      int next_pos;
      move_index = boardAnalysis(yut_state, next_pos);

      // 로봇 말 이동 내 보드 상태 업데이트
      RCLCPP_WARN(this->get_logger(), "movePieces");
      movePieces(move_index, next_pos);

      RCLCPP_WARN(this->get_logger(), "your dsadfkasdjfl!");

      // 윷과 모가 나오지 않았다면 턴을 종료
      if ((yut_state != 5 && yut_state != 4) && kill_flag == false) //! 캐치 플래그 추가
      {
        this->turn_count++;
      }
      kill_flag = false;
      send_msg_say_board_state(MY_TURN_RES);
    }
    else
    {
      //* 상대 턴
      RCLCPP_WARN(this->get_logger(), "your turn!");
      send_request_next_turn();

      //seeOtherBoard();
    }

    print_dbg();
  }
  else
  {

    RCLCPP_INFO(this->get_logger(), "game end!");
    if(robot.finish_pieces == 4)
    {
       RCLCPP_ERROR(this->get_logger(), "robot.finish_pieces!");
    auto msg = std_msgs::msg::Int16();
    msg.data = 1;
    publisher_resui->publish(msg);

    }
    else if(player.finish_pieces == 4)
    {
      RCLCPP_ERROR(this->get_logger(), "player.finish_pieces!");
    auto msg = std_msgs::msg::Int16();
    msg.data = 0;
    publisher_resui->publish(msg);

    }
    if(end_say_flag){
          send_msg_say_board_state(END_GAME);
          end_say_flag = false;
    }



  }
}

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

    auto msg = std_msgs::msg::Int16();
    msg.data = 0;
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
    std::cout << "레프트는 : " << left << std::endl;

    if (!left)
      break; // 남은게 없으면 끝내기
    // 좌표가지고 픽엔 플레이스 하는데 , 플레이스는 이미 지정 위치 있으니까 디파인 해놔야겠다.

    send_request_yut_pick(pos[0], pos[1], 10 /*pos[2]*/, pos[3], left);
  }
}

void Master::seeOtherBoard()
{
  std::vector<std::pair<int, int>> redPlayer;
  std::vector<std::pair<int, int>> bluePlayer;

  std::vector<std::pair<int, int>> Player_befor;
  Player_befor = player.location;

  send_request_board_state(redPlayer, bluePlayer);

  if (this->robot.color == "red") // robot이 red면 상대는 blue 는 2
  {
    this->player.location = bluePlayer;
  }
  else
  { // robot이 blue면 상대는 red red는 1
    this->player.location = redPlayer;
  }
  check_goal_tokens(Player_befor, this->player.location);
}

void Master::seeMyBoard()
{

  std::vector<std::pair<int, int>> redPlayer;
  std::vector<std::pair<int, int>> bluePlayer;
  send_request_board_state(redPlayer, bluePlayer);

  // std::vector<int,bool> index;

  bool index[robot.location.size()] = {
      false,
  };

  if (this->robot.color == "red") // robot이 red면 상대는 blue 는 2
  {
    // 인덱스 맞는거 잡고 안 맞으면 안 맞은 만큼 업뎃 해줘야 겠지...
    for (size_t i = 0; i < robot.location.size(); i++)
    {

      if (robot.location[i].first < 0)
      {
        // RCLCPP_ERROR(get_logger(),"!!in1 %d",j);
        index[i] = true;
        // break;
      }
      for (size_t j = 0; j < redPlayer.size(); j++)
      {

        RCLCPP_ERROR(get_logger(), "!!in1 i : %d l : %d :: %d :: %d", i, j, robot.location[i].first, redPlayer[j].first);

        if (redPlayer[j].first == robot.location[i].first)
        {
          //RCLCPP_ERROR(get_logger(),"!!in2 %d",j);
          index[i] = true;
          // break;
        }
        if(redPlayer[j].first == 27 && robot.location[i].first == 22){
          index[i] = true;

        }
      }
    }

    for (size_t i = 0; i < robot.location.size(); i++)
    {
      RCLCPP_ERROR(get_logger(), "!!index %d", index[i]);
    }
    // flase 되어있으면 삭제된거임
    int delete_tokens = 0;
    for (size_t i = 0; i < robot.location.size(); i++)
    {
      if (index[i] == false)
      {
        RCLCPP_ERROR(get_logger(), "delet index~~ %d", i);
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
          j = -5;
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

        if (robot.location[j].first < 0)
        {
          index[j] = true;
          break;
        }
        if (bluePlayer[i].first == robot.location[j].first)
        {
          index[j] = true;
          break;
        }
      }
    }
    // flase 되어있으면 삭제된거임
    size_t delete_tokens = 0;
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
      robot.location[i].second++;
      place_z = (robot.location[i].second - 1) * height; //
      overlap = true;
      break;
    }
  }

  if (!overlap)
  {
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
    if(current_yut == -1 && current_pos < 0)
    {

      new_pos[i] = 19;

    }
    else if (robot.short_cut_index[0] == current_pos) // 5
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

      if (20 <= current_pos && current_pos <= 24) // 지름길에서 나오는 경우
      {
        if (new_pos[i] > 24)
          new_pos[i] -= 10;
      }
    }
  }

  int max_tokens = -1;
  int min_left = 100;
  int index;
  bool Arrivalflag = false;

  // 1. 잡을 수 있는 말이 있는지 확인 (현재 위치에 상대방 말이 있는지)
  max_tokens = -1;
  index = -1;
  for (size_t i = 0; i < robot.location.size(); i++)
  {
    for (size_t k = 0; k < player.location.size(); ++k)
    {
      if (player.location[k].first == new_pos[i] || ((player.location[k].first == 27)&&(new_pos[i] == 22))) // 같은 위치에 상대 말이 있는지 확인
      {
        RCLCPP_INFO(this->get_logger(), "Captured opponent's piece at position %d!  %d", new_pos[i],max_tokens);

        // send_request_say_special_state(std::string("Write down what you want to say when you catch the opponent's token"));

        // player[j].pieces_location[k] = 0; // 상대 말 다시 시작 위치로

        if (max_tokens <= player.location[k].second) // 잡을 수 있는 것 중 토큰이 큰 것
        {
                          // update
          index = i;                                 // 움직일 말의 인덱스
          max_tokens = player.location[k].second;
          kill_tokens = player.location[k].second;
          next_pos = new_pos[i];
          kill_flag = true;
        }
      }
    }
  }

  if (kill_flag)
  {
    send_msg_say_board_state(KILL);
    pickAndPlace(new_pos[index], 0); // 겹치는 부분 치우는 부분
    return index;
  }

  // 2. 골 지점으로 들어올 수 있는 말이 있는지? 토큰큰게 있으면 그것부터
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

  if (Arrivalflag)
  {
    next_pos = 30;

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

  int result = -2;

  RCLCPP_INFO(this->get_logger(), "send_request_yut_res");
  // 서비스가 준비될 때까지 대기
  while (!client_yut->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_INFO(this->get_logger(), "send_request_yut_res Service not available, waiting...");
  }

  // 요청 메시지 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutnoriYutState::Request>();
  // 서비스 요청
  auto future = client_yut->async_send_request(request);

  // 응답 대기 및 처리
  if (rclcpp::spin_until_future_complete(get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_ERROR(this->get_logger(), "send_request_yut_res Service call successful.");
    auto response = future.get();
    result = response->yut_info;
    RCLCPP_ERROR(this->get_logger(), "rrrrrrrresult%d", result);
  }
  else
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to call service.");
  }

  return result;
}

// 윷 위치 알려달라고 요청 보내는 함수
void Master::send_request_yut_pos(float (&pos)[4], int &left)
{
  RCLCPP_ERROR(this->get_logger(), "send_request_yut_pos");
  // 서비스가 준비될 때까지 대기
  while (!client_yut_pos->wait_for_service(std::chrono::seconds(1)))
  {
    RCLCPP_INFO(this->get_logger(), "send_request_yut_pos Service not available, waiting...");
  }

  // 요청 메시지 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutPosition::Request>();
  // 서비스 요청
  auto future = client_yut_pos->async_send_request(request);

  // 응답 대기 및 처리
  if (rclcpp::spin_until_future_complete(get_node_base_interface(), future) == rclcpp::FutureReturnCode::SUCCESS)
  {
    RCLCPP_ERROR(this->get_logger(), "send_request_yut_pos Service call successful.");
    auto response = future.get();
    left = response->left_num;
    RCLCPP_ERROR(this->get_logger(), "LEFT : %d", response->left_num);
    RCLCPP_ERROR(this->get_logger(), "%f %f %f %f", response->pos[0], response->pos[1], response->pos[2], response->pos[3]);
    pos[0] = response->pos[0];
    pos[1] = response->pos[1];
    pos[2] = response->pos[2];
    pos[3] = response->pos[3];
  }
  else
  {
    RCLCPP_ERROR(this->get_logger(), "Failed to call service.");
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

        RCLCPP_INFO(this->get_logger(), "player 1 %d , %d", mapping(response->positions_p1[i]), response->tokens_p1[i]);
        player1.push_back(std::make_pair(mapping(response->positions_p1[i]), response->tokens_p1[i]));
      }
      for (size_t i = 0; i < response->positions_p2.size(); i++)
      {
        RCLCPP_INFO(this->get_logger(), "player 2 %d , %d", mapping(response->positions_p2[i]), response->tokens_p2[i]);
        player2.push_back(std::make_pair(mapping(response->positions_p2[i]), response->tokens_p2[i]));
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

void Master::send_request_yut_pick(float x, float y, float z, float r, int left)
{

  // 요청 객체 생성
  auto request = std::make_shared<xyz_interfaces::srv::YutPick::Request>();
  RCLCPP_WARN(this->get_logger(), "so %f %f %f %f", x, y, z, r);
  std::array<float, 4> pickpos = {x, y + (37.5) * 4, z, r};
  request->pick_pos = pickpos;
  request->place_num = 4 - left;

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
  RCLCPP_INFO(this->get_logger(), "send_request_next_turn.");
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
        turn_count++;
      }
    }
    else
    {
      // 서비스 호출 실패
      RCLCPP_ERROR(this->get_logger(), "Failed to call ssend_request_next_turn.");
    }
  }
}

void Master::send_msg_say_board_state(int state)
{

  auto message = xyz_interfaces::msg::SayBoardState();
  RCLCPP_INFO(this->get_logger(), "send_msg_say_board_state STATE :%d", state);

  // message.start_game = start_flag;
  // message.in_game = in_game_flag;
  // message.end_game = end_game_flag;

  message.turn = isMyTurn(robot); // 0이면 상대턴

  if (endGame())
  { // 게임 끝난 경우
    message.end_game = true;
  }
  else if (!start_flag) // 게임 시작 안 한 경우
  {
    message.start_game = true;
  }
  else
  {
    message.in_game = true;
  }

  switch (state)
  {
  case GAME_START:

    break;
  case END_GAME:

    break;
  case IN_GAME:

    break;
  case YUT_THROW:
    message.in_game = true;
    message.yut_throw_flag = true; // 내 턴에 윷을 던질 때 게임 결과에 따른 답변생성
    break;
  case MY_TURN_RES:
    message.in_game = true;
    message.my_turn_res_flag = true; // 게임 말을 옮긴 후 내 턴을 마칠 때 결과에 따른 답변 생성
    /* code */
    break;
  case BUTTON_PUSH:
    message.button_pushed = true; // 게임 도중 버튼을 눌러 대화를 시도할 때 flag
    break;
  case KILL:
    message.kill_flag = true;
    break;
  default:
    break;
  }

  int robotmaptokens_ = 0;
  RCLCPP_INFO(this->get_logger(), "robot.location.size() :%d", robot.location.size());
  for (size_t i = 0; i < robot.location.size(); i++)
  {
    RCLCPP_WARN(this->get_logger(), "robot.location. :%d", robot.location[i].first);
    message.positions_p1.push_back(robot.location[i].first);
    if (robot.location[i].first < 0)
    {
      robotmaptokens_++;
    }
  }
  message.robotgoaltokens = robot.finish_pieces;

  RCLCPP_WARN(this->get_logger(), "player.location.size(); :%d", player.location.size());
  for (size_t i = 0; i < player.location.size(); i++)
  {
    message.positions_p2.push_back(player.location[i].first);
  }
  message.playergoaltokens = player.finish_pieces;

  message.robotmaptokens = robotmaptokens_ - robot.finish_pieces;
  message.playermaptokens = 4 - player.location.size() - player.finish_pieces;

  Publisher_say_board_state->publish(message);
}

int Master::mapping(int input)
{

  auto it = vtom_coor.find(input);

  // 키가 존재하는지 확인
  if (it != vtom_coor.end())
  {
    // 키를 찾은 경우
    input = it->second;
    std::cout << "Key: " << it->first << ", Value: " << it->second << std::endl;
  }

  return input;
}

void Master::check_goal_tokens(std::vector<std::pair<int, int>> &befor, // pair <pos_index, token num>
                               std::vector<std::pair<int, int>> &after)
{
  int befor_num = 0;
  int after_num = 0;
  int goal_plus = 0;
  for (size_t i = 0; i < befor.size(); i++)
  {
    if (befor[i].first > 0)
      befor_num += befor[i].second;
  }
  for (size_t i = 0; i < after.size(); i++)
  {
    after_num += after[i].second;
  }

  RCLCPP_ERROR(get_logger(), "befor_num %d - kill_tokens%d - after_num %d  = goal_plus %d" ,befor_num,kill_tokens,after_num ,goal_plus);

  goal_plus = (befor_num - kill_tokens) - after_num;
  if (/*kill_flag == true && */ goal_plus > 0)
  {

    player.finish_pieces += goal_plus;
    RCLCPP_ERROR(get_logger(), "plus other !!!!!!!!!!!!goal~!!!!!!!!!! : %d", player.finish_pieces);
    RCLCPP_ERROR(get_logger(), "plus other !!!!!!!!!!!!goal~!!!!!!!!!! : %d", player.finish_pieces);
    RCLCPP_ERROR(get_logger(), "plus other !!!!!!!!!!!!goal~!!!!!!!!!! : %d", player.finish_pieces);
    RCLCPP_ERROR(get_logger(), "plus other !!!!!!!!!!!!goal~!!!!!!!!!! : %d", player.finish_pieces);
    RCLCPP_ERROR(get_logger(), "plus other !!!!!!!!!!!!goal~!!!!!!!!!! : %d", player.finish_pieces);
  }
  kill_tokens = 0;
  kill_flag = false;
}
