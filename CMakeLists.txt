cmake_minimum_required(VERSION 3.8)
project(yutnori_master)

# 컴파일러가 GCC 또는 Clang인 경우 컴파일 옵션 추가
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 필수 패키지 찾기
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)

# 노드 빌드 설정
add_executable(yutnori_master_node src/main.cpp)
ament_target_dependencies(yutnori_master_node rclcpp std_msgs std_srvs)

# 빌드된 실행 파일을 설치할 위치 설정
install(TARGETS
  yutnori_master_node
  DESTINATION lib/${PROJECT_NAME}
)

# 런치 파일이 있다면 설치 설정 (필요시 주석 해제)
# install(DIRECTORY launch
#   DESTINATION share/${PROJECT_NAME}/
# )

# 테스트 관련 설정
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)

  # 주석 처리된 두 줄은 라이선스 검사 무시
  # 라이선스 추가 후 주석 해제
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_copyright_FOUND TRUE)

  ament_lint_auto_find_test_dependencies()
endif()

# 패키지 설정 완료
ament_package()
