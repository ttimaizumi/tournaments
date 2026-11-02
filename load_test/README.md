# Tournament API load test (Locust)

Each simulated user repeatedly creates an independent tournament with unique
resource names. A scenario covers every **implemented** API route:

- Health and Prometheus metrics reads.
- Team creation, individual team reads, and team listing.
- Tournament creation and listing.
- Group creation, individual group reads, and group listing.
- Team assignment through PATCH, followed by verification of persisted membership.

This does **not** simulate matches or declare a tournament winner: those APIs
do not exist. The generic group PATCH returns 501 and is deliberately excluded.
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
sequence. The default scenario creates 8 teams and 2 groups.

## Run headless (Bash or fish)

```bash
~/.venvs/tournaments-load/bin/locust -f load_test/locustfile.py \
  --host http://localhost:8080 \
  --headless --users 5 --spawn-rate 1 --run-time 2m \
  --teams-per-tournament 8 --groups-per-tournament 2 \
  --csv /tmp/tournaments-load --html /tmp/tournaments-load.html
```

`teams` and `groups` must be positive, with at least one team per group and no
more than 32 teams in any group. Users stop with a configuration error if
`groups > teams` or `teams > 32 * groups`. Increase concurrency gradually and
compare API p95, database timings, and Pod CPU with `docs/monitoring.md`.

The task may be interrupted at the run-time limit, leaving a partial tournament.
Use `--stop-timeout 60` if you want to allow an in-progress scenario time to
finish (not guaranteed if the scenario takes longer). A single scenario includes
`2 * teams + 4 * groups + 6` HTTP requests, plus one synthetic SCENARIO sample.

## Data and interpretation

If a user stops in `on_start` with `StopUser`, the tournament sizes are invalid;
this is not an API/network error. Read the preceding log message for the actual
team/group counts. In interactive mode, check the custom team/group fields in
the start dialog as well as CLI arguments: the web UI can override them. For
example, 8 teams and 2 groups is valid; 8 teams and 16 groups is not. Stop the
test and restart with corrected counts.

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
