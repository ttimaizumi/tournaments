# Tournaments

The project is a .NET 10 solution containing an ASP.NET Core API, PostgreSQL repositories, and an ActiveMQ background consumer.

## Projects

- `Tournaments.Core`: domain models, contracts, and application services.
- `Tournaments.Infrastructure`: Npgsql repositories and ActiveMQ publishing.
- `Tournaments.Api`: HTTP endpoints on port 8080.
- `Tournaments.Consumer`: `tournament.team-add` queue consumer.
- `Tournaments.Tests`: API contract and application tests.

## Local development

Requires the .NET 10 SDK.

```bash
dotnet restore
dotnet test
dotnet run --project src/Tournaments.Api
```

Configuration can be overridden with standard ASP.NET Core environment variables, for example:

```bash
ConnectionStrings__Tournaments='Host=localhost;Database=tournament_db;Username=tournament_svc;Password=password' \
ActiveMq__BrokerUri='activemq:tcp://localhost:61616' \
dotnet run --project src/Tournaments.Api
```

## Load testing

Use [the Locust load-test guide](load_test/README.md) to exercise team,
tournament, and group creation, reads, and team assignment. The match, score,
and standings APIs are not exercised by the load test.

## Containers

The API exposes OpenTelemetry HTTP/runtime and Npgsql database-client metrics
at `/metrics`. It also instruments REST requests and SQL operations for tracing;
the Kubernetes Deployment exports traces to Tempo in the `monitoring` namespace.
See [the monitoring guide](docs/monitoring.md)
for Prometheus discovery, Grafana, trace configuration, and server-side SQL analysis.

To run only PostgreSQL in a local kind Kubernetes cluster, follow
[the database setup guide](database/README.md).

Start PostgreSQL, ActiveMQ, the API, and the consumer with:

```bash
podman compose up --build
```

The local setup uses the hard-coded database password `password`; do not use it
in production. The API is available at `http://localhost:8080`; the ActiveMQ console is available at `http://localhost:8161`.

## Deploy the application to kind

Run these commands from the repository root in Bash. Requires `podman`, `kind`,
`kubectl`, and a running container runtime. The existing multi-stage
`Containerfile.api` and `Containerfile.consumer` build the .NET applications;
you do not need the .NET SDK installed on the host to build these images.

### 1. Create the cluster and prepare PostgreSQL

For hostname-based routing through a shared Traefik Ingress controller, follow
[the optional Traefik setup](docs/ingress.md) instead of the direct API exposure
configuration below. It includes separate kind/Helm/Ingress YAML files and
Bash/fish commands; the application build and deployment steps remain the same.

For a new cluster, use [kind.yml](kind.yml) to map host port 8080 to the API's
NodePort (30080). It also reserves localhost ports 3000 and 9090 for Grafana and
Prometheus when installed using [the monitoring guide](docs/monitoring.md).
Ensure all three host ports are free, including any running Compose API
container or dashboard port-forwards.

```bash
# Choose an unused name if you already have a cluster named kind.
export KIND_CLUSTER=kind
kind create cluster --name "$KIND_CLUSTER" --config kind.yml
kubectl config use-context "kind-${KIND_CLUSTER}"
```

Port mappings cannot be added to an existing kind cluster. If keeping an existing
cluster without this mapping, skip cluster creation, set `KIND_CLUSTER` to its
name, select its context, and use the port-forward alternative in step 4.
Alternatively, create a new cluster under a different name and redeploy there.
**Deleting an existing cluster deletes its local database storage.**

Follow steps 1–3 in [the database setup guide](database/README.md) using this
same cluster/context; do not create another cluster or switch back to `kind-kind`
if you chose a different name. Those steps create the `tournaments` namespace,
initialize `tournament_db`, and verify the tables. Then confirm it is ready:

```bash
kubectl -n tournaments rollout status statefulset/tournament-db --timeout=180s
```

### 2. Build and load the application images

```bash
podman build -f Containerfile.api -t localhost/tournaments-api:dev .
podman build -f Containerfile.consumer -t localhost/tournaments-consumer:dev .

# Export Docker-compatible archives so kind can import the Podman images.
export IMAGE_DIR=$(mktemp -d)
podman save --format docker-archive -o "$IMAGE_DIR/api.tar" localhost/tournaments-api:dev
podman save --format docker-archive -o "$IMAGE_DIR/consumer.tar" localhost/tournaments-consumer:dev
kind load image-archive "$IMAGE_DIR/api.tar" --name "$KIND_CLUSTER"
kind load image-archive "$IMAGE_DIR/consumer.tar" --name "$KIND_CLUSTER"
rm "$IMAGE_DIR/api.tar" "$IMAGE_DIR/consumer.tar"
rmdir "$IMAGE_DIR"
```

Loading images puts them in the kind nodes' image stores; no registry push is
needed. If using Docker instead, build with `docker build` using the same tags,
then use `kind load docker-image localhost/tournaments-api:dev
localhost/tournaments-consumer:dev --name "$KIND_CLUSTER"` instead of exporting
and loading archives.

### 3. Deploy ActiveMQ, the API, and the consumer

#### Enable CPU metrics for autoscaling

Install Metrics Server once per cluster. The kubelet certificate workaround
below is for **local kind development only**, not production:

```bash
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/download/v0.8.0/components.yaml
kubectl -n kube-system patch deployment metrics-server --type=json \
  -p='[{"op":"add","path":"/spec/template/spec/containers/0/args/-","value":"--kubelet-insecure-tls"}]'
kubectl -n kube-system rollout status deployment/metrics-server --timeout=180s
kubectl wait --for=condition=Available apiservice/v1beta1.metrics.k8s.io --timeout=180s
```

Metrics Server v0.8.0 requires Kubernetes 1.31 or newer. For older clusters,
choose a release from its
[compatibility matrix](https://github.com/kubernetes-sigs/metrics-server#compatibility-matrix).

#### Deploy the application resources

The API and consumer both need PostgreSQL and ActiveMQ. Apply this local-only
configuration (credentials are intentionally hard-coded, as in Compose):

- [Application configuration](kubernetes/app-config.yml): shared database and broker settings.
- [ActiveMQ](kubernetes/activemq.yml): broker Deployment and Service.
- [API](kubernetes/api.yml): API Deployment and Service.
- [API autoscaler](kubernetes/api-hpa.yml): 1–3 API Pods targeting 40% CPU utilization.
- [Consumer](kubernetes/consumer.yml): background consumer Deployment.

For REST/SQL traces in Grafana, deploy Tempo and its datasource using
[monitoring guide section 3](docs/monitoring.md#export-rest-and-sql-traces-to-tempo).
The API Deployment already points to that backend; without Tempo, trace exports
fail, but the application and Prometheus metrics can still run.

```bash
kubectl apply -f kubernetes/app-config.yml
kubectl apply -f kubernetes/activemq.yml
kubectl apply -f kubernetes/api.yml
kubectl apply -f kubernetes/api-hpa.yml
kubectl apply -f kubernetes/consumer.yml

kubectl -n tournaments rollout status deployment/artemis --timeout=180s
kubectl -n tournaments rollout status deployment/tournaments-api --timeout=180s
kubectl -n tournaments rollout status deployment/tournaments-consumer --timeout=180s
```

The ConfigMap overrides the Compose database hostname (`tournament_db`) with
the PostgreSQL Kubernetes Service address. Both applications connect as
`tournament_svc`; no database port-forward is needed inside the cluster.
`artemis` is the broker Service name, retained from Compose even though the
image runs ActiveMQ Classic. This example does not persist broker messages
across Pod replacements; it is not a production deployment.

The API starts with 1 replica. Its Horizontal Pod Autoscaler (HPA) maintains
between 1 and 3 replicas, targeting **40% of each Pod's requested CPU**, averaged
across API Pods. With a `250m` CPU request, the target is approximately `100m`
(0.1 CPU core) per Pod, not 40% of the host's CPU. Requests are scheduling
reservations, not CPU limits. The Service routes traffic to ready API Pods.
This scales Pods only; the kind node count remains fixed and must have enough
capacity for additional replicas.

After the first metrics samples arrive, inspect scaling with:

```bash
kubectl top pods -n tournaments
kubectl -n tournaments get hpa tournaments-api --watch
```

Scaling is not immediate, and scale-down normally has a stabilization delay.
Reapplying `api.yml` resets the Deployment's replica count to 1 temporarily;
the HPA resumes managing it afterward.

### 4. Access the API and verify database connectivity

With the cluster created from `kind.yml`, the API is available at
`http://localhost:8080` without port-forwarding:

```text
Host 127.0.0.1:8080 → kind node :30080 → API Pod :8080
```

Run:

```bash
curl --fail http://localhost:8080/health
curl --fail http://localhost:8080/teams
kubectl -n tournaments logs deployment/tournaments-consumer --tail=50
```

For an existing cluster without the port mapping, leave this running in another
terminal before running the commands above:

```bash
kubectl -n tournaments port-forward service/tournaments-api 8080:8080
```

`/health` checks only that the API is running. `/teams` queries PostgreSQL and
should return `[]` for an empty database, or existing teams otherwise. To inspect
the database directly:

```bash
kubectl -n tournaments exec tournament-db-0 -- \
  psql -U tournament_svc -d tournament_db -c 'SELECT id, document FROM teams;'
```

Optionally, forward the broker console with
`kubectl -n tournaments port-forward service/artemis 8161:8161` and open
`http://localhost:8161`.

### 5. Rebuild or troubleshoot

After changing application code, repeat step 2, then restart both Deployments
to use the newly loaded images with the same `dev` tags:

```bash
kubectl -n tournaments rollout restart deployment/tournaments-api deployment/tournaments-consumer
kubectl -n tournaments rollout status deployment/tournaments-api --timeout=180s
kubectl -n tournaments rollout status deployment/tournaments-consumer --timeout=180s
```

After changing the ConfigMap, reapply step 3 and restart both applications as
above so they receive the updated environment variables.

```bash
kubectl -n tournaments get pods,services
kubectl -n tournaments logs deployment/tournaments-api --tail=100
kubectl -n tournaments logs deployment/tournaments-consumer --tail=100
kubectl -n tournaments logs deployment/artemis --tail=100
kubectl -n tournaments get events --sort-by=.metadata.creationTimestamp
```

`ErrImageNeverPull` means the application image was not loaded into the selected
kind cluster with the exact tag used above. For database initialization or
storage problems, see [database troubleshooting](database/README.md#troubleshooting-and-resetting).

If the HPA shows `<unknown>` CPU utilization, allow time for metrics collection,
then inspect `kubectl -n tournaments describe hpa tournaments-api` and
`kubectl -n kube-system logs deployment/metrics-server`. Confirm Metrics Server
is available and the API containers have CPU requests. Additional Pods left
`Pending` may indicate insufficient node capacity; inspect their events with
`kubectl -n tournaments describe pod <pod-name>`.
