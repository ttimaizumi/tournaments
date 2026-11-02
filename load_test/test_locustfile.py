"""Offline contract tests: fake HTTP responses, no application traffic."""

from locustfile import ScenarioFailed, TournamentUser

import unittest
from types import SimpleNamespace
from unittest.mock import patch
from uuid import uuid4

from locust.env import Environment
from locust.exception import StopUser


class Response:
    def __init__(self, status=200, document=None, location=None):
        self.status_code = status
        self.document = document
        self.headers = {"Location": location} if location is not None else {}
        self.error = None

    def __enter__(self):
        return self

    def __exit__(self, *args):
        return False

    def json(self):
        return self.document

    def failure(self, error):
        self.error = error


class ApiClient:
    def __init__(self):
        self.teams = {}
        self.tournaments = {}
        self.groups = {}
        self.requests = []
        self.responses = []
        self.fail_patch = False
        self.invalid_location = False
        self.corrupt_membership = False

    def request(self, method, path, **options):
        self.requests.append((method, path, options))
        response = self.respond(method, path, options["json"])
        self.responses.append(response)
        return response

    def respond(self, method, path, payload):
        if path in ("/health", "/metrics"):
            return Response()
        segments = path.strip("/").split("/")
        store = self.teams if segments[0] == "teams" else self.tournaments
        if len(segments) >= 3:
            store = self.groups
        if method == "POST":
            identifier = str(uuid4())
            document = {**payload, "id": identifier}
            if store is self.groups:
                document.update(tournamentId=segments[1], teams=[])
            store[identifier] = document
            location = "invalid" if self.invalid_location else identifier
            return Response(201, location=location)
        if method == "PATCH":
            if self.fail_patch:
                return Response(500)
            group = self.groups[segments[3]]
            group["teams"] = [] if self.corrupt_membership else [self.teams[t["id"]] for t in payload]
            return Response(204)
        if len(segments) == 1:
            return Response(document=list(store.values()))
        if len(segments) == 3:
            return Response(document=[g for g in store.values() if g["tournamentId"] == segments[1]])
        return Response(document=store[segments[-1]])


class LoadTestTests(unittest.TestCase):
    def setUp(self):
        self.environment = Environment(
            host="http://offline.invalid",
            parsed_options=SimpleNamespace(teams_per_tournament=8, groups_per_tournament=2),
        )
        class OfflineTournamentUser(TournamentUser):
            host = "http://offline.invalid"

        self.user = OfflineTournamentUser(self.environment)
        self.client = ApiClient()
        self.user.client = self.client
        self.user.on_start()

    def test_complete_scenario_covers_implemented_routes_and_membership(self):
        self.user.run_tournament()
        self.assertEqual(30, len(self.client.requests))
        self.assertEqual(8, len(self.client.teams))
        self.assertEqual(1, len(self.client.tournaments))
        self.assertEqual(2, len(self.client.groups))
        assigned = [t["id"] for g in self.client.groups.values() for t in g["teams"]]
        self.assertCountEqual(self.client.teams, assigned)
        names = {options["name"] for _, _, options in self.client.requests}
        self.assertEqual(11, len(names))
        self.assertTrue(all(response.error is None for response in self.client.responses))
        self.assertTrue(all(options["timeout"] == 30 for _, _, options in self.client.requests))

    def test_scenarios_have_unique_resource_names(self):
        self.user.run_tournament()
        self.user.run_tournament()
        names = [t["name"] for t in self.client.teams.values()]
        self.assertEqual(16, len(set(names)))

    def test_uneven_groups_assign_every_team_once(self):
        self.user.team_count = 5
        self.user.run_tournament()
        self.assertEqual([3, 2], [len(g["teams"]) for g in self.client.groups.values()])

    def test_invalid_location_aborts_before_dependent_requests(self):
        self.client.invalid_location = True
        with self.assertRaises(ScenarioFailed):
            self.user.run_tournament()
        self.assertEqual(2, len(self.client.requests))
        self.assertIsNotNone(self.client.responses[-1].error)

    def test_failed_assignment_aborts_and_records_scenario_failure(self):
        self.client.fail_patch = True
        samples = []
        self.environment.events.request.add_listener(lambda **sample: samples.append(sample))
        self.user.tournament_lifecycle()
        self.assertEqual("PATCH", self.client.requests[-1][0])
        self.assertEqual(1, len(samples))
        self.assertIsInstance(samples[0]["exception"], ScenarioFailed)

    def test_corrupt_membership_is_a_failed_request(self):
        self.client.corrupt_membership = True
        with self.assertRaises(ScenarioFailed):
            self.user.run_tournament()
        self.assertIn("membership", self.client.responses[-1].error)

    def test_completed_scenario_reports_success(self):
        samples = []
        self.environment.events.request.add_listener(lambda **sample: samples.append(sample))
        self.user.tournament_lifecycle()
        self.assertIsNone(samples[0]["exception"])
        self.assertEqual("SCENARIO", samples[0]["request_type"])

    def test_invalid_group_capacity_stops_user(self):
        self.environment.parsed_options.teams_per_tournament = 65
        with self.assertLogs(level="ERROR"), self.assertRaisesRegex(StopUser, "teams=65, groups=2"):
            self.user.on_start()
        self.assertEqual([], self.client.requests)

    def test_interrupted_scenario_does_not_report_success(self):
        samples = []
        self.environment.events.request.add_listener(lambda **sample: samples.append(sample))
        with patch.object(self.user, "run_tournament", side_effect=StopUser):
            with self.assertRaises(StopUser):
                self.user.tournament_lifecycle()
        self.assertEqual([], samples)


if __name__ == "__main__":
    unittest.main()
