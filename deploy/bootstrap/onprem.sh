#!/usr/bin/env bash
# One-time setup of the on-prem (primary) cluster. Run with kubectl pointed at that cluster.
set -euo pipefail

RG=${RG:?resource group}
LOCATION=${LOCATION:?azure region}
CLUSTER=${CLUSTER:?arc cluster name}
GIT_URL=${GIT_URL:-https://github.com/ttimaizumi/tournaments.git}
GIT_BRANCH=${GIT_BRANCH:-master}
# Service principal with "Key Vault Secrets User" on the vault
SP_CLIENT_ID=${SP_CLIENT_ID:?}
SP_CLIENT_SECRET=${SP_CLIENT_SECRET:?}

# Connect the cluster to Azure Arc (outbound-only agent)
az connectedk8s connect --resource-group "$RG" --name "$CLUSTER" --location "$LOCATION"

# Key Vault secrets driver
az k8s-extension create --resource-group "$RG" --cluster-name "$CLUSTER" --cluster-type connectedClusters \
  --extension-type Microsoft.AzureKeyVaultSecretsProvider --name akvsecretsprovider

kubectl create namespace tournaments --dry-run=client -o yaml | kubectl apply -f -
kubectl -n tournaments create secret generic secrets-store-creds \
  --from-literal clientid="$SP_CLIENT_ID" --from-literal clientsecret="$SP_CLIENT_SECRET" \
  --dry-run=client -o yaml | kubectl apply -f -
kubectl -n tournaments label secret secrets-store-creds secrets-store.csi.k8s.io/used=true --overwrite

# Flux: infra -> migrations -> apps, each waits for the previous to be healthy
az k8s-configuration flux create --resource-group "$RG" --cluster-name "$CLUSTER" --cluster-type connectedClusters \
  --name tournaments --namespace flux-system --scope cluster \
  --url "$GIT_URL" --branch "$GIT_BRANCH" \
  --kustomization name=infra path=./deploy/infra/onprem prune=true wait=true \
  --kustomization name=migrations path=./deploy/migrations/onprem prune=true wait=true 'dependsOn=["infra"]' \
  --kustomization name=apps path=./deploy/apps/onprem prune=true wait=true 'dependsOn=["migrations"]'
