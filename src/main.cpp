#include <iostream>
#include "yutnori_master/master.hpp"


int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);

  // Master 노드 인스턴스 생성 및 실행
  auto master_node = std::make_shared<Master>();
  rclcpp::spin(master_node);

  rclcpp::shutdown();
  return 0;
}
