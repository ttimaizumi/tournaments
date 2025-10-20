#include "delegate/TournamentDelegate.hpp"

#include <expected>
#include <iostream>
#include <regex>
#include <utility>
#include <pqxx/pqxx>

#include "exception/Error.hpp"

TournamentDelegate::TournamentDelegate(
    std::shared_ptr<IRepository<domain::Tournament, std::string>> repository)
    : tournamentRepository(std::move(repository)) {}

std::expected<std::vector<std::shared_ptr<domain::Tournament>>, Error>
TournamentDelegate::ReadAll() {
  try {
    auto tournaments = tournamentRepository->ReadAll();
    return tournaments;

  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<std::shared_ptr<domain::Tournament>, Error> TournamentDelegate::GetTournament(std::string_view id) {

  try {
    auto tournament = tournamentRepository->ReadById(std::string(id));
    if (!tournament) {
      return std::unexpected(Error::NOT_FOUND);
    }
    return tournament;

  } catch (const pqxx::data_exception& e) {
    if (e.sqlstate() == "22P02") {
      return std::unexpected(Error::INVALID_FORMAT);
    }
    return std::unexpected(Error::UNKNOWN_ERROR);

  } catch (const std::exception& e) {
    return std::unexpected(Error::UNKNOWN_ERROR);
  }
}

std::expected<std::string, Error> TournamentDelegate::CreateTournament(
    const domain::Tournament& tournament) {
  if (!tournament.Id().empty() || tournament.Name().empty()) {
    return std::unexpected(Error::INVALID_FORMAT);
  }

  try {
    auto id_view = tournamentRepository->Create(tournament);
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

std::expected<std::string, Error> TournamentDelegate::UpdateTournament(
    const domain::Tournament& tournament) {

  try {
    auto updated_view = tournamentRepository->Update(tournament);
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

std::expected<void, Error> TournamentDelegate::DeleteTournament(std::string_view id) {
  try {
    tournamentRepository->Delete(std::string(id));
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