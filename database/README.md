# PostgreSQL on local kind

Deploy [postgres.yml](postgres.yml) and initialize PostgreSQL 17.6 using
[db_script.sql](db_script.sql). Run commands from the repository root in Bash.
This setup is for local development, not production.

The password is hard-coded as `password` in the SQL script and the API/consumer
`appsettings.json` files. PostgreSQL's bootstrap password is also `password` in
the deployment manifest. No SOPS, age, or Kubernetes Secret configuration is needed.

## 1. Select your cluster

Requires `kind`, `kubectl`, and a running container runtime.

```bash
kind get clusters
# Replace kind with your cluster name if different.
kubectl config use-context kind-kind
kubectl cluster-info
kubectl get storageclass
```

If needed, create a cluster first with `kind create cluster`. Standard kind
clusters provide the default StorageClass required by the manifest. Check your
context before deploying so you do not modify a different cluster.

## 2. Create the namespace and initialization ConfigMap, then deploy

```bash
kubectl create namespace tournaments --dry-run=client -o yaml | kubectl apply -f -

kubectl -n tournaments create configmap tournament-db-init \
  --from-file=01-tournaments.sql=database/db_script.sql \
  --dry-run=client -o yaml | kubectl apply -f -

kubectl apply -f database/postgres.yml
kubectl -n tournaments rollout status statefulset/tournament-db --timeout=180s
kubectl -n tournaments logs tournament-db-0
```

The SQL script creates `tournament_admin` and `tournament_svc`, both with password
`password`. Do not set `POSTGRES_DB=tournament_db`: the script creates it itself.
The image runs initialization only on the first startup of an empty data
directory. Data survives Pod restarts, but deleting the kind cluster removes
its local storage. This is not a backup.

## 3. Verify the database

```bash
kubectl -n tournaments exec tournament-db-0 -- \
  psql -U tournament_svc -d tournament_db -c '\dt'
```

Expect `teams`, `tournaments`, `groups`, and `matches`, owned by
`tournament_admin`. For an interactive session:

```bash
kubectl -n tournaments exec -it tournament-db-0 -- \
  psql -U tournament_admin -d tournament_db
```

These commands use the image's local Unix-socket authentication inside the Pod;
network connections require the password `password`.

## 4. Connect your application

### Application running on your machine

Leave this running in another terminal:

```bash
kubectl -n tournaments port-forward service/tournament-db 5432:5432
```

The API and consumer JSON configuration files already contain the service
username and password. Their default hostname, `tournament_db`, is for Compose.
For a local kind connection, override the connection string:

```bash
ConnectionStrings__Tournaments='Host=localhost;Port=5432;Database=tournament_db;Username=tournament_svc;Password=password;Maximum Pool Size=2' \
  dotnet run --project src/Tournaments.Api
```

Use `src/Tournaments.Consumer` instead to run the consumer. If local port 5432
is occupied, forward `5433:5432` and use `Port=5433` above. Alternatively, set
`Host=localhost` in the relevant `appsettings.json` when running against kind.

### Application running inside Kubernetes

In the API/consumer container's `env` section, override the Compose hostname
with the Kubernetes service address:

```yaml
env:
  - name: ConnectionStrings__Tournaments
    value: "Host=tournament-db.tournaments.svc.cluster.local;Port=5432;Database=tournament_db;Username=tournament_svc;Password=password;Maximum Pool Size=2"
```

No port-forward is needed inside the cluster. This guide deploys only PostgreSQL;
configure ActiveMQ separately for the API/consumer.

### Compose alternative

```bash
podman compose up --build
```

Compose uses the hard-coded passwords in the SQL script and application JSON
configuration files, with `POSTGRES_PASSWORD: password` for PostgreSQL bootstrap.
No credentials file or environment-variable setup is required.

## Troubleshooting and resetting

```bash
kubectl -n tournaments get pods,pvc
kubectl -n tournaments describe pod tournament-db-0
kubectl -n tournaments logs tournament-db-0
kubectl -n tournaments logs tournament-db-0 --previous
```

- A `Pending` PVC usually means no working default StorageClass. Inspect it with
  `kubectl -n tournaments describe pvc data-tournament-db-0`.
- Missing ConfigMap errors mean step 2 has not completed.
- The SQL script is **not idempotent**. Do not rerun it against an initialized
  database: roles, the database, and tables already exist.
- Updating the ConfigMap or restarting the Pod does **not** rerun initialization.
- Existing database volumes retain their old passwords. If migrating from the
  previous credentials setup, change the PostgreSQL role passwords explicitly,
  or reset disposable local data as described below. This also applies to
  existing Compose volumes.

To start from scratch, these commands **permanently delete all tournament
database data**. Only run them if that is intended:

```bash
kubectl -n tournaments scale statefulset tournament-db --replicas=0
kubectl -n tournaments wait --for=delete pod/tournament-db-0 --timeout=120s
kubectl -n tournaments delete pvc data-tournament-db-0
# If SQL changed, update the ConfigMap using step 2 before scaling back up.
kubectl -n tournaments scale statefulset tournament-db --replicas=1
kubectl -n tournaments rollout status statefulset/tournament-db --timeout=180s
```

The new empty volume is initialized with the current script and hard-coded passwords.
