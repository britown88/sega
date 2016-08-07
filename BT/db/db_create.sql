--
-- File generated with SQLiteStudio v3.1.0 on Sat Aug 6 19:59:55 2016
--
-- Text encoding used: System
--
PRAGMA foreign_keys = off;
BEGIN TRANSACTION;

-- Table: Images
CREATE TABLE Images (id STRING PRIMARY KEY NOT NULL UNIQUE ON CONFLICT REPLACE, image BLOB);

-- Table: Palettes
CREATE TABLE Palettes (id STRING PRIMARY KEY NOT NULL UNIQUE ON CONFLICT REPLACE, palette BLOB);

-- Table: Schemas
CREATE TABLE Schemas (id INTEGER PRIMARY KEY AUTOINCREMENT UNIQUE, "set" STRING NOT NULL, "index" CHAR NOT NULL);

COMMIT TRANSACTION;
PRAGMA foreign_keys = on;
