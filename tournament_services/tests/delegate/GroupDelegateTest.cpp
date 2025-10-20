#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Team.hpp"
#include "model/Group.hpp"
#include "model/Tournament.hpp"

#include "delegate/GroupDelegate.hpp"

using ::testing::_;
using ::testing::Return;

class MockTournamentRepoForGroupDelegate {
public:
  MOCK_METHOD(bool, Exists, (const std::string& tid), ());
};

class MockGroupRepoForGroupDelegate {
public:
  MOCK_METHOD((std::optional<std::string>), Insert,   (const std::string& tid, const Group&), ());
  MOCK_METHOD((std::optional<Group>),       FindById, (const std::string& tid, const std::string& gid), ());
  MOCK_METHOD((std::vector<Group>),         List,     (const std::string& tid), ());
  MOCK_METHOD(bool,                         Update,   (const std::string& tid, const std::string& gid, const Group&), ());
  MOCK_METHOD(bool,                         AddTeam,  (const std::string& tid, const std::string& gid, const std::string& teamId), ());
  MOCK_METHOD(int,                          GroupSize,(const std::string& tid, const std::string& gid), ());
};

class MockTeamRepoForGroupDelegate {
public:
  MOCK_METHOD(bool, Exists, (const std::string& teamId), ());
};

class MockEventBusForGroupDelegate {
public:
  MOCK_METHOD(void, Publish, (const std::string& topic, const std::string& payload), ());
};

//12 tests

TEST(GroupDelegateTest, Create_Ok_PublishesEvent){
  MockTournamentRepoForGroupDelegate tr;
  MockGroupRepoForGroupDelegate gr;
  MockTeamRepoForGroupDelegate tm;
  MockEventBusForGroupDelegate bus;
  EXPECT_CALL(tr, Exists("T1")).WillOnce(Return(true));
  EXPECT_CALL(gr, Insert("T1", _)).WillOnce(Return(std::optional<std::string>{"GA"}));
  EXPECT_CALL(bus, Publish("group-created", _));
  GroupDelegate d(tr,gr,tm,bus);
  auto r = d.Create("T1", Group{"","A",0});
  ASSERT_TRUE(r); EXPECT_EQ("GA", *r);
}

TEST(GroupDelegateTest, Create_Conflict409){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tr, Exists("T1")).WillOnce(Return(true));
  EXPECT_CALL(gr, Insert("T1", _)).WillOnce(Return(std::nullopt));
  auto r = d.Create("T1", Group{"","A",0});
  ASSERT_FALSE(r); EXPECT_EQ(409, r.error());
}

TEST(GroupDelegateTest, Create_TournamentNotFound404){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tr, Exists("T1")).WillOnce(Return(false));
  auto r = d.Create("T1", Group{"","A",0});
  ASSERT_FALSE(r); EXPECT_EQ(404, r.error());
}

TEST(GroupDelegateTest, FindById_Ok){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(gr, FindById("T1","GA")).WillOnce(Return(Group{"GA","A",4}));
  auto g = d.FindById("T1","GA");
  ASSERT_TRUE(g); EXPECT_EQ(4, g->size);
}

TEST(GroupDelegateTest, FindById_Null){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(gr, FindById("T1","NO")).WillOnce(Return(std::nullopt));
  auto g = d.FindById("T1","NO");
  EXPECT_FALSE(g.has_value());
}

TEST(GroupDelegateTest, List_Groups){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(gr, List("T1")).WillOnce(Return(std::vector<Group>{{"GA","A",0},{"GB","B",0}}));
  auto v = d.List("T1");
  EXPECT_EQ(2u, v.size());
}

TEST(GroupDelegateTest, Update_Ok){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(gr, Update("T1","GA", _)).WillOnce(Return(true));
  auto r = d.Update("T1","GA", Group{"GA","A",4});
  EXPECT_TRUE(r.has_value());
}

TEST(GroupDelegateTest, Update_NotFound404){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(gr, Update("T1","GA", _)).WillOnce(Return(false));
  auto r = d.Update("T1","GA", Group{"GA","A",4});
  ASSERT_FALSE(r); EXPECT_EQ(404, r.error());
}

TEST(GroupDelegateTest, AddTeam_Ok_Publishes){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("TEAM-1")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(3));
  EXPECT_CALL(gr, AddTeam("T1","GA","TEAM-1")).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("team-added", _));
  auto r = d.AddTeam("T1","GA","TEAM-1");
  EXPECT_TRUE(r.has_value());
}

TEST(GroupDelegateTest, AddTeam_TeamNotFound422){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("X")).WillOnce(Return(false));
  auto r = d.AddTeam("T1","GA","X");
  ASSERT_FALSE(r); EXPECT_EQ(422, r.error());
}

TEST(GroupDelegateTest, AddTeam_GroupFull422){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("T")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(4));
  auto r = d.AddTeam("T1","GA","T");
  ASSERT_FALSE(r); EXPECT_EQ(422, r.error());
}

TEST(GroupDelegateTest, AddTeam_Conflict409){
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("T")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(2));
  EXPECT_CALL(gr, AddTeam("T1","GA","T")).WillOnce(Return(false));
  auto r = d.AddTeam("T1","GA","T");
  ASSERT_FALSE(r); EXPECT_EQ(409, r.error());
}

// ---- Tests adicionales de validación del Mundial ----

TEST(GroupDelegateTest, Mundial_AddTeam_Exactly4Teams_GroupFull){
  // Verifica que con exactamente 4 equipos el grupo esté lleno
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("T5")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(4));  // Ya tiene 4 equipos
  auto r = d.AddTeam("T1","GA","T5");
  ASSERT_FALSE(r);
  EXPECT_EQ(422, r.error());  // Grupo lleno con 4 equipos
}

TEST(GroupDelegateTest, Mundial_AddTeam_3Teams_CanAddFourth){
  // Verifica que con 3 equipos se puede agregar el cuarto
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("T4")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(3));  // Tiene 3, puede agregar el 4to
  EXPECT_CALL(gr, AddTeam("T1","GA","T4")).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("team-added", _));
  auto r = d.AddTeam("T1","GA","T4");
  EXPECT_TRUE(r.has_value());
}

TEST(GroupDelegateTest, Mundial_Group_MinimumSize_0){
  // Verifica que un grupo puede empezar vacío (tamaño 0)
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("T1")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(0));  // Grupo vacío
  EXPECT_CALL(gr, AddTeam("T1","GA","T1")).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("team-added", _));
  auto r = d.AddTeam("T1","GA","T1");
  EXPECT_TRUE(r.has_value());
}

TEST(GroupDelegateTest, Mundial_EventPublished_ContainsTeamInfo){
  // Verifica que el evento publicado contiene información del equipo agregado
  MockTournamentRepoForGroupDelegate tr; MockGroupRepoForGroupDelegate gr; MockTeamRepoForGroupDelegate tm; MockEventBusForGroupDelegate bus;
  GroupDelegate d(tr,gr,tm,bus);
  EXPECT_CALL(tm, Exists("TEAM-BRASIL")).WillOnce(Return(true));
  EXPECT_CALL(gr, GroupSize("T1","GA")).WillOnce(Return(2));
  EXPECT_CALL(gr, AddTeam("T1","GA","TEAM-BRASIL")).WillOnce(Return(true));
  EXPECT_CALL(bus, Publish("team-added", _));  // El evento debe contener info del equipo
  auto r = d.AddTeam("T1","GA","TEAM-BRASIL");
  EXPECT_TRUE(r.has_value());
}
