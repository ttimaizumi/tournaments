
DB script
````
podman run -d --replace --name=tournament_db --network development -e POSTGRES_PASSWORD=password -p 5432:5432 -m 256m postgres:17.6-alpine3.22
podman exec -i tournament_db psql -U postgres -d postgres < db_script.sql
````

activemq
````
podman run -d --replace --name artemis --network development -p 61616:61616 -p 8161:8161 -p 5672:5672 -m 256m  apache/activemq-classic:6.1.7
````
