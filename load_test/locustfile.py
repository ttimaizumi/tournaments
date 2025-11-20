# from typing import Any
#
# from locust import HttpUser, task
# import json
# import uuid
#
# class TournamentUser(HttpUser):
#
#     def create_teams(self):
#         team_ids = list()
#         for i in range(32):
#             team_data = {
#                 "name": f"Team {uuid.uuid4()}"
#             }
#             with self.client.post(
#                     "/teams",
#                     json=team_data,
#                     catch_response=True,
#                     name="POST /teams"
#             ) as response:
#                 if response.status_code == 200 or response.status_code == 201:
#                     # Extract group ID from Location header
#                     location = response.headers.get("Location") or response.headers.get("location")
#                     team_ids.append(location)
#                 else:
#                     response.failure(f"Team creation failed: {response.status_code}")
#         return team_ids
#     def create_tournament(self):
#         tournament_data = {
#             "name": f"Tournament - {uuid.uuid4()}"
#         }
#         with self.client.post(
#                 "/tournaments",
#                 json=tournament_data,
#                 catch_response=True,
#                 name="POST /tournaments"
#         ) as response:
#             if response.status_code == 200 or response.status_code == 201:
#                 # Extract group ID from Location header
#                 return response.headers.get("Location") or response.headers.get("location")
#             else:
#                 response.failure(f"Tournament creation failed: {response.status_code}")
#
#     def create_group(self, tournament_id: Any | None):
#         group_data = {
#             "name": f"Group - {uuid.uuid4()}"
#         }
#         with self.client.post(
#                 f"/tournaments/{tournament_id}/groups",
#                 json=group_data,
#                 catch_response=True,
#                 name=f"POST /tournaments/{tournament_id}/groups"
#         ) as response:
#             if response.status_code == 200 or response.status_code == 201:
#                 # Extract group ID from Location header
#                 return response.headers.get("Location") or response.headers.get("location")
#             else:
#                 response.failure(f"Group creation failed: {response.status_code}")
#
#     @task
#     def get_teams(self):
#         #self.client.get("/teams")
#         team_ids = self.create_teams()
#         tournament_id = self.create_tournament()
#         group_id = self.create_group(tournament_id)
#         for team_id in team_ids:
#             team_data = [{
#                 "id": f"{team_id}"
#             }]
#             self.client.patch(
#                     f"/tournaments/{tournament_id}/groups/{group_id}/teams",
#                     json=team_data,
#                     catch_response=True,
#                     name=f"POST /tournaments/{tournament_id}/groups/{group_id}/teams"
#             )
# locustfile.py

import os
import time
import uuid

from locust import HttpUser, task

TEAMS_PER_TOURNAMENT = int(os.environ.get("LOADTEST_TEAMS", "32"))


def _extract_id_from_location(location_header: str) -> str:
    """
    Recibe el header Location (puede ser solo el id o una URL)
    y regresa el ultimo segmento como id.
    """
    if not location_header:
        raise ValueError("Location header vacio, no se pudo obtener el id")
    return location_header.rstrip("/").rsplit("/", 1)[-1]


def create_teams(client, count: int = 32):
    team_ids = []
    for i in range(count):
        body = {
            "name": f"Team-{i+1}-{uuid.uuid4()}"
        }
        with client.post(
                "/teams",
                json=body,
                catch_response=True,
                name="POST /teams"
        ) as resp:
            if resp.status_code not in (200, 201):
                resp.failure(f"POST /teams fallo ({resp.status_code}): {resp.text}")
                raise RuntimeError("No se pudo crear equipo")

            loc = resp.headers.get("location") or resp.headers.get("Location")
            if loc:
                team_id = _extract_id_from_location(loc)
            else:
                # fallback por si algun dia regresan el cuerpo con el id
                try:
                    data = resp.json()
                except ValueError:
                    data = {}
                team_id = data.get("id")

            if not team_id:
                resp.failure("No se pudo obtener el id del equipo creado")
                raise RuntimeError("No se pudo obtener el id del equipo creado")

            team_ids.append(team_id)
    return team_ids


def create_tournament(client):
    body = {
        "name": f"LoadTest-DE-{uuid.uuid4().hex[:8]}",
        "format": {
            "maxTeamsPerGroup": 32,
            "numberOfGroups": 1,
            "type": "DOUBLE_ELIMINATION",
        },
    }
    with client.post(
            "/tournaments",
            json=body,
            catch_response=True,
            name="POST /tournaments"
    ) as resp:
        if resp.status_code not in (200, 201):
            resp.failure(f"POST /tournaments fallo ({resp.status_code}): {resp.text}")
            raise RuntimeError("No se pudo crear torneo")

        loc = resp.headers.get("location") or resp.headers.get("Location")
        if loc:
            tournament_id = _extract_id_from_location(loc)
        else:
            try:
                data = resp.json()
            except ValueError:
                data = {}
            tournament_id = data.get("id")

        if not tournament_id:
            resp.failure("No se pudo obtener el id del torneo")
            raise RuntimeError("No se pudo obtener el id del torneo")

        return tournament_id


def create_group(client, tournament_id: str):
    body = {
        "name": "Group-A"
    }
    with client.post(
            f"/tournaments/{tournament_id}/groups",
            json=body,
            catch_response=True,
            name="POST /tournaments/:id/groups"
    ) as resp:
        if resp.status_code not in (200, 201):
            resp.failure(
                f"POST /tournaments/{tournament_id}/groups fallo "
                f"({resp.status_code}): {resp.text}"
            )
            raise RuntimeError("No se pudo crear grupo")

        loc = resp.headers.get("location") or resp.headers.get("Location")
        if loc:
            group_id = _extract_id_from_location(loc)
        else:
            try:
                data = resp.json()
            except ValueError:
                data = {}
            group_id = data.get("id")

        if not group_id:
            resp.failure("No se pudo obtener el id del grupo")
            raise RuntimeError("No se pudo obtener el id del grupo")

        return group_id


def add_teams_to_group(client, tournament_id: str, group_id: str, team_ids):
    """
    Usa el endpoint UpdateTeams del GroupController:
    POST /tournaments/{tournamentId}/groups/{groupId}
    Body: arreglo de objetos { "id": "...", "name": "..."? }
    """
    payload = [{"id": tid} for tid in team_ids]
    with client.post(
            f"/tournaments/{tournament_id}/groups/{group_id}",
            json=payload,
            catch_response=True,
            name="POST /tournaments/:id/groups/:id/teams"
    ) as resp:
        try:
            resp.raise_for_status()
        except Exception:
            resp.failure(
                f"POST /tournaments/{tournament_id}/groups/{group_id} fallo "
                f"({resp.status_code}): {resp.text}"
            )
            raise


def wait_for_tournament_finished(
        client,
        tournament_id: str,
        timeout_seconds: float = 120.0,
        poll_interval: float = 1.0,
        stable_checks: int = 5,
):
    """
    Estrategia:
      - Preguntar /tournaments/{id}/matches periodicamente.
      - Cuando el numero de matches deja de crecer y se mantiene igual
        durante `stable_checks` consultas seguidas, asumimos que el
        torneo ya termino (el consumer ya no esta generando nada nuevo).
    """
    start = time.time()

    print(f"[INFO] Esperando a que se generen matches para torneo {tournament_id}...")

    last_count = -1
    stable = 0

    while True:
        with client.get(
                f"/tournaments/{tournament_id}/matches",
                catch_response=True,
                name="GET /tournaments/:id/matches"
        ) as resp:
            if resp.status_code != 200:
                resp.failure(
                    f"GET /tournaments/{tournament_id}/matches fallo "
                    f"({resp.status_code}): {resp.text}"
                )
                raise RuntimeError("No se pudieron obtener matches")

            try:
                matches = resp.json()
            except ValueError:
                matches = []

        # Puede ser lista o dict, pero en tu caso parece lista
        if isinstance(matches, list):
            total_matches = len(matches)
        else:
            total_matches = len(matches.get("items", []))

        print(
            f"[INFO] Torneo {tournament_id}: se detectan {total_matches} matches "
            f"(estable={stable}/{stable_checks})"
        )

        # Si nunca se generan matches y se pasa el timeout, error
        if total_matches == 0 and (time.time() - start) > timeout_seconds:
            raise TimeoutError(
                f"Torneo {tournament_id}: no se generaron matches "
                f"en {timeout_seconds} segundos"
            )

        # Revisar estabilidad: si el numero de matches no cambia, subimos contador
        if total_matches == last_count and total_matches > 0:
            stable += 1
        else:
            stable = 0
            last_count = total_matches

        # Si llevamos N lecturas seguidas con el mismo numero, asumimos que ya termino
        if total_matches > 0 and stable >= stable_checks:
            print(
                f"[INFO] Torneo {tournament_id} sin nuevos matches en "
                f"{stable_checks} ciclos consecutivos. Se asume que el torneo ya termino."
            )
            break

        if (time.time() - start) > timeout_seconds:
            raise TimeoutError(
                f"Torneo {tournament_id}: no se estabilizo el numero de matches "
                f"en {timeout_seconds} segundos (ultimo total={total_matches})"
            )

        time.sleep(poll_interval)


class TournamentUser(HttpUser):
    # Locust usa host en lugar de BASE_URL
    host = os.environ.get("TOURNAMENT_BASE_URL", "http://localhost:8000")

    @task
    def run_single_tournament(self):
        print("=" * 80)
        print(f"[CONFIG] host = {self.host}")
        print(f"[CONFIG] Equipos por torneo = {TEAMS_PER_TOURNAMENT}")
        print("=" * 80)

        # 1) Crear equipos
        print(f"[STEP] Creando {TEAMS_PER_TOURNAMENT} equipos...")
        team_ids = create_teams(self.client, count=TEAMS_PER_TOURNAMENT)
        print(f"[OK] Se crearon {len(team_ids)} equipos.")

        # 2) Crear torneo
        print("[STEP] Creando torneo DOUBLE_ELIMINATION...")
        tournament_id = create_tournament(self.client)
        print(f"[OK] Torneo creado con id = {tournament_id}")

        # 3) Crear grupo
        print("[STEP] Creando grupo para el torneo...")
        group_id = create_group(self.client, tournament_id)
        print(f"[OK] Grupo creado con id = {group_id}")

        # 4) Agregar equipos al grupo
        print("[STEP] Agregando equipos al grupo...")
        add_teams_to_group(self.client, tournament_id, group_id, team_ids)
        print("[OK] Equipos agregados al grupo.")

        # 5) Esperar a que el torneo termine
        print("[STEP] Esperando a que el torneo termine...")
        wait_for_tournament_finished(self.client, tournament_id)
        print(f"[DONE] Torneo {tournament_id} finalizado.\n")

        # Para que solo se ejecute una vez y Locust pare
        if self.environment.runner is not None:
            self.environment.runner.quit()
#por que no cambia