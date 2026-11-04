"""Offline contract tests: fake HTTP responses, no application traffic."""

from locustfile import ScenarioFailed, TournamentUser

import unittest
from itertools import combinations
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
        self.matches = {}
        self.polls = 0
        self.polls_before_schedule = 2
        self.never_schedule = False
        self.corrupt_standings = False
        self.bad_seeding = False

    def request(self, method, path, **options):
        self.requests.append((method, path, options))
        response = self.respond(method, path, options["json"])
        self.responses.append(response)
        return response

    def respond(self, method, path, payload):
        if path in ("/health", "/metrics"):
            return Response()
        path, _, query = path.partition("?")
        segments = path.strip("/").split("/")
        if len(segments) >= 3 and segments[2] in ("matches", "standings"):
            return self.respond_matches(method, segments, query, payload)
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
            if len(group["teams"]) == 32:
                self.matches[group["tournamentId"]] = {"group": group, "list": []}
            return Response(204)
        if len(segments) == 1:
            return Response(document=list(store.values()))
        if len(segments) == 3:
            return Response(document=[g for g in store.values() if g["tournamentId"] == segments[1]])
        return Response(document=store[segments[-1]])

    # Minimal stand-in for MatchDelegate: schedule, scores, standings, playoff rounds.
    def respond_matches(self, method, segments, query, payload):
        state = self.matches[segments[1]]
        matches = state["list"]
        if segments[2] == "standings":
            return Response(document=self.standings(state))
        if method == "PUT":
            match = next(m for m in matches if m["id"] == segments[3])
            match["score"] = payload
            self.advance(state)
            return Response(204)
        stage = query.removeprefix("stage=")
        if stage == "ROUND_ROBIN" and not matches:
            self.polls += 1
            if not self.never_schedule and self.polls > self.polls_before_schedule:
                team_ids = [t["id"] for t in state["group"]["teams"]]
                for number, (home, visitor) in enumerate(combinations(team_ids, 2), start=1):
                    matches.append(self.match("ROUND_ROBIN", 1, number, home, visitor))
        return Response(document=[m for m in matches if m["stage"] == stage])

    @staticmethod
    def match(stage, round_number, number, home, visitor, home_seed=None, visitor_seed=None):
        return {"id": str(uuid4()), "stage": stage, "round": round_number, "matchNumber": number,
                "homeTeamId": home, "visitorTeamId": visitor,
                "homeSeed": home_seed, "visitorSeed": visitor_seed, "score": None}

    def standings(self, state):
        team_ids = [t["id"] for t in state["group"]["teams"]]
        wins = dict.fromkeys(team_ids, 0)
        for m in state["list"]:
            if m["stage"] == "ROUND_ROBIN" and m["score"]:
                home_won = m["score"]["homeTeamScore"] > m["score"]["visitorTeamScore"]
                wins[m["homeTeamId"] if home_won else m["visitorTeamId"]] += 1
        ranked = sorted(team_ids, key=lambda t: (-wins[t], team_ids.index(t)))
        if self.corrupt_standings:
            ranked.reverse()
        last = len(team_ids) - 1
        return [{"rank": i + 1, "teamId": t, "played": last, "wins": wins[t]}
                for i, t in enumerate(ranked)]

    def advance(self, state):
        matches = state["list"]
        playoffs = [m for m in matches if m["stage"] == "PLAYOFF"]
        if not playoffs:
            if all(m["score"] for m in matches):
                seeds = [s["teamId"] for s in self.standings(state)][:8]
                if self.bad_seeding:
                    seeds.reverse()
                for number, (h, v) in enumerate([(1, 8), (4, 5), (2, 7), (3, 6)], start=1):
                    matches.append(self.match("PLAYOFF", 1, number, seeds[h - 1], seeds[v - 1], h, v))
            return
        last = max(m["round"] for m in playoffs)
        current = sorted((m for m in playoffs if m["round"] == last), key=lambda m: m["matchNumber"])
        if len(current) > 1 and all(m["score"] for m in current):
            def winner(m):
                home = m["score"]["homeTeamScore"] > m["score"]["visitorTeamScore"]
                return (m["homeTeamId"], m["homeSeed"]) if home else (m["visitorTeamId"], m["visitorSeed"])
            for i in range(0, len(current), 2):
                a, b = sorted((winner(current[i]), winner(current[i + 1])), key=lambda w: w[1])
                matches.append(self.match("PLAYOFF", last + 1, i // 2 + 1, a[0], b[0], a[1], b[1]))


class LoadTestTests(unittest.TestCase):
    def setUp(self):
        self.environment = Environment(
            host="http://offline.invalid",
            parsed_options=SimpleNamespace(
                teams_per_tournament=8, scenario="setup", schedule_timeout=120),
        )
        class OfflineTournamentUser(TournamentUser):
            host = "http://offline.invalid"

        self.user = OfflineTournamentUser(self.environment)
        self.client = ApiClient()
        self.user.client = self.client
        self.user.on_start()

    def test_complete_scenario_covers_implemented_routes_and_membership(self):
        self.user.run_tournament()
        self.assertEqual(26, len(self.client.requests))
        self.assertEqual(8, len(self.client.teams))
        self.assertEqual(1, len(self.client.tournaments))
        self.assertEqual(1, len(self.client.groups))
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

    def test_tournament_uses_single_group_of_32_format(self):
        self.user.run_tournament()
        payload = next(
            options["json"] for method, path, options in self.client.requests
            if method == "POST" and path == "/tournaments"
        )
        self.assertEqual(
            {"numberOfGroups": 1, "maxTeamsPerGroup": 32, "type": "ROUND_ROBIN"},
            payload["format"],
        )

    def test_full_group_assigns_all_32_teams(self):
        self.user.team_count = 32
        self.user.run_tournament()
        group = next(iter(self.client.groups.values()))
        self.assertEqual(32, len(group["teams"]))

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
        self.environment.parsed_options.teams_per_tournament = 33
        with self.assertLogs(level="ERROR"), self.assertRaisesRegex(StopUser, "teams=33"):
            self.user.on_start()
        self.assertEqual([], self.client.requests)

    def test_interrupted_scenario_does_not_report_success(self):
        samples = []
        self.environment.events.request.add_listener(lambda **sample: samples.append(sample))
        with patch.object(self.user, "run_tournament", side_effect=StopUser):
            with self.assertRaises(StopUser):
                self.user.tournament_lifecycle()
        self.assertEqual([], samples)


class FullScenarioTests(unittest.TestCase):
    def setUp(self):
        self.environment = Environment(
            host="http://offline.invalid",
            parsed_options=SimpleNamespace(
                teams_per_tournament=8, scenario="full", schedule_timeout=120),
        )

        class OfflineTournamentUser(TournamentUser):
            host = "http://offline.invalid"

        self.user = OfflineTournamentUser(self.environment)
        self.client = ApiClient()
        self.user.client = self.client
        self.user.on_start()
        sleeper = patch("locustfile.sleep")
        self.sleep = sleeper.start()
        self.addCleanup(sleeper.stop)
        self.samples = []
        self.environment.events.request.add_listener(lambda **sample: self.samples.append(sample))

    def requests_named(self, name):
        return [r for r in self.client.requests if r[2]["name"] == name]

    def test_full_scenario_plays_to_a_champion(self):
        self.user.tournament_lifecycle()

        self.assertEqual(32, self.user.team_count)
        self.assertTrue(all(response.error is None for response in self.client.responses))
        scores = self.requests_named("PUT /tournaments/{tournamentId}/matches/{matchId}/score")
        self.assertEqual(496 + 7, len(scores))
        self.assertEqual(1, len(self.requests_named("GET /tournaments/{tournamentId}/standings")))
        self.assertEqual(2, self.sleep.call_count)
        state = next(iter(self.client.matches.values()))
        final = next(m for m in state["list"] if m["stage"] == "PLAYOFF" and m["round"] == 3)
        self.assertEqual((1, 2), (final["homeSeed"], final["visitorSeed"]))
        self.assertGreater(final["score"]["homeTeamScore"], final["score"]["visitorTeamScore"])
        names = [(s["name"], s["exception"]) for s in self.samples]
        self.assertEqual(
            [("Round-robin schedule ready", None), ("Full tournament lifecycle", None)], names)

    def test_schedule_timeout_fails_scenario(self):
        self.client.never_schedule = True
        self.user.schedule_timeout = 0

        self.user.tournament_lifecycle()

        self.assertEqual([], self.requests_named("PUT /tournaments/{tournamentId}/matches/{matchId}/score"))
        self.assertEqual(["Round-robin schedule ready", "Full tournament lifecycle"],
                         [s["name"] for s in self.samples])
        self.assertTrue(all(isinstance(s["exception"], ScenarioFailed) for s in self.samples))

    def test_wrong_standings_fail_before_playoffs(self):
        self.client.corrupt_standings = True

        with self.assertRaises(ScenarioFailed):
            self.user.run_full_tournament()
        self.assertEqual("GET", self.client.requests[-1][0])
        self.assertIn("standing", self.client.responses[-1].error)

    def test_wrong_playoff_seeding_fails(self):
        self.client.bad_seeding = True

        with self.assertRaises(ScenarioFailed):
            self.user.run_full_tournament()
        self.assertIn("seeding", self.client.responses[-1].error)

    def test_full_scenario_ignores_team_count_option(self):
        self.environment.parsed_options.teams_per_tournament = 99
        self.user.on_start()
        self.assertEqual(32, self.user.team_count)


if __name__ == "__main__":
    unittest.main()
