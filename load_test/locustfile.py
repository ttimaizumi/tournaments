"""Load-test the tournament APIs.

The default ``setup`` scenario creates teams, a tournament and its group. The ``full``
scenario fills the group with 32 teams, waits for the consumer to generate the round
robin, scores every match, checks standings, and plays the playoffs through to a final.
"""

import logging
from argparse import ArgumentTypeError
from time import perf_counter
from uuid import UUID, uuid4

from gevent import sleep
from locust import HttpUser, between, events, task
from locust.exception import StopUser


def positive_integer(value):
    number = int(value)
    if number < 1:
        raise ArgumentTypeError("must be a positive integer")
    return number


@events.init_command_line_parser.add_listener
def add_options(parser):
    parser.add_argument("--teams-per-tournament", type=positive_integer, default=8,
                        help="Teams per tournament in the setup scenario (1-32)")
    parser.add_argument("--scenario", choices=SCENARIOS, default="setup",
                        help="setup: create and fill the group; full: also play the "
                             "whole tournament with 32 teams")
    parser.add_argument("--schedule-timeout", type=positive_integer, default=120,
                        help="Seconds the full scenario waits for the round robin")


SCENARIOS = ("setup", "full")

# The API accepts only this format: one group that holds exactly 32 teams.
NUMBER_OF_GROUPS = 1
MAX_TEAMS_PER_GROUP = 32
ROUND_ROBIN_MATCHES = MAX_TEAMS_PER_GROUP * (MAX_TEAMS_PER_GROUP - 1) // 2
# Server default for TournamentFormat.PlayoffTeams; seeds play in standard bracket order.
PLAYOFF_TEAMS = 8
FIRST_ROUND_SEEDS = [(1, 8), (4, 5), (2, 7), (3, 6)]
SCHEDULE_POLL_SECONDS = 1

MATCHES_NAME = "GET /tournaments/{tournamentId}/matches"
SCORE_NAME = "PUT /tournaments/{tournamentId}/matches/{matchId}/score"
STANDINGS_NAME = "GET /tournaments/{tournamentId}/standings"


class ScenarioFailed(Exception):
    """A failed request or contract check; do not issue dependent requests."""


class TournamentUser(HttpUser):
    wait_time = between(1, 3)

    def on_start(self):
        options = self.environment.parsed_options
        self.scenario = getattr(options, "scenario", "setup")
        self.schedule_timeout = getattr(options, "schedule_timeout", 120)
        self.team_count = getattr(options, "teams_per_tournament", 8)
        if self.scenario == "full":
            # Matches are only generated for a full group, so --teams-per-tournament is ignored.
            self.team_count = MAX_TEAMS_PER_GROUP
        elif not 1 <= self.team_count <= MAX_TEAMS_PER_GROUP:
            message = (
                f"Invalid tournament configuration: teams={self.team_count}. "
                f"Require between 1 and {MAX_TEAMS_PER_GROUP} teams (one group). "
                "Try --teams-per-tournament 8 "
                "(or set this value in the Locust web UI)."
            )
            logging.error(message)
            raise StopUser(message)

    def request(self, method, path, name, status=200, payload=None, validate=None):
        with self.client.request(
            method, path, name=name, json=payload, timeout=30,
            catch_response=True, allow_redirects=False,
        ) as response:
            try:
                if response.status_code != status:
                    raise ValueError(f"Expected HTTP {status}, got {response.status_code}")
                return validate(response) if validate else None
            except (ValueError, TypeError, KeyError) as error:
                response.failure(str(error))
                raise ScenarioFailed(f"{name}: {error}") from error

    def create(self, path, name, payload):
        def identifier(response):
            # The API returns a bare UUID, not a URL or a JSON body.
            return str(UUID(response.headers.get("Location", "")))

        return self.request("POST", path, name, 201, payload, identifier)

    @staticmethod
    def object_validator(expected):
        def validate(response):
            document = response.json()
            if not isinstance(document, dict):
                raise ValueError("Expected a JSON object")
            for key, value in expected.items():
                if document.get(key) != value:
                    raise ValueError(f"Unexpected {key} in response")
            return document

        return validate

    @staticmethod
    def list_validator(expected_ids):
        def validate(response):
            documents = response.json()
            if not isinstance(documents, list) or any(
                not isinstance(document, dict) for document in documents
            ):
                raise ValueError("Expected a JSON array of objects")
            actual = {document.get("id") for document in documents}
            if not set(expected_ids).issubset(actual):
                raise ValueError("Created resources missing from listing")
            return documents

        return validate

    @task
    def tournament_lifecycle(self):
        started = perf_counter()
        error = None
        try:
            if self.scenario == "full":
                self.run_full_tournament()
            else:
                self.run_tournament()
        except ScenarioFailed as failure:
            error = failure
        # An interrupted/killed user must not report an incomplete scenario as successful.
        name = "Full tournament lifecycle" if self.scenario == "full" else "Tournament lifecycle"
        self.environment.events.request.fire(
            request_type="SCENARIO", name=name,
            response_time=(perf_counter() - started) * 1000,
            response_length=0, exception=error, context=self.context(),
        )

    def run_tournament(self):
        self.request("GET", "/health", "GET /health")
        run_id = uuid4().hex
        teams = []
        for index in range(self.team_count):
            name = f"load-{run_id}-team-{index}"
            team_id = self.create("/teams", "POST /teams", {"name": name})
            teams.append(team_id)
            self.request(
                "GET", f"/teams/{team_id}", "GET /teams/{teamId}",
                validate=self.object_validator({"id": team_id, "name": name}),
            )
        self.request("GET", "/teams", "GET /teams", validate=self.list_validator(teams))

        tournament_name = f"load-{run_id}"
        tournament_id = self.create("/tournaments", "POST /tournaments", {
            "name": tournament_name,
            "format": {
                "numberOfGroups": NUMBER_OF_GROUPS,
                "maxTeamsPerGroup": MAX_TEAMS_PER_GROUP,
                "type": "ROUND_ROBIN",
            },
        })
        self.request(
            "GET", "/tournaments", "GET /tournaments",
            validate=self.list_validator([tournament_id]),
        )

        group_path = f"/tournaments/{tournament_id}/groups"
        group_name = f"load-{run_id}-group"
        group_id = self.create(
            group_path, "POST /tournaments/{tournamentId}/groups", {"name": group_name},
        )
        path = f"{group_path}/{group_id}"
        self.request(
            "GET", path, "GET /tournaments/{tournamentId}/groups/{groupId}",
            validate=self.object_validator({"id": group_id, "name": group_name}),
        )
        self.request(
            "PATCH", f"{path}/teams",
            "PATCH /tournaments/{tournamentId}/groups/{groupId}/teams",
            status=204, payload=[{"id": team_id} for team_id in teams],
        )

        def group_with_teams(response):
            document = self.object_validator({"id": group_id})(response)
            members = document.get("teams")
            if not isinstance(members, list) or any(not isinstance(t, dict) for t in members):
                raise ValueError("Expected a teams array")
            actual = [team.get("id") for team in members]
            if len(actual) != len(teams) or set(actual) != set(teams):
                raise ValueError("Group membership does not match assigned teams")

        self.request(
            "GET", path, "GET /tournaments/{tournamentId}/groups/{groupId}",
            validate=group_with_teams,
        )
        self.request(
            "GET", group_path, "GET /tournaments/{tournamentId}/groups",
            validate=self.list_validator([group_id]),
        )
        self.request("GET", "/metrics", "GET /metrics")
        return tournament_id, teams

    def run_full_tournament(self):
        tournament_id, teams = self.run_tournament()
        base = f"/tournaments/{tournament_id}"
        round_robin = self.wait_for_schedule(base, teams)

        # The team created first beats every later one, so the final table is fully predictable.
        rank = {team_id: index for index, team_id in enumerate(teams)}
        for match in round_robin:
            self.score(base, match, home_wins=rank[match["homeTeamId"]] < rank[match["visitorTeamId"]])
        self.request("GET", f"{base}/standings", STANDINGS_NAME, validate=self.standings_validator(teams))

        round_number, size = 1, PLAYOFF_TEAMS // 2
        while size >= 1:
            current = self.request(
                "GET", f"{base}/matches?stage=PLAYOFF", MATCHES_NAME,
                validate=self.playoff_round_validator(teams, round_number, size),
            )
            for match in current:
                # Every playoff is won by the better seed, so seed 1 (teams[0]) is champion.
                self.score(base, match, home_wins=match["homeSeed"] < match["visitorSeed"])
            round_number, size = round_number + 1, size // 2

    def wait_for_schedule(self, base, teams):
        """Polls until the consumer has generated the round robin; reports the wait as a sample."""
        started = perf_counter()
        try:
            while True:
                matches = self.request(
                    "GET", f"{base}/matches?stage=ROUND_ROBIN", MATCHES_NAME,
                    validate=self.match_list_validator(),
                )
                if matches:
                    # Generation is one INSERT, so the schedule appears all at once.
                    self.check_round_robin(matches, teams)
                    break
                if perf_counter() - started >= self.schedule_timeout:
                    raise ScenarioFailed(
                        f"Round robin not generated within {self.schedule_timeout}s; "
                        "check the consumer and ActiveMQ")
                sleep(SCHEDULE_POLL_SECONDS)
        except ScenarioFailed as failure:
            self.report_schedule_wait(started, failure)
            raise
        self.report_schedule_wait(started, None)
        return matches

    def report_schedule_wait(self, started, error):
        self.environment.events.request.fire(
            request_type="SCENARIO", name="Round-robin schedule ready",
            response_time=(perf_counter() - started) * 1000,
            response_length=0, exception=error, context=self.context(),
        )

    @staticmethod
    def check_round_robin(matches, teams):
        pairs = {frozenset((m["homeTeamId"], m["visitorTeamId"])) for m in matches}
        if len(matches) != ROUND_ROBIN_MATCHES or len(pairs) != ROUND_ROBIN_MATCHES:
            raise ScenarioFailed(
                f"Expected {ROUND_ROBIN_MATCHES} unique round-robin matches, got {len(matches)}")
        if set().union(*pairs) != set(teams):
            raise ScenarioFailed("Round-robin teams do not match the group")

    def score(self, base, match, home_wins):
        winner, loser = 3, 1
        payload = {
            "homeTeamScore": winner if home_wins else loser,
            "visitorTeamScore": loser if home_wins else winner,
        }
        self.request(
            "PUT", f"{base}/matches/{match['id']}/score", SCORE_NAME,
            status=204, payload=payload,
        )

    @staticmethod
    def match_list_validator():
        def validate(response):
            matches = response.json()
            if not isinstance(matches, list) or any(not isinstance(m, dict) for m in matches):
                raise ValueError("Expected a JSON array of matches")
            for match in matches:
                UUID(str(match["id"]))
                for key in ("homeTeamId", "visitorTeamId", "stage", "round", "matchNumber"):
                    if key not in match:
                        raise ValueError(f"Match missing {key}")
            return matches

        return validate

    @staticmethod
    def standings_validator(teams):
        last = len(teams) - 1

        def validate(response):
            standings = response.json()
            if not isinstance(standings, list) or len(standings) != len(teams):
                raise ValueError(f"Expected {len(teams)} standings")
            for index, (standing, team_id) in enumerate(zip(standings, teams)):
                actual = (standing.get("rank"), standing.get("teamId"),
                          standing.get("played"), standing.get("wins"))
                if actual != (index + 1, team_id, last, last - index):
                    raise ValueError(f"Unexpected standing at rank {index + 1}")
            return standings

        return validate

    def playoff_round_validator(self, teams, round_number, size):
        list_validator = self.match_list_validator()

        def validate(response):
            matches = [m for m in list_validator(response) if m["round"] == round_number]
            matches.sort(key=lambda m: m["matchNumber"])
            if len(matches) != size:
                raise ValueError(f"Expected {size} playoff matches in round {round_number}")
            if any(m.get("score") is not None for m in matches):
                raise ValueError(f"Playoff round {round_number} already has scores")
            if round_number == 1:
                expected = [(teams[h - 1], teams[v - 1]) for h, v in FIRST_ROUND_SEEDS]
                actual = [(m["homeTeamId"], m["visitorTeamId"]) for m in matches]
                if actual != expected:
                    raise ValueError("Playoff seeding does not match standings")
            return matches

        return validate
