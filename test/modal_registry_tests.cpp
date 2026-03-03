#include "gtest/gtest.h"

#include "modal_registry.h"

namespace {

TEST(ModalRegistryTest, PersistentGroupStateSurvivesBlockBoundary) {
  gcode::ModalRegistry registry;

  ASSERT_TRUE(registry.apply({gcode::ModalGroup::Dimensions, "G90"}));
  ASSERT_TRUE(registry.hasActiveCode(gcode::ModalGroup::Dimensions));

  registry.beginBlock();
  const auto active = registry.activeCode(gcode::ModalGroup::Dimensions);
  ASSERT_TRUE(active.has_value());
  EXPECT_EQ(*active, "G90");
}

TEST(ModalRegistryTest, Group11IsBlockScoped) {
  gcode::ModalRegistry registry;

  ASSERT_TRUE(registry.apply({gcode::ModalGroup::ExactStopOneShot, "G9"}));
  ASSERT_TRUE(registry.hasActiveCode(gcode::ModalGroup::ExactStopOneShot));

  registry.beginBlock();
  EXPECT_FALSE(registry.hasActiveCode(gcode::ModalGroup::ExactStopOneShot));
}

TEST(ModalRegistryTest, ConflictInSameGroupWithinOneBlockIsErrorByDefault) {
  gcode::ModalRegistry registry;
  std::optional<std::string> error;

  ASSERT_TRUE(registry.apply({gcode::ModalGroup::Dimensions, "G90"}));
  EXPECT_FALSE(registry.apply({gcode::ModalGroup::Dimensions, "G91"}, &error));
  ASSERT_TRUE(error.has_value());
  EXPECT_NE(error->find("modal conflict"), std::string::npos);
}

TEST(ModalRegistryTest, RepeatingSameCodeInBlockIsAllowed) {
  gcode::ModalRegistry registry;

  ASSERT_TRUE(registry.apply({gcode::ModalGroup::Plane, "G17"}));
  EXPECT_TRUE(registry.apply({gcode::ModalGroup::Plane, "G17"}));

  const auto active = registry.activeCode(gcode::ModalGroup::Plane);
  ASSERT_TRUE(active.has_value());
  EXPECT_EQ(*active, "G17");
}

} // namespace
