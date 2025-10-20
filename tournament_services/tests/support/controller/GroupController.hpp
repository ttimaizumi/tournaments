#ifndef SUPPORT_GROUPCONTROLLER_HPP
#define SUPPORT_GROUPCONTROLLER_HPP

#include <utility>
#include <optional>
#include <vector>
#include <string>
#include "model/Group.hpp"
#include "delegate/IGroupDelegate.hpp"

// Stub de GroupController para tests
class GroupController {
public:
    explicit GroupController(IGroupDelegate* delegate) : delegate_(delegate) {}

    int PostGroup(const std::string& tid, const Group& group, std::string* location) {
        auto result = delegate_->CreateGroup(tid, group);
        if (result) {
            if (location) {
                *location = *result;
            }
            return 201;
        }
        return result.error();
    }

    std::pair<int, std::vector<Group>> GetGroups(const std::string& tid) {
        return {200, delegate_->ListGroups(tid)};
    }

    std::pair<int, std::optional<Group>> GetGroup(const std::string& tid, const std::string& gid) {
        auto result = delegate_->FindGroupById(tid, gid);
        if (result) {
            return {200, *result};
        }
        return {404, std::nullopt};
    }

    int PatchGroup(const std::string& tid, const std::string& gid, const Group& group) {
        auto result = delegate_->UpdateGroup(tid, gid, group);
        if (result) {
            return 204;
        }
        return result.error();
    }

    int PostAddTeam(const std::string& tid, const std::string& gid, const std::string& teamId) {
        auto result = delegate_->AddTeam(tid, gid, teamId);
        if (result) {
            return 204;
        }
        return result.error();
    }

private:
    IGroupDelegate* delegate_;
};

#endif