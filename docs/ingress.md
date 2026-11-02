# Optional: expose the API through Traefik on kind

This is an alternative to the README's direct API NodePort mapping. It uses
the standard Kubernetes **Ingress** API, a Traefik controller, and hostname
routing. Requires Helm 3+, `kind`, `kubectl`, and the container runtime used by
your cluster. Run from the repository root. Nothing is installed automatically.

```text
http://tournaments.local:8080
  → host 127.0.0.1:8080 → kind node:30081
  → Traefik web entrypoint → tournaments-api Service:8080 → ready API Pods
```

## 1. Create a cluster with the ingress port mapping

Use [kind-traefik.yml](../kind-traefik.yml) **instead of** `kind.yml` when
creating the cluster. Both configurations use host port 8080, but the Traefik
variant sends it to the controller's NodePort `30081`, not the API's `30080`.
Ports 3000 and 9090 remain available for the existing monitoring setup.
All three host ports must be free; two clusters cannot bind them simultaneously.

**Bash:**

```bash
export KIND_CLUSTER=tournaments-ingress
kind create cluster --name "$KIND_CLUSTER" --config kind-traefik.yml
kubectl config use-context "kind-${KIND_CLUSTER}"
```

**Fish:**

```fish
set -gx KIND_CLUSTER tournaments-ingress
kind create cluster --name "$KIND_CLUSTER" --config kind-traefik.yml
kubectl config use-context "kind-$KIND_CLUSTER"
```

kind cannot add mappings to a running cluster. Do not delete an existing cluster
without backing up any needed database data; deleting it removes local storage.
If its ports are occupied and you need to keep that cluster running, choose
different `hostPort` values in `kind-traefik.yml` before creation, and adjust the
URLs below. For an existing cluster without this mapping, see the port-forward
fallback in section 4 instead of creating another cluster.

Continue README steps 1–3 on the selected cluster: initialize PostgreSQL,
build/load the application images, install Metrics Server, and deploy the
applications. Skip the README's cluster-creation command and do not switch to
another context. The API Deployment, HPA, and database configuration are unchanged.

## 2. Install Traefik

The following Helm repo commands work in Bash and fish:

```bash
helm repo add traefik https://traefik.github.io/charts
helm repo update
helm search repo traefik/traefik --versions | head
```

Choose a chart version compatible with your Kubernetes version and record it.
The values use the modern Traefik chart's `ports.web.expose.default` structure.
Check the selected chart's values if using a substantially different release.

**Bash:**

```bash
read -r -p 'Compatible Traefik chart version: ' TRAEFIK_CHART_VERSION
```

**Fish:**

```fish
read --prompt-str 'Compatible Traefik chart version: ' TRAEFIK_CHART_VERSION
```

Then, in either shell:

```bash
helm upgrade --install traefik traefik/traefik \
  --version "$TRAEFIK_CHART_VERSION" \
  --namespace traefik --create-namespace \
  --values kubernetes/traefik/values.yml \
  --wait --timeout 5m
kubectl -n traefik rollout status deployment/traefik --timeout=180s
kubectl get ingressclass traefik
kubectl -n traefik get service traefik
```

[values.yml](../kubernetes/traefik/values.yml) creates the controller Service
as `NodePort` with HTTP node port `30081`, matching the kind mapping. The
`traefik` IngressClass is explicit, not the default for unrelated applications.
The Traefik dashboard is not exposed. This example is HTTP-only; HTTPS is not
enabled simply by installing the controller.

## 3. Apply the application route

After the API Service exists:

```bash
kubectl apply -f kubernetes/traefik/api-ingress.yml
kubectl -n tournaments get ingress tournaments-api
```

[api-ingress.yml](../kubernetes/traefik/api-ingress.yml) routes hostname
`tournaments.local`, path prefix `/`, to `tournaments-api:8080` in the same
namespace. It preserves the request path and uses `ingressClassName: traefik`.
Ingress rules do not configure DNS or install a controller themselves.

The existing API Service is still `NodePort` to preserve the direct-access
alternative, but Traefik reaches it internally; its NodePort is not used here.
For an ingress-only setup, optionally convert it to `ClusterIP`:

```bash
kubectl -n tournaments patch service tournaments-api --type=strategic \
  -p '{"spec":{"type":"ClusterIP","ports":[{"port":8080,"nodePort":null}]}}'
```

If choosing that option permanently, also change the Service in
`kubernetes/api.yml` to `type: ClusterIP` and remove `nodePort` so reapplying it
does not restore NodePort. Do not change the Deployment or Service selectors.

## 4. Access and verify the API

Add this line to your machine's hosts file (usually `/etc/hosts`) using your
usual editor/administrator workflow:

```text
127.0.0.1 tournaments.local
```

Open <http://tournaments.local:8080>. The API's actual endpoints include
`/health` and `/teams`; a 404 at `/` is expected. Test without editing hosts:

```bash
curl --fail --resolve tournaments.local:8080:127.0.0.1 \
  http://tournaments.local:8080/health
curl --fail --resolve tournaments.local:8080:127.0.0.1 \
  http://tournaments.local:8080/teams
```

Requests to `http://localhost:8080` will not match the hostname rule; use
`tournaments.local` or pass `-H 'Host: tournaments.local'` with curl. Update API
URLs in the monitoring guide's manual tests/load generation accordingly.
Prometheus ServiceMonitor scrapes still use the internal Service and do not
need DNS or Ingress changes. Grafana and Prometheus retain their existing
localhost ports 3000 and 9090; this guide does not route them through Traefik.

For an existing cluster without the ingress host mapping, leave this running
in another terminal, then run the same hostname-based tests:

```bash
kubectl -n traefik port-forward service/traefik 8080:80
```

Stop any existing API port-forward on port 8080 first. This fallback exercises
Traefik routing but is not the persistent host-port setup.

## Troubleshooting and production differences

```bash
kubectl -n traefik logs deployment/traefik --tail=100
kubectl -n tournaments describe ingress tournaments-api
kubectl -n tournaments get endpointslices \
  -l kubernetes.io/service-name=tournaments-api
```

- **Connection refused:** check cluster port mappings and controller readiness.
- **Traefik 404:** verify the Host header and IngressClass; plain `localhost`
  does not match `tournaments.local`.
- **502/503:** check API Pod readiness, Service selectors, and backend port.
- **Empty Ingress ADDRESS:** a local NodePort controller may not publish an
  external load-balancer address. Test routing rather than relying on this field.
- **Helm schema error:** inspect `helm show values traefik/traefik --version
  "$TRAEFIK_CHART_VERSION"` for the chosen release. Helm values are not resources
  for `kubectl apply`; do not apply the entire `kubernetes/traefik/` directory.

Host port 8080 avoids privileged-port restrictions with rootless Podman. To use
standard HTTP port 80, change only its `hostPort` before cluster creation and
adjust URLs; the controller NodePort remains `30081`. HTTPS additionally needs
an enabled secure entrypoint, a mapped host port, TLS certificates, and Ingress
TLS configuration. Production normally exposes the controller with an external
load balancer, uses real DNS and HTTPS, and keeps application Services internal.
Keep this local HTTP setup on loopback; do not expose it publicly as-is.
