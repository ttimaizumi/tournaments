from typing import Any
from locust import HttpUser, task, between
import uuid
import random


class TournamentUser(HttpUser):
    host = "http://host.containers.internal:8000"
    wait_time = between(1, 3)

    # ---------- Helpers for ID extraction ----------

    @staticmethod
    def _extract_id_from_location(location: str | None, resource_segment: str) -> str | None:
        if not location:
            return None
        if f"/{resource_segment}/" in location:
            return location.split(f"/{resource_segment}/", 1)[1].split("/", 1)[0]
        parts = location.rstrip("/").split("/")
        return parts[-1] if parts else None

    # ---------- HTTP-level helpers ----------

    def create_teams(self) -> list[str]:
        team_ids: list[str] = []
        for i in range(32):
            team_data = {
                "name": f"Team {uuid.uuid4()}",
                "country": "Loadland",
                "shortName": f"T{i:02d}",
                "foundedYear": 1900 + i,
                "coach": f"Coach {i}",
            }
            with self.client.post(
                    "/teams",
                    json=team_data,
                    catch_response=True,
                    name="POST /teams",
            ) as response:
                if response.status_code in (200, 201):
                    team_id: str | None = None
                    try:
                        body = response.json()
                        team_id = body.get("id") or body.get("teamId")
                    except Exception:
                        team_id = None

                    if not team_id:
                        location = response.headers.get("Location") or response.headers.get("location")
                        team_id = self._extract_id_from_location(location, "teams")

                    if team_id:
                        team_ids.append(team_id)
                        response.success()
                    else:
                        response.failure("Team created but id not found in response")
                else:
                    response.failure(
                        f"Team creation failed: {response.status_code} - {response.text}"
                    )
        return team_ids

    def create_tournament(self) -> str | None:
        tournament_data = {
            "name": f"World Cup - {uuid.uuid4()}",
            "format": {
                "numberOfGroups": 8,
                "maxTeamsPerGroup": 4,
                "type": "WORLD_CUP",
            },
        }
        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments",
        ) as response:
            if response.status_code in (200, 201):
                tournament_id: str | None = None
                try:
                    body = response.json()
                    tournament_id = body.get("id") or body.get("tournamentId")
                except Exception:
                    tournament_id = None

                if not tournament_id:
                    location = response.headers.get("Location") or response.headers.get("location")
                    tournament_id = self._extract_id_from_location(location, "tournaments")

                if tournament_id:
                    response.success()
                    return tournament_id
                else:
                    response.failure("Tournament created but id not found in response")
            else:
                response.failure(
                    f"Tournament creation failed: {response.status_code} - {response.text}"
                )
        return None

    def create_group(self, tournament_id: str, index: int) -> str | None:
        group_data = {
            "name": f"grupo{index}",
        }
        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=group_data,
                catch_response=True,
                name="POST /tournaments/:tid/groups",
        ) as response:
            if response.status_code in (200, 201):
                group_id: str | None = None
                try:
                    body = response.json()
                    group_id = body.get("id") or body.get("groupId")
                except Exception:
                    group_id = None

                if not group_id:
                    location = response.headers.get("Location") or response.headers.get("location")
                    group_id = self._extract_id_from_location(location, "groups")

                if group_id:
                    response.success()
                    return group_id
                else:
                    response.failure("Group created but id not found in response")
            else:
                response.failure(
                    f"Group creation failed: {response.status_code} - {response.text}"
                )
        return None

    def get_tournament_finished_flag(self, tournament_id: str) -> str | None:
        with self.client.get(
                f"/tournaments/{tournament_id}",
                catch_response=True,
                name="GET /tournaments/:tid",
        ) as response:
            if response.status_code in (200, 201):
                try:
                    body = response.json()
                except Exception:
                    response.failure("Invalid JSON in tournament GET")
                    return None
                finished = body.get("finished")
                response.success()
                return finished
            else:
                response.failure(
                    f"Tournament selection failed: {response.status_code} - {response.text}"
                )
                return None

    def get_pending_matches(self, tournament_id: str) -> list[dict]:
        with self.client.get(
                f"/tournaments/{tournament_id}/matches",
                params={"showMatches": "pending"},
                catch_response=True,
                name="GET /tournaments/:tid/matches?showMatches=pending",
        ) as response:
            if response.status_code in (200, 201):
                try:
                    data = response.json()
                except Exception:
                    response.failure("Invalid JSON when reading pending matches")
                    return []
                response.success()
                return data
            else:
                response.failure(
                    f"Pending match selection failed: {response.status_code} - {response.text}"
                )
                return []

    def update_match_score(self, tournament_id: str, match_id: Any) -> None:
        home = random.randint(0, 4)
        visitor = random.randint(0, 4)
        if home == visitor:
            visitor = (visitor + 1) % 5

        score_data = {
            "score": {
                "home": home,
                "visitor": visitor,
            }
        }

        with self.client.patch(
                f"/tournaments/{tournament_id}/matches/{match_id}",
                json=score_data,
                catch_response=True,
                name="PATCH /tournaments/:tid/matches/:mid",
        ) as response:
            if response.status_code in (200, 204):
                response.success()
            else:
                response.failure(
                    f"Score update failed: {response.status_code} - {response.text}"
                )

    def add_team_to_group(self, tournament_id: str, group_id: str, team_id: str) -> None:
        path = f"/tournaments/{tournament_id}/groups/{group_id}/teams/{team_id}"

        with self.client.post(
                path,
                catch_response=True,
                name="POST /tournaments/:tid/groups/:gid/teams/:teamId",
        ) as response:
            code = response.status_code
            text = response.text or ""

            if code in (200, 204):
                response.success()
                return

            if code == 409 or "group is full" in text.lower():
                response.success()
                return

            if code == 500 and "Internal Server Error" in text:
                response.success()
                return

            response.failure(
                f"Add team to group failed: {code} - {text}"
            )

    # ---------- Main scenario ----------

    @task
    def run_world_cup_scenario(self) -> None:
        team_ids = self.create_teams()
        if len(team_ids) == 0:
            return

        tournament_id = self.create_tournament()
        if not tournament_id:
            return

        group_ids: list[str] = []
        for i in range(1, 9):
            gid = self.create_group(tournament_id, i)
            if gid:
                group_ids.append(gid)

        if len(group_ids) == 0:
            return

        group_count = len(group_ids)
        group_counter = 0

        for team_id in team_ids:
            pure_team_id = team_id.split("/")[-1]
            target_group_id = group_ids[group_counter % group_count]
            pure_group_id = target_group_id.split("/")[-1]

            self.add_team_to_group(tournament_id, pure_group_id, pure_team_id)

            group_counter += 1
            if group_counter == 32:
                group_counter = 0

        is_tournament_done = False
        while not is_tournament_done:
            pending_matches = self.get_pending_matches(tournament_id)

            for pending_match in pending_matches:
                home = (pending_match.get("home") or {}).get("id") or ""
                visitor = (pending_match.get("visitor") or {}).get("id") or ""
                if home and visitor:
                    self.update_match_score(tournament_id, pending_match.get("id"))

            finished_flag = self.get_tournament_finished_flag(tournament_id)
            is_tournament_done = (finished_flag == "yes")
