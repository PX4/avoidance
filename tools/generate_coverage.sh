#!/bin/bash
catkin clean -y
catkin config --cmake-args -DCMAKE_BUILD_TYPE=Coverage -DCATKIN_ENABLE_TESTING=True
catkin build avoidance local_planner global_planner safe_landing_planner
catkin build avoidance --no-deps --catkin-make-args avoidance-test_coverage
catkin build local_planner --no-deps --catkin-make-args local_planner-test_coverage
catkin build global_planner --no-deps --catkin-make-args global_planner-test_coverage
catkin build safe_landing_planner --no-deps --catkin-make-args safe_landing_planner-test_coverage
lcov -a ../../build/avoidance/coverage.info -a ../../build/local_planner/coverage.info -a ../../build/global_planner/coverage.info -a ../..build/safe_landing_planner/coverage.info -o repo_total.info --rc lcov_branch_coverage=1

