#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Team.hpp"
#include "model/Group.hpp"
#include "model/Tournament.hpp"


#include "controller/GroupController.hpp"
#include "delegate/IGroupDelegate.hpp"

using ::testing::_;
using ::testing::Return;

class MockGroupDelegate : public IGroupDelegate {
public:
  MOCK_METHOD((std::expected<std::string,int>), CreateGroup, (const std::string&, const Group&), (override));
  MOCK_METHOD((std::optional<Group>),           FindGroupById, (const std::string&, const std::string&), (override));
  MOCK_METHOD((std::vector<Group>),             ListGroups, (const std::string&), (override));
  MOCK_METHOD((std::expected<void,int>),        UpdateGroup, (const std::string&, const std::string&, const Group&), (override));
  MOCK_METHOD((std::expected<void,int>),        AddTeam, (const std::string&, const std::string&, const std::string&), (override));
};

//9 tests

TEST(GroupControllerTest, PostGroup_201_Location){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, CreateGroup("T1", _)).WillOnce(Return(std::expected<std::string,int>{"GA"}));
  std::string loc;
  EXPECT_EQ(201, c.PostGroup("T1", Group{"","A",0}, &loc));
  EXPECT_EQ("GA", loc);
}

TEST(GroupControllerTest, PostGroup_409_Conflict){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, CreateGroup("T1", _)).WillOnce(Return(std::unexpected{409}));
  EXPECT_EQ(409, c.PostGroup("T1", Group{}, nullptr));
}

TEST(GroupControllerTest, GetGroups_200_List){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, ListGroups("T1")).WillOnce(Return(std::vector<Group>{{"GA","A",0},{"GB","B",0}}));
  auto [code, list] = c.GetGroups("T1");
  EXPECT_EQ(200, code);
  EXPECT_EQ(2u, list.size());
}

TEST(GroupControllerTest, GetGroup_200_Object){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, FindGroupById("T1","GA")).WillOnce(Return(Group{"GA","A",4}));
  auto [code, obj] = c.GetGroup("T1","GA");
  EXPECT_EQ(200, code);
  ASSERT_TRUE(obj);
  EXPECT_EQ(4, obj->size);
}

TEST(GroupControllerTest, GetGroup_404_NotFound){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, FindGroupById("T1","NO")).WillOnce(Return(std::nullopt));
  auto [code, obj] = c.GetGroup("T1","NO");
  EXPECT_EQ(404, code);
  EXPECT_FALSE(obj);
}

TEST(GroupControllerTest, PatchGroup_204_Ok){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, UpdateGroup("T1","GA", _)).WillOnce(Return(std::expected<void,int>{}));
  EXPECT_EQ(204, c.PatchGroup("T1","GA", Group{"GA","A",4}));
}

TEST(GroupControllerTest, PatchGroup_404_NotFound){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, UpdateGroup("T1","GA", _)).WillOnce(Return(std::unexpected{404}));
  EXPECT_EQ(404, c.PatchGroup("T1","GA", Group{"GA","A",4}));
}

TEST(GroupControllerTest, AddTeam_204_Ok){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, AddTeam("T1","GA","TEAM-1")).WillOnce(Return(std::expected<void,int>{}));
  EXPECT_EQ(204, c.PostAddTeam("T1","GA","TEAM-1"));
}

TEST(GroupControllerTest, AddTeam_422_Validation){
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, AddTeam("T1","GA","X")).WillOnce(Return(std::unexpected{422}));
  EXPECT_EQ(422, c.PostAddTeam("T1","GA","X"));
}

//Tests adicionales de validación del Mundial
TEST(GroupControllerTest, Mundial_AddTeam_GroupFull_4Teams_Returns422){
  // Verifica que no se puede agregar un 5to equipo cuando el grupo ya tiene 4
  MockGroupDelegate mock; GroupController c(&mock);
  EXPECT_CALL(mock, AddTeam("T1","GA","TEAM-5")).WillOnce(Return(std::unexpected{422}));
  EXPECT_EQ(422, c.PostAddTeam("T1","GA","TEAM-5"));
}
