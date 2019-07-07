--
-- PostgreSQL database dump
--

-- Dumped from database version 11.3 (Debian 11.3-1.pgdg90+1)
-- Dumped by pg_dump version 11.3

-- Started on 2019-07-07 02:21:39

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 3070 (class 1262 OID 16709)
-- Name: beatmods-2; Type: DATABASE; Schema: -; Owner: -
--

CREATE DATABASE "beatmods-2" WITH TEMPLATE = template0 ENCODING = 'UTF8' LC_COLLATE = 'C' LC_CTYPE = 'C';


\connect -reuse-previous=on "dbname='beatmods-2'"

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 11 (class 2615 OID 16710)
-- Name: mod-repo; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA "mod-repo";


--
-- TOC entry 9 (class 2615 OID 16711)
-- Name: server-state; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA "server-state";


--
-- TOC entry 1 (class 3079 OID 16712)
-- Name: adminpack; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS adminpack WITH SCHEMA pg_catalog;


--
-- TOC entry 3071 (class 0 OID 0)
-- Dependencies: 1
-- Name: EXTENSION adminpack; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION adminpack IS 'administrative functions for PostgreSQL';


--
-- TOC entry 4 (class 3079 OID 16721)
-- Name: pgcrypto; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pgcrypto WITH SCHEMA public;


--
-- TOC entry 3072 (class 0 OID 0)
-- Dependencies: 4
-- Name: EXTENSION pgcrypto; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';


--
-- TOC entry 3 (class 3079 OID 16758)
-- Name: uuid-ossp; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS "uuid-ossp" WITH SCHEMA public;


--
-- TOC entry 3073 (class 0 OID 0)
-- Dependencies: 3
-- Name: EXTENSION "uuid-ossp"; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION "uuid-ossp" IS 'generate universally unique identifiers (UUIDs)';


--
-- TOC entry 658 (class 1247 OID 16770)
-- Name: Approval; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Approval" AS ENUM (
    'Approved',
    'Declined',
    'Pending',
    'Inactive'
);


--
-- TOC entry 661 (class 1247 OID 16780)
-- Name: CompareOp; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."CompareOp" AS ENUM (
    'Greater',
    'Equal',
    'GreaterEqual',
    'Less',
    'LessEqual'
);


--
-- TOC entry 664 (class 1247 OID 16792)
-- Name: DownloadType; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."DownloadType" AS ENUM (
    'Steam',
    'Oculus',
    'Universal'
);


--
-- TOC entry 667 (class 1247 OID 16801)
-- Name: ModRange; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."ModRange" AS (
	id character varying(128),
	"versionRange" character varying(64)
);


--
-- TOC entry 670 (class 1247 OID 16803)
-- Name: Permission; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Permission" AS ENUM (
    'gameversion_add',
    'gameversion_edit',
    'mod_create',
    'mod_edit',
    'mod_reposess',
    'user_delete',
    'group_add',
    'group_edit',
    'group_delete',
    'mod_see_pending',
    'mod_approve_deny',
    'user_edit_groups',
    'news_edit',
    'news_add',
    'mod_upload_as'
);


--
-- TOC entry 673 (class 1247 OID 16834)
-- Name: System; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."System" AS ENUM (
    'PC',
    'Quest'
);


--
-- TOC entry 676 (class 1247 OID 16841)
-- Name: Version; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Version" AS (
	major integer,
	minor integer,
	patch integer,
	prerelease text,
	build text
);


--
-- TOC entry 679 (class 1247 OID 16843)
-- Name: Visibility; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Visibility" AS ENUM (
    'Public',
    'Groups'
);


--
-- TOC entry 682 (class 1247 OID 16848)
-- Name: LogLevel; Type: TYPE; Schema: server-state; Owner: -
--

CREATE TYPE "server-state"."LogLevel" AS ENUM (
    'Debug',
    'Warning',
    'Error',
    'Fatal',
    'Info',
    'Security'
);


--
-- TOC entry 276 (class 1255 OID 16861)
-- Name: add_to_enum(regtype, name); Type: FUNCTION; Schema: mod-repo; Owner: -
--

CREATE FUNCTION "mod-repo".add_to_enum(enum_type regtype, new_value name) RETURNS void
    LANGUAGE sql
    AS $$INSERT INTO pg_enum (enumtypid, enumlabel, enumsortorder)
    SELECT enum_type::oid, new_value, ( SELECT MAX(enumsortorder) + 1 FROM pg_enum WHERE enumtypid = enum_type );
$$;


--
-- TOC entry 277 (class 1255 OID 16862)
-- Name: remove_from_enum(regtype, name); Type: FUNCTION; Schema: mod-repo; Owner: -
--

CREATE FUNCTION "mod-repo".remove_from_enum(enum_type regtype, element name) RETURNS void
    LANGUAGE sql
    AS $$DELETE FROM pg_enum
WHERE enumpg_enum.enumtypid = enum_type::oid 
   AND pg_enum.enumlabel = element;$$;


SET default_tablespace = '';

SET default_with_oids = false;

--
-- TOC entry 203 (class 1259 OID 16863)
-- Name: Downloads; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Downloads" (
    mod uuid NOT NULL,
    type "mod-repo"."DownloadType" NOT NULL,
    "cdnFile" text NOT NULL,
    hashes json NOT NULL
);


--
-- TOC entry 204 (class 1259 OID 16869)
-- Name: GameVersion_VisibleGroups_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."GameVersion_VisibleGroups_Joiner" (
    "gameVersion" uuid NOT NULL,
    "group" bigint NOT NULL
);


--
-- TOC entry 205 (class 1259 OID 16872)
-- Name: GameVersions; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."GameVersions" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    version "mod-repo"."Version" NOT NULL,
    "steamBuildId" bigint NOT NULL,
    visibility "mod-repo"."Visibility" NOT NULL
);


--
-- TOC entry 206 (class 1259 OID 16879)
-- Name: Groups; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Groups" (
    id bigint NOT NULL,
    name text NOT NULL,
    permissions "mod-repo"."Permission"[] NOT NULL
);


--
-- TOC entry 207 (class 1259 OID 16885)
-- Name: Groups_id_seq; Type: SEQUENCE; Schema: mod-repo; Owner: -
--

CREATE SEQUENCE "mod-repo"."Groups_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3074 (class 0 OID 0)
-- Dependencies: 207
-- Name: Groups_id_seq; Type: SEQUENCE OWNED BY; Schema: mod-repo; Owner: -
--

ALTER SEQUENCE "mod-repo"."Groups_id_seq" OWNED BY "mod-repo"."Groups".id;


--
-- TOC entry 208 (class 1259 OID 16887)
-- Name: Mods; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Mods" (
    name text NOT NULL,
    description text NOT NULL,
    version "mod-repo"."Version" NOT NULL,
    uploaded timestamp with time zone NOT NULL,
    approved timestamp with time zone,
    "approvalState" "mod-repo"."Approval" NOT NULL,
    uuid uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    author uuid NOT NULL,
    id character varying(128) NOT NULL,
    "gameVersion" uuid NOT NULL,
    system "mod-repo"."System" NOT NULL,
    required boolean NOT NULL,
    "dependsOn" "mod-repo"."ModRange"[] NOT NULL,
    "conflictsWith" "mod-repo"."ModRange"[] NOT NULL
);


--
-- TOC entry 209 (class 1259 OID 16894)
-- Name: Mods_Tags_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Mods_Tags_Joiner" (
    mod uuid NOT NULL,
    tag bigint NOT NULL
);


--
-- TOC entry 210 (class 1259 OID 16897)
-- Name: News; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."News" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    title text NOT NULL,
    author uuid NOT NULL,
    body text NOT NULL,
    posted timestamp with time zone NOT NULL,
    edited timestamp with time zone,
    system "mod-repo"."System"
);


--
-- TOC entry 211 (class 1259 OID 16904)
-- Name: Tags; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Tags" (
    id bigint NOT NULL,
    name text NOT NULL
);


--
-- TOC entry 212 (class 1259 OID 16910)
-- Name: Tags_id_seq; Type: SEQUENCE; Schema: mod-repo; Owner: -
--

CREATE SEQUENCE "mod-repo"."Tags_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3075 (class 0 OID 0)
-- Dependencies: 212
-- Name: Tags_id_seq; Type: SEQUENCE OWNED BY; Schema: mod-repo; Owner: -
--

ALTER SEQUENCE "mod-repo"."Tags_id_seq" OWNED BY "mod-repo"."Tags".id;


--
-- TOC entry 213 (class 1259 OID 16912)
-- Name: Users; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Users" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    name text NOT NULL,
    created timestamp with time zone DEFAULT now() NOT NULL,
    "githubId" text NOT NULL,
    profile text DEFAULT ''::text NOT NULL
);


--
-- TOC entry 214 (class 1259 OID 16921)
-- Name: Users_Groups_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Users_Groups_Joiner" (
    "user" uuid NOT NULL,
    "group" bigint NOT NULL
);


--
-- TOC entry 215 (class 1259 OID 16924)
-- Name: Log; Type: TABLE; Schema: server-state; Owner: -
--

CREATE TABLE "server-state"."Log" (
    id bigint NOT NULL,
    "time" timestamp with time zone NOT NULL,
    message text NOT NULL,
    level "server-state"."LogLevel"
);


--
-- TOC entry 216 (class 1259 OID 16930)
-- Name: Log_id_seq; Type: SEQUENCE; Schema: server-state; Owner: -
--

CREATE SEQUENCE "server-state"."Log_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3076 (class 0 OID 0)
-- Dependencies: 216
-- Name: Log_id_seq; Type: SEQUENCE OWNED BY; Schema: server-state; Owner: -
--

ALTER SEQUENCE "server-state"."Log_id_seq" OWNED BY "server-state"."Log".id;


--
-- TOC entry 217 (class 1259 OID 16932)
-- Name: Tokens; Type: TABLE; Schema: server-state; Owner: -
--

CREATE TABLE "server-state"."Tokens" (
    "user" uuid NOT NULL,
    token text NOT NULL
);


--
-- TOC entry 2886 (class 2604 OID 16938)
-- Name: Groups id; Type: DEFAULT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Groups" ALTER COLUMN id SET DEFAULT nextval('"mod-repo"."Groups_id_seq"'::regclass);


--
-- TOC entry 2889 (class 2604 OID 16939)
-- Name: Tags id; Type: DEFAULT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Tags" ALTER COLUMN id SET DEFAULT nextval('"mod-repo"."Tags_id_seq"'::regclass);


--
-- TOC entry 2893 (class 2604 OID 16940)
-- Name: Log id; Type: DEFAULT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Log" ALTER COLUMN id SET DEFAULT nextval('"server-state"."Log_id_seq"'::regclass);


--
-- TOC entry 2896 (class 2606 OID 16942)
-- Name: Downloads Downloads_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Downloads"
    ADD CONSTRAINT "Downloads_pkey" PRIMARY KEY (mod, type);


--
-- TOC entry 2899 (class 2606 OID 16944)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_pkey" PRIMARY KEY ("gameVersion", "group");


--
-- TOC entry 2901 (class 2606 OID 16946)
-- Name: GameVersions GameVersions_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersions"
    ADD CONSTRAINT "GameVersions_pkey" PRIMARY KEY (id);


--
-- TOC entry 2904 (class 2606 OID 17030)
-- Name: Groups Groups_name_key; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Groups"
    ADD CONSTRAINT "Groups_name_key" UNIQUE (name);


--
-- TOC entry 2906 (class 2606 OID 16948)
-- Name: Groups Groups_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Groups"
    ADD CONSTRAINT "Groups_pkey" PRIMARY KEY (id);


--
-- TOC entry 2913 (class 2606 OID 16950)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_pkey" PRIMARY KEY (mod, tag);


--
-- TOC entry 2909 (class 2606 OID 17032)
-- Name: Mods Mods_id_key; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_id_key" UNIQUE (id);


--
-- TOC entry 2911 (class 2606 OID 16952)
-- Name: Mods Mods_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_pkey" PRIMARY KEY (uuid);


--
-- TOC entry 2916 (class 2606 OID 16954)
-- Name: News News_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."News"
    ADD CONSTRAINT "News_pkey" PRIMARY KEY (id);


--
-- TOC entry 2918 (class 2606 OID 17034)
-- Name: Tags Tags_name_key; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Tags"
    ADD CONSTRAINT "Tags_name_key" UNIQUE (name);


--
-- TOC entry 2920 (class 2606 OID 16956)
-- Name: Tags Tags_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Tags"
    ADD CONSTRAINT "Tags_pkey" PRIMARY KEY (id);


--
-- TOC entry 2928 (class 2606 OID 16958)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_pkey" PRIMARY KEY ("user", "group");


--
-- TOC entry 2923 (class 2606 OID 16960)
-- Name: Users Users_name_key; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users"
    ADD CONSTRAINT "Users_name_key" UNIQUE (name);


--
-- TOC entry 2925 (class 2606 OID 16962)
-- Name: Users Users_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users"
    ADD CONSTRAINT "Users_pkey" PRIMARY KEY (id);


--
-- TOC entry 2930 (class 2606 OID 16964)
-- Name: Log Log_pkey; Type: CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Log"
    ADD CONSTRAINT "Log_pkey" PRIMARY KEY (id);


--
-- TOC entry 2932 (class 2606 OID 16966)
-- Name: Tokens Tokens_pkey; Type: CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Tokens"
    ADD CONSTRAINT "Tokens_pkey" PRIMARY KEY (token);


--
-- TOC entry 2894 (class 1259 OID 16967)
-- Name: Downloads_ModIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Downloads_ModIndex" ON "mod-repo"."Downloads" USING btree (mod) INCLUDE (type, "cdnFile", hashes);


--
-- TOC entry 2897 (class 1259 OID 16968)
-- Name: GameVersion_VisibleGroups_Joiner_index; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE UNIQUE INDEX "GameVersion_VisibleGroups_Joiner_index" ON "mod-repo"."GameVersion_VisibleGroups_Joiner" USING btree ("gameVersion", "group");


--
-- TOC entry 2902 (class 1259 OID 16969)
-- Name: GroupIDindex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "GroupIDindex" ON "mod-repo"."Groups" USING btree (id);


--
-- TOC entry 2907 (class 1259 OID 16970)
-- Name: Mods_SearchIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Mods_SearchIndex" ON "mod-repo"."Mods" USING btree (name text_pattern_ops, description text_pattern_ops, "approvalState", system) INCLUDE (version, uuid, author, id, "gameVersion");


--
-- TOC entry 2914 (class 1259 OID 16971)
-- Name: News_OrderIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "News_OrderIndex" ON "mod-repo"."News" USING btree (posted DESC NULLS LAST, edited DESC NULLS LAST) INCLUDE (id, title, author);


--
-- TOC entry 2921 (class 1259 OID 16972)
-- Name: UserSearch; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE UNIQUE INDEX "UserSearch" ON "mod-repo"."Users" USING btree (name text_pattern_ops) INCLUDE (id);


--
-- TOC entry 2926 (class 1259 OID 16973)
-- Name: Users_Groups_Joiner_index; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Users_Groups_Joiner_index" ON "mod-repo"."Users_Groups_Joiner" USING btree ("user", "group");


--
-- TOC entry 2933 (class 2606 OID 16974)
-- Name: Downloads Downloads_mod_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Downloads"
    ADD CONSTRAINT "Downloads_mod_fkey" FOREIGN KEY (mod) REFERENCES "mod-repo"."Mods"(uuid) MATCH FULL;


--
-- TOC entry 2934 (class 2606 OID 16979)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_gameVersion_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_gameVersion_fkey" FOREIGN KEY ("gameVersion") REFERENCES "mod-repo"."GameVersions"(id) MATCH FULL;


--
-- TOC entry 2935 (class 2606 OID 16984)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_group_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_group_fkey" FOREIGN KEY ("group") REFERENCES "mod-repo"."Groups"(id) MATCH FULL;


--
-- TOC entry 2938 (class 2606 OID 16989)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_mod_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_mod_fkey" FOREIGN KEY (mod) REFERENCES "mod-repo"."Mods"(uuid) MATCH FULL;


--
-- TOC entry 2939 (class 2606 OID 16994)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_tag_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_tag_fkey" FOREIGN KEY (tag) REFERENCES "mod-repo"."Tags"(id) MATCH FULL;


--
-- TOC entry 2936 (class 2606 OID 16999)
-- Name: Mods Mods_author_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_author_fkey" FOREIGN KEY (author) REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2937 (class 2606 OID 17004)
-- Name: Mods Mods_gameVersion_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_gameVersion_fkey" FOREIGN KEY ("gameVersion") REFERENCES "mod-repo"."GameVersions"(id) MATCH FULL;


--
-- TOC entry 2940 (class 2606 OID 17009)
-- Name: News News_author_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."News"
    ADD CONSTRAINT "News_author_fkey" FOREIGN KEY (author) REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2941 (class 2606 OID 17014)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_group_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_group_fkey" FOREIGN KEY ("group") REFERENCES "mod-repo"."Groups"(id) MATCH FULL;


--
-- TOC entry 2942 (class 2606 OID 17019)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_user_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_user_fkey" FOREIGN KEY ("user") REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2943 (class 2606 OID 17024)
-- Name: Tokens Tokens_user_fkey; Type: FK CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Tokens"
    ADD CONSTRAINT "Tokens_user_fkey" FOREIGN KEY ("user") REFERENCES "mod-repo"."Users"(id) MATCH FULL;


-- Completed on 2019-07-07 02:21:43

--
-- PostgreSQL database dump complete
--

