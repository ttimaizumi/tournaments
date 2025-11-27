
DB script ewf
````
podman run -d --replace --name=tournament_db --network development -e POSTGRES_PASSWORD=password -p 5432:5432 postgres:17.6-alpine3.22
podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql
````

activemq
````
podman run -d --replace --name artemis --network development -p 61616:61616 -p 8161:8161 -p 5672:5672  apache/activemq-classic:6.1.7
````
Test trigger auto build

//new things

podman build -v /home/tomas/workspace/container/cache/vcpkg/:/opt/vcpkg:z -f tournament_services/Containerfile -t tournament_services

podman build -f tournament_services/Containerfile -t tournament_services
podman build -f tournament_consumer/Containerfile -t tournament_consumer 


podman run --replace -d --network dev --name tournament_services_1 -m 256m tournament_services
podman run --replace -d --network dev --name tournament_services_2 -m 256m tournament_services
podman run --replace -d --network dev --name tournament_services_3 -m 256m tournament_services

podman run --replace -d --network dev --name tournament_consumer_1 -m 256m tournament_consumer
podman run --replace -d --network dev --name tournament_consumer_2 -m 256m tournament_consumer
podman run --replace -d --network dev --name tournament_consumer_3 -m 256m tournament_consumer


podman run -d --replace --name load_balancer --network dev -p 8000:8080 -p 8404:8404 -v ./tournament_services/haproxy.cfg:/usr/local/etc/haproxy/haproxy.cfg:Z haproxy