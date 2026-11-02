# Find API and PostgreSQL hotspots with Prometheus and Grafana

This guide is for the existing **local kind** deployment. Run commands from the
repository root against the same cluster/context as the application. Requires
`kubectl`, Helm 3+, the .NET 10 SDK for the optional instrumentation changes,
and enough host capacity for Prometheus, Grafana, and their supporting Pods.
Nothing in this file is installed automatically.

### Shell compatibility

Commands marked `bash` also work in **fish**, except for Bash's `read -r -p`
prompt and the SQL heredoc (`<<'SQL'`). Fish alternatives are provided beside
those examples below. Pipelines, quoted arguments, and backslash line
continuations can be used unchanged. Run the examples from the repository root
in the same shell session so the chart-version variable remains available.

## What each tool can show

| Layer | Measurements | Requirement |
| --- | --- | --- |
| Kubernetes | API/DB CPU, memory, restarts, replica count | Monitoring stack |
| API | Slow routes, throughput, HTTP errors, runtime and Npgsql metrics | Add instrumentation in section 3 |
| PostgreSQL | Expensive normalized SQL, calls, average execution time | `pg_stat_statements` and exporter |
| Code | Exact expensive methods, allocation stacks, request-to-query correlation | A profiler and/or distributed tracing, not Prometheus alone |

The API currently exposes `/health`, **not `/metrics`**. Installing Grafana alone
does not provide endpoint latency or SQL timing. Metrics Server, installed for
the HPA, is also not a replacement for Prometheus historical metrics.

## 1. Install Prometheus and Grafana

Check the context before installing. The chart installs cluster-wide RBAC and
Prometheus Operator CRDs as well as namespaced workloads:

```bash
kubectl config current-context
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update
helm search repo prometheus-community/kube-prometheus-stack --versions | head
```

Choose a chart version compatible with your Kubernetes version. Record it so
you can repeat the installation; the prompt avoids silently tracking `latest`:

```bash
read -r -p 'Compatible kube-prometheus-stack chart version: ' MONITORING_CHART_VERSION
helm upgrade --install monitoring prometheus-community/kube-prometheus-stack \
  --version "$MONITORING_CHART_VERSION" \
  --namespace monitoring --create-namespace --wait --timeout 10m \
  -f kubernetes/monitoring/prometheus-values.yml

kubectl -n monitoring get pods
```

**Fish:** replace only the `read` line above with the following, then run the
same `helm upgrade` and `kubectl` commands:

```fish
read --prompt-str 'Compatible kube-prometheus-stack chart version: ' MONITORING_CHART_VERSION
```

The [Helm values](../kubernetes/monitoring/prometheus-values.yml) are
local-development settings, not a production sizing recommendation. This file
is input to Helm, not a Kubernetes resource; do not use `kubectl apply` on it
or on the entire `kubernetes/monitoring/` directory.
Prometheus and Grafana have no persistent storage here: Pod replacements can
lose metrics and manually created dashboards. Export dashboards and add PVCs
before relying on retained data. The stack competes with the API and DB for
the kind node's resources; check Pod events if anything stays `Pending`.

## 2. Open the dashboards

With a cluster created using the current [kind.yml](../kind.yml), the Helm values
expose both dashboards automatically while their workloads are running:

| Dashboard | Host URL | Service NodePort |
| --- | --- | --- |
| Grafana | <http://localhost:3000> | `30030` |
| Prometheus | <http://localhost:9090> | `30090` |

No port-forward processes are needed. The kind mappings bind to `127.0.0.1`,
not all host interfaces. If monitoring is already installed, rerun the Helm
upgrade command in section 1 to apply the NodePort values.

**Existing clusters:** changing `kind.yml` does not update a running cluster's
port mappings. Create a new cluster with that configuration and redeploy to
enable automatic host access, or keep using the port-forwards below.
Do not delete a cluster containing data you need: its local database storage
will be lost. Ensure host ports 3000 and 9090 are free before cluster creation.

For clusters without these mappings, run each forward in its own terminal:

```bash
kubectl -n monitoring port-forward service/monitoring-grafana 3000:80
```

```bash
kubectl -n monitoring port-forward service/monitoring-kube-prometheus-prometheus 9090:9090
```

Get Grafana's generated password; do not commit or share it:

```bash
kubectl -n monitoring get secret monitoring-grafana \
  -o jsonpath='{.data.admin-password}' | base64 --decode
```

Open Grafana at <http://localhost:3000> with user `admin`. The chart provisions
a Prometheus data source. Open Prometheus at <http://localhost:9090> and check
**Status → Target health** (or **Targets**, depending on the UI version).

Start with the bundled Kubernetes compute dashboards, selecting namespace
`tournaments` and workload `tournaments-api`. They work before API instrumentation.

## 3. Add API metrics (manual application changes)

This section describes changes to make; this guide does **not** modify the API.
Add the following packages to `src/Tournaments.Api`:

```bash
dotnet add src/Tournaments.Api package OpenTelemetry.Extensions.Hosting
dotnet add src/Tournaments.Api package OpenTelemetry.Instrumentation.AspNetCore
dotnet add src/Tournaments.Api package OpenTelemetry.Instrumentation.Runtime
dotnet add src/Tournaments.Api package OpenTelemetry.Exporter.Prometheus.AspNetCore --prerelease
```

Choose mutually compatible versions and review the versions written to the
project file before committing. The Prometheus exporter may still require a
prerelease version; do not blindly upgrade existing OpenTelemetry dependencies.

In `src/Tournaments.Api/Program.cs`, add these imports:

```csharp
using OpenTelemetry.Metrics;
using OpenTelemetry.Resources;
```

Before `builder.Build()`, add:

```csharp
builder.Services.AddOpenTelemetry()
    .ConfigureResource(resource => resource.AddService("tournaments-api"))
    .WithMetrics(metrics => metrics
        .AddAspNetCoreInstrumentation()
        .AddRuntimeInstrumentation()
        .AddMeter("Npgsql")
        .AddView("http.server.request.duration",
            new ExplicitBucketHistogramConfiguration
            {
                Boundaries = [0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10]
            })
        .AddPrometheusExporter());
```

After `var app = builder.Build();`, add:

```csharp
app.MapPrometheusScrapingEndpoint();
```

Npgsql's meter provides driver/pool metrics supported by the installed Npgsql
version; it does not automatically expose individual SQL text. Rebuild and
load the API image with README step 2, then restart the API Deployment and wait
for rollout. Verify `http://localhost:8080/metrics` contains metrics, not a 404.
Use the README port-forward alternative if the cluster has no host mapping.

Apply the [API ServiceMonitor](../kubernetes/monitoring/api-servicemonitor.yml)
after the Prometheus Operator CRDs are installed:

```bash
# The existing Service has a selector, but needs its own discovery label.
kubectl -n tournaments label service tournaments-api app=tournaments-api --overwrite
kubectl apply -f kubernetes/monitoring/api-servicemonitor.yml
```

`port: http` refers to the **Service port name**, not the container port number.
The ServiceMonitor scrapes individual ready API endpoints, not just a random
Pod through NodePort. If maintaining this setup, add the discovery label to
the Service's `metadata.labels` in `kubernetes/api.yml` as well.

Keep `/metrics` private in any non-local environment. Do not use raw URLs,
team names, UUIDs, passwords, or SQL text as metric labels: they can expose
sensitive information and create excessive time-series cardinality.

## 4. Enable PostgreSQL query statistics

`pg_stat_statements` records normalized query statistics inside PostgreSQL.
Enabling its preload library requires a **database restart and brief downtime**.
Do not delete the PVC or rerun the initialization SQL. These changes persist in
the existing data directory, but must be configured again after a fresh reset.

First inspect the current settings:

```bash
kubectl -n tournaments exec tournament-db-0 -- \
  psql -U postgres -d tournament_db -c 'SHOW shared_preload_libraries;'
```

If empty (the repository default), run the following. If other libraries are
already listed, preserve them and append `pg_stat_statements` instead of
overwriting the list:

```bash
kubectl -n tournaments exec tournament-db-0 -- \
  psql -U postgres -d tournament_db \
  -c "ALTER SYSTEM SET shared_preload_libraries = 'pg_stat_statements';"
kubectl -n tournaments rollout restart statefulset/tournament-db
kubectl -n tournaments rollout status statefulset/tournament-db --timeout=180s

kubectl -n tournaments exec -i tournament-db-0 -- \
  psql -v ON_ERROR_STOP=1 -U postgres -d tournament_db <<'SQL'
CREATE EXTENSION IF NOT EXISTS pg_stat_statements;
DO $$
BEGIN
  IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'tournament_monitor') THEN
    CREATE ROLE tournament_monitor LOGIN PASSWORD 'password';
  END IF;
END;
$$;
GRANT CONNECT ON DATABASE tournament_db TO tournament_monitor;
GRANT USAGE ON SCHEMA public TO tournament_monitor;
GRANT pg_monitor TO tournament_monitor;
SQL
```

**Fish:** use this block instead of the Bash block above. `printf` pipes the SQL
to `psql` because fish does not support heredocs:

```fish
kubectl -n tournaments exec tournament-db-0 -- \
  psql -U postgres -d tournament_db \
  -c "ALTER SYSTEM SET shared_preload_libraries = 'pg_stat_statements';"
kubectl -n tournaments rollout restart statefulset/tournament-db
kubectl -n tournaments rollout status statefulset/tournament-db --timeout=180s

printf '%s\n' \
  'CREATE EXTENSION IF NOT EXISTS pg_stat_statements;' \
  'DO $$' \
  'BEGIN' \
  "  IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'tournament_monitor') THEN" \
  "    CREATE ROLE tournament_monitor LOGIN PASSWORD 'password';" \
  '  END IF;' \
  'END;' \
  '$$;' \
  'GRANT CONNECT ON DATABASE tournament_db TO tournament_monitor;' \
  'GRANT USAGE ON SCHEMA public TO tournament_monitor;' \
  'GRANT pg_monitor TO tournament_monitor;' \
  | kubectl -n tournaments exec -i tournament-db-0 -- \
      psql -v ON_ERROR_STOP=1 -U postgres -d tournament_db
```

The monitoring role is separate from the application role. `pg_monitor` can
see sensitive server/query activity; use strong credentials and stricter
access controls outside local development. Query text and statistics are
also sensitive even though statements are usually normalized.

## 5. Export PostgreSQL metrics to Prometheus

This example uses an exporter with a PostgreSQL 17-compatible statement
collector. The password below is the repository's **local-only** password:

The [exporter manifest](../kubernetes/monitoring/postgres-exporter.yml) contains
its Deployment, Service, and ServiceMonitor. Create its Secret before applying it:

```bash
kubectl -n tournaments create secret generic postgres-exporter \
  --from-literal=DATA_SOURCE_NAME='postgresql://tournament_monitor:password@tournament-db:5432/tournament_db?sslmode=disable' \
  --dry-run=client -o yaml | kubectl apply -f -

kubectl apply -f kubernetes/monitoring/postgres-exporter.yml

kubectl -n tournaments rollout status deployment/postgres-exporter --timeout=180s
```

Verify `pg_up` is `1` in Prometheus and look for `pg_stat_statements_*` metrics
after generating database traffic. The statement collector is deliberately
opt-in: it adds overhead and a series per query identity. This exporter limits
the statement set, so it is not a complete SQL audit log. A missing series is
not proof that a query did not execute. Use SQL statistics directly as well.

## 6. Useful Grafana panels (PromQL)

In Grafana, create a dashboard, add a visualization, choose Prometheus, and
paste a query. Use a time-series visualization for trends and a table for
rankings. Units below matter. Run representative traffic for several minutes;
`rate(...[5m])` needs multiple samples. Compare the same time window across
API, PostgreSQL, and Kubernetes panels.

### API CPU usage in cores, by Pod

```promql
sum by (pod) (
  rate(container_cpu_usage_seconds_total{
    namespace="tournaments", container="api", pod=~"tournaments-api-.*"
  }[5m])
)
```

### API working-set memory in bytes, by Pod

```promql
sum by (pod) (
  container_memory_working_set_bytes{
    namespace="tournaments", container="api", pod=~"tournaments-api-.*"
  }
)
```

Repeat these panels with `container="postgres"` and `pod="tournament-db-0"`
for database resource usage. The API HPA targets 40% of its `250m` CPU request,
approximately `0.1` CPU core per Pod, not 40% of the node.

### Requests per second, by route (requires section 3)

```promql
sum by (http_route) (
  rate(http_server_request_duration_seconds_count{
    namespace="tournaments", service="tournaments-api", http_route!="/metrics"
  }[5m])
)
```

### p95 request duration in seconds, by route

```promql
histogram_quantile(0.95,
  sum by (le, http_route) (
    rate(http_server_request_duration_seconds_bucket{
      namespace="tournaments", service="tournaments-api", http_route!="/metrics"
    }[5m])
  )
)
```

Aggregate histogram buckets across Pods **before** calculating the percentile;
do not average Pod-level p95 values. Low-traffic percentiles are noisy.

### HTTP 5xx responses per second, by route

```promql
sum by (http_route) (
  rate(http_server_request_duration_seconds_count{
    namespace="tournaments", service="tournaments-api",
    http_response_status_code=~"5.."
  }[5m])
)
```

OpenTelemetry metric/label names can vary with package versions and exporter
settings. Inspect `/metrics` or Prometheus's metric browser if these queries
are empty; the examples assume the default translated names and classic
histogram buckets. Use route templates (`/teams/{teamId}`), not individual IDs.

### Top SQL queries by database execution time per second

```promql
topk(10,
  sum by (queryid) (
    rate(pg_stat_statements_seconds_total{
      datname="tournament_db", user="tournament_svc"
    }[5m])
  )
)
```

### Top SQL queries by average execution time in seconds

```promql
topk(10,
  sum by (queryid) (
    rate(pg_stat_statements_seconds_total{datname="tournament_db", user="tournament_svc"}[5m])
  )
  /
  (
    sum by (queryid) (
      rate(pg_stat_statements_calls_total{datname="tournament_db", user="tournament_svc"}[5m])
    ) > 0
  )
)
```

Rank by both total time and average time: a fast query called excessively can
cost more than a slow query called once. Metrics use `queryid`, not SQL text;
map it back to SQL with section 7. Query IDs are not globally stable across
PostgreSQL versions or different databases.

## 7. Identify and investigate the actual SQL

Open a database session as the local bootstrap administrator:

```bash
kubectl -n tournaments exec -it tournament-db-0 -- psql -U postgres -d tournament_db
```

Find the service's most expensive statements. Times here are **milliseconds**
and cumulative since statistics were last reset, not just the Grafana window:

```sql
SELECT queryid, calls,
       round(total_exec_time::numeric, 2) AS total_ms,
       round(mean_exec_time::numeric, 2) AS mean_ms,
       round(max_exec_time::numeric, 2) AS max_ms,
       rows, shared_blks_hit, shared_blks_read,
       temp_blks_written, query
FROM pg_stat_statements
WHERE dbid = (SELECT oid FROM pg_database WHERE datname = current_database())
  AND userid = (SELECT oid FROM pg_roles WHERE rolname = 'tournament_svc')
ORDER BY total_exec_time DESC
LIMIT 20;
```

For lock waits or queries currently running:

```sql
SELECT pid, usename, now() - query_start AS elapsed,
       state, wait_event_type, wait_event, pg_blocking_pids(pid) AS blockers,
       query
FROM pg_stat_activity
WHERE datname = current_database()
  AND state <> 'idle' AND pid <> pg_backend_pid()
ORDER BY query_start;
```

Inspect a real repository query, for example the unpaginated teams listing:

```sql
EXPLAIN (ANALYZE, BUFFERS)
SELECT id, document::text FROM teams ORDER BY created_at;
```

Look for expensive sorts, sequential scans over large tables, inaccurate row
estimates, temporary disk spills, and many buffer reads. A sequential scan is
often correct for small tables or queries returning most rows; do not add an
index just because a scan appears. An index on `created_at` might avoid sorting
but does not solve an endpoint returning an unbounded number of rows.

**`EXPLAIN ANALYZE` executes the statement.** Start with plain `EXPLAIN` for
unknown/expensive queries. Avoid running modifying statements against real
data; transactions do not undo every possible side effect. Substitute realistic
parameters and test changes on disposable data first. Never reset shared query
statistics without coordinating with other users; compare before/after samples
or use Prometheus rates instead.

## 8. A practical hotspot investigation

1. Generate representative traffic through NodePort at `localhost:8080`, using
   realistic data volumes. Port-forward selects a Pod and is not a useful test
   of Service load distribution. Do not benchmark only `/health`.
2. Identify the slow route using p95 latency, throughput, and errors.
3. Compare API/DB CPU and memory, HPA replica counts, and Npgsql pool metrics.
   The configured pool maximum is only 2 connections **per API process**;
   scaling from 1 to 3 Pods changes the aggregate connection budget. The consumer
   has its own pool too.
4. Rank SQL by total time and mean time. Investigate locking if latency rises
   without corresponding CPU usage.
5. Inspect the query plan. Change one thing (pagination, query shape, index,
   pool sizing) and rerun the same workload and measurement window.
6. If API CPU remains high but SQL is cheap, use `dotnet-trace`/`dotnet-counters`
   or a profiler. For exact request-to-query correlation, add OpenTelemetry
   tracing and a backend such as Tempo; Prometheus metrics alone cannot provide
   stack traces or establish which individual request executed a query.

## Troubleshooting

- **Target missing:** check `kubectl get servicemonitors -A`, the Service labels,
  the `release: monitoring` label, and the Operator namespace selectors.
- **API target down/404:** deploy the instrumented image, verify `/metrics`, and
  confirm the Service's named `http` port. `/health` succeeding is not enough.
- **PostgreSQL metrics absent:** inspect
  `kubectl -n tournaments logs deployment/postgres-exporter`; confirm `pg_up`,
  credentials, extension installation, and `SHOW shared_preload_libraries`.
- **No SQL series yet:** generate query traffic; wait for scrapes. The statement
  collector filters/limits its result set. Direct SQL statistics are the fallback.
- **Kubernetes CPU metrics missing:** check Prometheus kubelet/cAdvisor targets.
  Some kind control-plane targets may be unavailable without extra configuration;
  these are different from the API/exporter targets.
- **Unexpected slowdown:** monitor the monitors as well. Scraping and collecting
  SQL statistics consume resources on the same development machine.

Do not expose Grafana, Prometheus, exporter ports, or SQL text publicly. This
guide does not change HPA settings. Dashboard host ports in `kind.yml` are
localhost-only; Prometheus has no authentication configured, so do not change
these bindings to public interfaces without appropriate access controls.
