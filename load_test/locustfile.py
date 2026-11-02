"""Load-test the supported tournament APIs; no match/winner simulation."""

import logging
from argparse import ArgumentTypeError
from time import perf_counter
from uuid import UUID, uuid4

from locust import HttpUser, between, events, task
from locust.exception import StopUser


def positive_integer(value):
    number = int(value)
    if number < 1:
        raise ArgumentTypeError("must be a positive integer")
    return number


@events.init_command_line_parser.add_listener
def add_options(parser):
    parser.add_argument("--teams-per-tournament", type=positive_integer, default=8)
    parser.add_argument("--groups-per-tournament", type=positive_integer, default=2)


class ScenarioFailed(Exception):
    """A failed request or contract check; do not issue dependent requests."""


class TournamentUser(HttpUser):
    wait_time = between(1, 3)

    def on_start(self):
        options = self.environment.parsed_options
        self.team_count = getattr(options, "teams_per_tournament", 8)
        self.group_count = getattr(options, "groups_per_tournament", 2)
        if not 1 <= self.group_count <= self.team_count <= 32 * self.group_count:
            message = (
                f"Invalid tournament configuration: teams={self.team_count}, "
                f"groups={self.group_count}. Require positive counts, at least "
                "one team per group, and at most 32 teams per group. "
                "Try --teams-per-tournament 8 --groups-per-tournament 2 "
                "(or set these values in the Locust web UI)."
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
            self.run_tournament()
        except ScenarioFailed as failure:
            error = failure
        # An interrupted/killed user must not report an incomplete scenario as successful.
        self.environment.events.request.fire(
            request_type="SCENARIO", name="Tournament lifecycle",
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
                "numberOfGroups": self.group_count,
                "maxTeamsPerGroup": (self.team_count + self.group_count - 1) // self.group_count,
                "type": "ROUND_ROBIN",
            },
        })
        self.request(
            "GET", "/tournaments", "GET /tournaments",
            validate=self.list_validator([tournament_id]),
        )

        group_path = f"/tournaments/{tournament_id}/groups"
        group_ids = []
        for index in range(self.group_count):
            group_name = f"load-{run_id}-group-{index}"
            group_id = self.create(
                group_path, "POST /tournaments/{tournamentId}/groups", {"name": group_name},
            )
            group_ids.append(group_id)
            path = f"{group_path}/{group_id}"
            self.request(
                "GET", path, "GET /tournaments/{tournamentId}/groups/{groupId}",
                validate=self.object_validator({"id": group_id, "name": group_name}),
            )
            assigned = teams[index::self.group_count]
            self.request(
                "PATCH", f"{path}/teams",
                "PATCH /tournaments/{tournamentId}/groups/{groupId}/teams",
                status=204, payload=[{"id": team_id} for team_id in assigned],
            )

            def group_with_teams(response):
                document = self.object_validator({"id": group_id})(response)
                members = document.get("teams")
                if not isinstance(members, list) or any(not isinstance(t, dict) for t in members):
                    raise ValueError("Expected a teams array")
                actual = [team.get("id") for team in members]
                if len(actual) != len(assigned) or set(actual) != set(assigned):
                    raise ValueError("Group membership does not match assigned teams")

            self.request(
                "GET", path, "GET /tournaments/{tournamentId}/groups/{groupId}",
                validate=group_with_teams,
            )
        self.request(
            "GET", group_path, "GET /tournaments/{tournamentId}/groups",
            validate=self.list_validator(group_ids),
        )
        self.request("GET", "/metrics", "GET /metrics")
