#include <set>

#include "gtest/gtest.h"

#include "lowering_family.h"

namespace {

TEST(MotionFamilyFactoryTest, RegistersExpectedFamilies) {
  const auto lowerers = gcode::createMotionFamilyLowerers();
  ASSERT_EQ(lowerers.size(), 3u);

  std::set<int> codes;
  for (const auto &lowerer : lowerers) {
    ASSERT_NE(lowerer, nullptr);
    codes.insert(lowerer->motionCode());
  }

  EXPECT_EQ(codes, (std::set<int>{1, 2, 3}));
}

} // namespace
