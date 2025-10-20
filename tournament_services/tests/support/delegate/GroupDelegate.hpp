#ifndef SUPPORT_GROUPDELEGATE_HPP
#define SUPPORT_GROUPDELEGATE_HPP

#include <expected>
#include <optional>
#include <vector>
#include <string>
#include "model/Group.hpp"

// Stub de GroupDelegate para tests - clase template simple
template<typename TournamentRepo, typename GroupRepo, typename TeamRepo, typename EventBus>
class GroupDelegateImpl {
private:
    TournamentRepo* tr_;
    GroupRepo* gr_;
    TeamRepo* tm_;
    EventBus* bus_;

public:
    GroupDelegateImpl(TournamentRepo& tr, GroupRepo& gr, TeamRepo& tm, EventBus& bus)
        : tr_(&tr), gr_(&gr), tm_(&tm), bus_(&bus) {}

    std::expected<std::string, int> Create(const std::string& tid, const Group& group) {
        if (!tr_->Exists(tid)) {
            return std::unexpected{404};
        }
        auto result = gr_->Insert(tid, group);
        if (result) {
            bus_->Publish("group-created", *result);
            return *result;
        }
        return std::unexpected{409};
    }

    std::optional<Group> FindById(const std::string& tid, const std::string& gid) {
        return gr_->FindById(tid, gid);
    }

    std::vector<Group> List(const std::string& tid) {
        return gr_->List(tid);
    }

    std::expected<void, int> Update(const std::string& tid, const std::string& gid, const Group& group) {
        if (gr_->Update(tid, gid, group)) {
            return {};
        }
        return std::unexpected{404};
    }

    std::expected<void, int> AddTeam(const std::string& tid, const std::string& gid, const std::string& teamId) {
        if (!tm_->Exists(teamId)) {
            return std::unexpected{422};
        }
        int currentSize = gr_->GroupSize(tid, gid);
        if (currentSize >= 4) {
            return std::unexpected{422};
        }
        if (gr_->AddTeam(tid, gid, teamId)) {
            bus_->Publish("team-added", teamId);
            return {};
        }
        return std::unexpected{409};
    }
};

// Helper function para crear GroupDelegate sin CTAD
template<typename TR, typename GR, typename TM, typename EB>
GroupDelegateImpl<TR, GR, TM, EB> makeGroupDelegate(TR& tr, GR& gr, TM& tm, EB& bus) {
    return GroupDelegateImpl<TR, GR, TM, EB>(tr, gr, tm, bus);
}

// Alias para compatibilidad - usa CTAD
template<typename TR, typename GR, typename TM, typename EB>
GroupDelegateImpl(TR&, GR&, TM&, EB&) -> GroupDelegateImpl<TR, GR, TM, EB>;

// Alias para uso en tests
#define GroupDelegate GroupDelegateImpl

#endif // SUPPORT_GROUPDELEGATE_HPP
