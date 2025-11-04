from typing import Any

from locust import HttpUser, task
import json
import uuid

class TournamentUser(HttpUser):

    def create_teams(self):
        team_ids = list()
        for i in range(32):
            team_data = {
                "name": f"Team {uuid.uuid4()}"
            }
            with self.client.post(
                    "/teams",
                    json=team_data,
                    catch_response=True,
                    name="POST /teams"
            ) as response:
                if response.status_code == 200 or response.status_code == 201:
                    # Extract group ID from Location header
                    location = response.headers.get("Location") or response.headers.get("location")
                    team_ids.append(location)
                else:
                    response.failure(f"Team creation failed: {response.status_code}")
        return team_ids
    def create_tournament(self):
        tournament_data = {
            "name": f"Tournament - {uuid.uuid4()}"
        }
        with self.client.post(
                "/tournaments",
                json=tournament_data,
                catch_response=True,
                name="POST /tournaments"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # Extract group ID from Location header
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"Tournament creation failed: {response.status_code}")

    def create_group(self, tournament_id: Any | None):
        group_data = {
            "name": f"Group - {uuid.uuid4()}"
        }
        with self.client.post(
                f"/tournaments/{tournament_id}/groups",
                json=group_data,
                catch_response=True,
                name=f"POST /tournaments/{tournament_id}/groups"
        ) as response:
            if response.status_code == 200 or response.status_code == 201:
                # Extract group ID from Location header
                return response.headers.get("Location") or response.headers.get("location")
            else:
                response.failure(f"Group creation failed: {response.status_code}")

    @task
    def get_teams(self):
        #self.client.get("/teams")
        team_ids = self.create_teams()
        tournament_id = self.create_tournament()
        group_id = self.create_group(tournament_id)
        for team_id in team_ids:
            team_data = [{
                "id": f"{team_id}"
            }]
            self.client.patch(
                    f"/tournaments/{tournament_id}/groups/{group_id}/teams",
                    json=team_data,
                    catch_response=True,
                    name=f"POST /tournaments/{tournament_id}/groups/{group_id}/teams"
            )
