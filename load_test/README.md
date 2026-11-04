# Tournament API load test (Locust)

Each simulated user repeatedly creates an independent tournament with unique
resource names. A scenario covers the tournament setup routes:

- Health and Prometheus metrics reads.
- Team creation, individual team reads, and team listing.
- Tournament creation and listing.
- Group creation, individual group reads, and group listing.
- Team assignment through PATCH, followed by verification of persisted membership.

The default `setup` scenario stops there. The `full` scenario (`--scenario
full`) then plays a whole tournament: see [Full tournament scenario](#full-tournament-scenario).
The generic group PATCH returns 501 and is deliberately excluded.
The script groups statistics by route templates rather than individual UUIDs,
validates status codes/IDs/response data, and aborts a scenario at its first
failure. `SCENARIO Tournament lifecycle` reports end-to-end success and latency
in addition to individual HTTP request statistics.

## Install

Run from the repository root. Python 3.10+ is recommended; use a Python version
supported by the resolved Locust release. Create a virtual environment outside
the repository to avoid accidentally committing it:

```bash
python3 -m venv ~/.venvs/tournaments-load
~/.venvs/tournaments-load/bin/python -m pip install -r load_test/requirements.txt
```

These commands work in Bash and fish without activation. If your system lacks
Python venv/pip support, install those OS packages first.

## Start the application first

Use the repository README to start PostgreSQL, ActiveMQ, the API, and consumer.
ActiveMQ must be available: tournament creation and team assignment publish
messages. Use a **disposable local environment**, not production.

For the normal kind NodePort or Compose setup, the base URL is
`http://localhost:8080`. For Traefik, resolve `tournaments.local` to `127.0.0.1`
in your hosts file and use `http://tournaments.local:8080` as `--host` instead.
The load test does not rewrite the Host header or rely on redirects.

## Run interactively

```bash
~/.venvs/tournaments-load/bin/locust -f load_test/locustfile.py \
  --host http://localhost:8080
```

Open <http://localhost:8089> and start with a small user count. Each user waits
1–3 seconds between complete scenarios; requests inside the scenario run in
sequence. The default scenario creates 8 teams in the tournament's single group.

## Run headless (Bash or fish)

```bash
~/.venvs/tournaments-load/bin/locust -f load_test/locustfile.py \
  --host http://localhost:8080 \
  --headless --users 5 --spawn-rate 1 --run-time 2m \
  --teams-per-tournament 8 \
  --csv /tmp/tournaments-load --html /tmp/tournaments-load.html
```

Tournaments always use one group of 32 teams (the API rejects any other
format), so `--teams-per-tournament` must be between 1 and 32; users stop with a
configuration error otherwise. With 32 teams the group fills, and the consumer
generates the 496-match round-robin schedule in the background. That adds
database write load not visible as Locust requests, so watch the consumer and
database alongside API timings. Below 32 teams no matches are generated.
Increase concurrency gradually and compare API p95, database timings, and Pod
CPU with `docs/monitoring.md`.

The task may be interrupted at the run-time limit, leaving a partial tournament.
Use `--stop-timeout 60` if you want to allow an in-progress scenario time to
finish (not guaranteed if the scenario takes longer). A single scenario includes
`2 * teams + 10` HTTP requests, plus one synthetic SCENARIO sample.

## Full tournament scenario

```bash
~/.venvs/tournaments-load/bin/locust -f load_test/locustfile.py \
  --host http://localhost:8080 \
  --headless --users 2 --spawn-rate 1 --run-time 5m --stop-timeout 60 \
  --scenario full --schedule-timeout 120
```

Each iteration always uses 32 teams (`--teams-per-tournament` is ignored) and:

1. Runs the setup scenario, which fills the group.
2. Polls `GET /tournaments/{id}/matches?stage=ROUND_ROBIN` once a second until the
   consumer has generated all 496 matches, or fails after `--schedule-timeout`
   seconds. The wait is reported as the `SCENARIO Round-robin schedule ready`
   sample, which measures the ActiveMQ → consumer → database path.
3. Scores every round-robin match with `PUT .../matches/{matchId}/score`. The team
   created first beats every later team (3–1), so the result is predictable.
4. Checks `GET /tournaments/{id}/standings`: rank, team, games played and wins
   must match that predictable table.
5. Plays the playoffs: checks the quarter-final seeding (1v8, 4v5, 2v7, 3v6), and
   the better seed wins each match, through the semi-finals and final. The API
   creates each next round inside the score request, so no polling is needed.

That is 74 setup requests, at least one schedule poll, 503 score updates and 4
reads: about 580 HTTP requests per iteration, against 26 for the default
scenario. The consumer must be running, or every iteration fails at the
schedule timeout. Start with one or two users.

## Data and interpretation

If a user stops in `on_start` with `StopUser`, the tournament sizes are invalid;
this is not an API/network error. Read the preceding log message for the actual
team count. In interactive mode, check the custom team field in the start
dialog as well as CLI arguments: the web UI can override it. For example, 8 or
32 teams is valid; 33 is not. Stop the test and restart with a corrected count.

- There are no delete APIs, so **every iteration retains its data**. This script
  performs no cleanup. Reset only disposable database storage using the database
  guide when intended; never delete existing data automatically for a load test.
- Global team/tournament list endpoints return all accumulated rows. Their cost
  grows over time; keep dataset size consistent when comparing benchmark runs.
- Each user's state is independent; no shared tournament is mutated concurrently.
  This measures independent workflows, not contention on a single tournament.
- The synthetic SCENARIO samples contribute to Locust's aggregate statistics.
  Filter by request type when calculating HTTP-only throughput.
- No real load is generated merely by importing the script or running its tests.

## Script tests

After installing dependencies:

```bash
~/.venvs/tournaments-load/bin/python -m unittest discover -s load_test -p 'test_*.py'
```
