#include "delegate/TeamDelegate.hpp"

#include <expected>
#include <iostream>
#include <regex>
#include <utility>
#include <pqxx/pqxx>

#include "exception/Error.hpp"

TeamDelegate::TeamDelegate(
    std::shared_ptr<IRepository<domain::Team, std::string_view>> repository)
    : teamRepository(std::move(repository)) {}

// Placeholder UUID-ish regex (replace with your real one)
static const std::regex ID_REGEX(R"(^[0-9a-fA-F]{8}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{4}\b-[0-9a-fA-F]{12}$)");

std::expected<std::vector<std::shared_ptr<domain::Team>>, Error>
TeamDelegate::GetAllTeams() {
  try {
    auto teams = teamRepository->ReadAll();
    return teams;

  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<std::shared_ptr<domain::Team>, Error> TeamDelegate::GetTeam(std::string_view id) {
  if (!std::regex_match(std::string{id}, ID_REGEX)) {
    return std::unexpected(Error::INVALID_FORMAT);
  }

  try {
    auto team = teamRepository->ReadById(id);
    if (!team) {
      return std::unexpected(Error::NOT_FOUND);
    }
    return team;

  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
      return std::unexpected(Error::INVALID_FORMAT);
    }

  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<std::string, Error> TeamDelegate::CreateTeam(
    const domain::Team& team) {
  if (!team.Id.empty() || team.Name.empty()) {
    return std::unexpected(Error::INVALID_FORMAT);
  }

  try {
    auto id_view = teamRepository->Create(team);
    if (id_view.empty()) {
      return std::unexpected(Error::UNKNOWN_ERROR);
    }
    return std::string{id_view};
    
  } catch (const pqxx::unique_violation& e) {
    if (e.sqlstate() == "23505") {
      return std::unexpected(Error::DUPLICATE);
    }
    return std::unexpected(Error::UNKNOWN_ERROR);

  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<std::string, Error> TeamDelegate::UpdateTeam(
    const domain::Team& team) {
  if (team.Id.empty() || !std::regex_match(team.Id, ID_REGEX)) {
    return std::unexpected(Error::INVALID_FORMAT);
  }

  try {
    auto updated_view = teamRepository->Update(team);
    if (updated_view.empty()) {
      return std::unexpected(Error::NOT_FOUND);
    }
    return std::string{updated_view};
  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
      return std::unexpected(Error::INVALID_FORMAT);
    }
    return std::unexpected(Error::UNKNOWN_ERROR);
  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<void, Error> TeamDelegate::DeleteTeam(std::string_view id) {
  try {
    teamRepository->Delete(id);
    return {};

  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
      return std::unexpected(Error::INVALID_FORMAT);
    }
    return std::unexpected(Error::UNKNOWN_ERROR);

  } catch (const std::exception&) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}