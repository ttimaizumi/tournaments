#!/usr/bin/env bash
# One-time setup of the AKS (standby) cluster.
set -euo pipefail

RG=${RG:?resource group}
CLUSTER=${CLUSTER:?aks cluster name}
ACR=${ACR:?acr name}
KEYVAULT=${KEYVAULT:?key vault name}
GIT_URL=${GIT_URL:-https://github.com/ttimaizumi/tournaments.git}
GIT_BRANCH=${GIT_BRANCH:-master}
IDENTITY=tournaments-aks

az aks update --resource-group "$RG" --name "$CLUSTER" --attach-acr "$ACR" \
  --enable-oidc-issuer --enable-workload-identity
az aks enable-addons --resource-group "$RG" --name "$CLUSTER" --addons azure-keyvault-secrets-provider

# Workload identity for the "tournaments" service account
az identity create --resource-group "$RG" --name "$IDENTITY"
CLIENT_ID=$(az identity show --resource-group "$RG" --name "$IDENTITY" --query clientId -o tsv)
PRINCIPAL_ID=$(az identity show --resource-group "$RG" --name "$IDENTITY" --query principalId -o tsv)
ISSUER=$(az aks show --resource-group "$RG" --name "$CLUSTER" --query oidcIssuerProfile.issuerUrl -o tsv)
az identity federated-credential create --resource-group "$RG" --identity-name "$IDENTITY" \
  --name tournaments-sa --issuer "$ISSUER" --subject system:serviceaccount:tournaments:tournaments
az role assignment create --assignee-object-id "$PRINCIPAL_ID" --assignee-principal-type ServicePrincipal \
  --role "Key Vault Secrets User" --scope "$(az keyvault show --name "$KEYVAULT" --query id -o tsv)"
echo "Set __AKS_IDENTITY_CLIENT_ID__ in deploy/infra/aks to: $CLIENT_ID"

# Flux: no migrations here; the database replica receives schema changes from the primary
az k8s-configuration flux create --resource-group "$RG" --cluster-name "$CLUSTER" --cluster-type managedClusters \
  --name tournaments --namespace flux-system --scope cluster \
  --url "$GIT_URL" --branch "$GIT_BRANCH" \
  --kustomization name=infra path=./deploy/infra/aks prune=true wait=true \
  --kustomization name=apps path=./deploy/apps/aks prune=true wait=true 'dependsOn=["infra"]'
