-- Upgrades a database created before match scheduling. Run as tournament_admin:
--   psql -h localhost -U tournament_admin -d tournament_db -f database/migrate_matches.sql
-- MATCHES held no data before this change, so it is safe to add the NOT NULL column directly.
\set ON_ERROR_STOP on

BEGIN;

ALTER TABLE MATCHES ADD COLUMN IF NOT EXISTS TOURNAMENT_ID UUID NOT NULL REFERENCES TOURNAMENTS(ID);

CREATE UNIQUE INDEX IF NOT EXISTS match_unique_slot_idx ON MATCHES (
    tournament_id, (document->>'stage'), (document->>'round'), (document->>'matchNumber'));

COMMIT;
