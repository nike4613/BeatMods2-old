--
-- PostgreSQL database dump
--

-- Dumped from database version 11.3
-- Dumped by pg_dump version 11.2

-- Started on 2019-06-19 16:17:16

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 3002 (class 1262 OID 16394)
-- Name: beatmods-2; Type: DATABASE; Schema: -; Owner: -
--

CREATE DATABASE "beatmods-2" WITH TEMPLATE = template0 ENCODING = 'UTF8' LC_COLLATE = 'POSIX' LC_CTYPE = 'POSIX';


\connect -reuse-previous=on "dbname='beatmods-2'"

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET client_min_messages = warning;
SET row_security = off;

--
-- TOC entry 11 (class 2615 OID 16395)
-- Name: mod-repo; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA "mod-repo";


--
-- TOC entry 8 (class 2615 OID 16697)
-- Name: server-state; Type: SCHEMA; Schema: -; Owner: -
--

CREATE SCHEMA "server-state";


--
-- TOC entry 1 (class 3079 OID 16384)
-- Name: adminpack; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS adminpack WITH SCHEMA pg_catalog;


--
-- TOC entry 3003 (class 0 OID 0)
-- Dependencies: 1
-- Name: EXTENSION adminpack; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION adminpack IS 'administrative functions for PostgreSQL';


--
-- TOC entry 3 (class 3079 OID 16470)
-- Name: pgcrypto; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS pgcrypto WITH SCHEMA public;


--
-- TOC entry 3004 (class 0 OID 0)
-- Dependencies: 3
-- Name: EXTENSION pgcrypto; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION pgcrypto IS 'cryptographic functions';


--
-- TOC entry 4 (class 3079 OID 16459)
-- Name: uuid-ossp; Type: EXTENSION; Schema: -; Owner: -
--

CREATE EXTENSION IF NOT EXISTS "uuid-ossp" WITH SCHEMA public;


--
-- TOC entry 3005 (class 0 OID 0)
-- Dependencies: 4
-- Name: EXTENSION "uuid-ossp"; Type: COMMENT; Schema: -; Owner: -
--

COMMENT ON EXTENSION "uuid-ossp" IS 'generate universally unique identifiers (UUIDs)';


--
-- TOC entry 661 (class 1247 OID 16413)
-- Name: Approval; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Approval" AS ENUM (
    'Approved',
    'Declined',
    'Pending',
    'Inactive'
);


--
-- TOC entry 667 (class 1247 OID 16423)
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
-- TOC entry 693 (class 1247 OID 16589)
-- Name: DownloadType; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."DownloadType" AS ENUM (
    'Steam',
    'Oculus',
    'Universal'
);


--
-- TOC entry 658 (class 1247 OID 16411)
-- Name: ModRange; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."ModRange" AS (
	id character varying(128),
	"versionRange" character varying(64)
);


--
-- TOC entry 690 (class 1247 OID 16562)
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
-- TOC entry 710 (class 1247 OID 16660)
-- Name: System; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."System" AS ENUM (
    'PC',
    'Quest'
);


--
-- TOC entry 664 (class 1247 OID 16421)
-- Name: Version; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Version" AS (
	major integer,
	minor integer,
	patch integer,
	prerelease_ids text[],
	build_ids text[]
);


--
-- TOC entry 682 (class 1247 OID 16545)
-- Name: Visibility; Type: TYPE; Schema: mod-repo; Owner: -
--

CREATE TYPE "mod-repo"."Visibility" AS ENUM (
    'Public',
    'Groups'
);


--
-- TOC entry 726 (class 1247 OID 16710)
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
-- TOC entry 276 (class 1255 OID 16595)
-- Name: add_to_enum(regtype, name); Type: FUNCTION; Schema: mod-repo; Owner: -
--

CREATE FUNCTION "mod-repo".add_to_enum(enum_type regtype, new_value name) RETURNS void
    LANGUAGE sql
    AS $$INSERT INTO pg_enum (enumtypid, enumlabel, enumsortorder)
    SELECT enum_type::oid, new_value, ( SELECT MAX(enumsortorder) + 1 FROM pg_enum WHERE enumtypid = enum_type );
$$;


--
-- TOC entry 277 (class 1255 OID 16669)
-- Name: remove_from_enum(regtype, name); Type: FUNCTION; Schema: mod-repo; Owner: -
--

CREATE FUNCTION "mod-repo".remove_from_enum(enum_type regtype, element name) RETURNS void
    LANGUAGE sql
    AS $$DELETE FROM pg_enum
	WHERE 		pg_enum.enumtypid = enum_type::oid 
			AND pg_enum.enumlabel = element;$$;


SET default_tablespace = '';

SET default_with_oids = false;

--
-- TOC entry 208 (class 1259 OID 16598)
-- Name: Downloads; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Downloads" (
    mod uuid NOT NULL,
    type "mod-repo"."DownloadType" NOT NULL,
    "cdnFile" text NOT NULL,
    hashes json NOT NULL
);


--
-- TOC entry 209 (class 1259 OID 16612)
-- Name: GameVersion_VisibleGroups_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."GameVersion_VisibleGroups_Joiner" (
    "gameVersion" uuid NOT NULL,
    "group" bigint NOT NULL
);


--
-- TOC entry 205 (class 1259 OID 16530)
-- Name: GameVersions; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."GameVersions" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    version "mod-repo"."Version" NOT NULL,
    "steamBuildId" bigint NOT NULL,
    visibility "mod-repo"."Visibility" NOT NULL
);


--
-- TOC entry 207 (class 1259 OID 16552)
-- Name: Groups; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Groups" (
    id bigint NOT NULL,
    name text NOT NULL,
    permissions "mod-repo"."Permission"[] NOT NULL
);


--
-- TOC entry 206 (class 1259 OID 16550)
-- Name: Groups_id_seq; Type: SEQUENCE; Schema: mod-repo; Owner: -
--

CREATE SEQUENCE "mod-repo"."Groups_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3006 (class 0 OID 0)
-- Dependencies: 206
-- Name: Groups_id_seq; Type: SEQUENCE OWNED BY; Schema: mod-repo; Owner: -
--

ALTER SEQUENCE "mod-repo"."Groups_id_seq" OWNED BY "mod-repo"."Groups".id;


--
-- TOC entry 203 (class 1259 OID 16435)
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
-- TOC entry 214 (class 1259 OID 16682)
-- Name: Mods_Tags_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Mods_Tags_Joiner" (
    mod uuid NOT NULL,
    tag bigint NOT NULL
);


--
-- TOC entry 211 (class 1259 OID 16645)
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
-- TOC entry 213 (class 1259 OID 16673)
-- Name: Tags; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Tags" (
    id bigint NOT NULL,
    name text NOT NULL
);


--
-- TOC entry 212 (class 1259 OID 16671)
-- Name: Tags_id_seq; Type: SEQUENCE; Schema: mod-repo; Owner: -
--

CREATE SEQUENCE "mod-repo"."Tags_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3007 (class 0 OID 0)
-- Dependencies: 212
-- Name: Tags_id_seq; Type: SEQUENCE OWNED BY; Schema: mod-repo; Owner: -
--

ALTER SEQUENCE "mod-repo"."Tags_id_seq" OWNED BY "mod-repo"."Tags".id;


--
-- TOC entry 204 (class 1259 OID 16507)
-- Name: Users; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Users" (
    id uuid DEFAULT public.uuid_generate_v4() NOT NULL,
    name text NOT NULL,
    profile text DEFAULT '' NOT NULL,
    created timestamp with time zone DEFAULT now() NOT NULL,
    "githubId" text NOT NULL
);


--
-- TOC entry 210 (class 1259 OID 16629)
-- Name: Users_Groups_Joiner; Type: TABLE; Schema: mod-repo; Owner: -
--

CREATE TABLE "mod-repo"."Users_Groups_Joiner" (
    "user" uuid NOT NULL,
    "group" bigint NOT NULL
);


--
-- TOC entry 216 (class 1259 OID 16700)
-- Name: Log; Type: TABLE; Schema: server-state; Owner: -
--

CREATE TABLE "server-state"."Log" (
    id bigint NOT NULL,
    "time" timestamp with time zone NOT NULL,
    message text NOT NULL,
    level "server-state"."LogLevel"
);


--
-- TOC entry 215 (class 1259 OID 16698)
-- Name: Log_id_seq; Type: SEQUENCE; Schema: server-state; Owner: -
--

CREATE SEQUENCE "server-state"."Log_id_seq"
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


--
-- TOC entry 3008 (class 0 OID 0)
-- Dependencies: 215
-- Name: Log_id_seq; Type: SEQUENCE OWNED BY; Schema: server-state; Owner: -
--

ALTER SEQUENCE "server-state"."Log_id_seq" OWNED BY "server-state"."Log".id;


--
-- TOC entry 217 (class 1259 OID 16723)
-- Name: Tokens; Type: TABLE; Schema: server-state; Owner: -
--

CREATE TABLE "server-state"."Tokens" (
    "user" uuid NOT NULL,
    token text NOT NULL
);


--
-- TOC entry 2828 (class 2604 OID 16555)
-- Name: Groups id; Type: DEFAULT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Groups" ALTER COLUMN id SET DEFAULT nextval('"mod-repo"."Groups_id_seq"'::regclass);


--
-- TOC entry 2830 (class 2604 OID 16676)
-- Name: Tags id; Type: DEFAULT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Tags" ALTER COLUMN id SET DEFAULT nextval('"mod-repo"."Tags_id_seq"'::regclass);


--
-- TOC entry 2831 (class 2604 OID 16703)
-- Name: Log id; Type: DEFAULT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Log" ALTER COLUMN id SET DEFAULT nextval('"server-state"."Log_id_seq"'::regclass);


--
-- TOC entry 2847 (class 2606 OID 16605)
-- Name: Downloads Downloads_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Downloads"
    ADD CONSTRAINT "Downloads_pkey" PRIMARY KEY (mod, type);


--
-- TOC entry 2850 (class 2606 OID 16616)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_pkey" PRIMARY KEY ("gameVersion", "group");


--
-- TOC entry 2841 (class 2606 OID 16538)
-- Name: GameVersions GameVersions_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersions"
    ADD CONSTRAINT "GameVersions_pkey" PRIMARY KEY (id);


--
-- TOC entry 2844 (class 2606 OID 16560)
-- Name: Groups Groups_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Groups"
    ADD CONSTRAINT "Groups_pkey" PRIMARY KEY (id);


--
-- TOC entry 2860 (class 2606 OID 16686)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_pkey" PRIMARY KEY (mod, tag);


--
-- TOC entry 2834 (class 2606 OID 16522)
-- Name: Mods Mods_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_pkey" PRIMARY KEY (uuid);


--
-- TOC entry 2856 (class 2606 OID 16652)
-- Name: News News_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."News"
    ADD CONSTRAINT "News_pkey" PRIMARY KEY (id);


--
-- TOC entry 2858 (class 2606 OID 16681)
-- Name: Tags Tags_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Tags"
    ADD CONSTRAINT "Tags_pkey" PRIMARY KEY (id);


--
-- TOC entry 2853 (class 2606 OID 16633)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_pkey" PRIMARY KEY ("user", "group");


--
-- TOC entry 2837 (class 2606 OID 16519)
-- Name: Users Users_name_key; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users"
    ADD CONSTRAINT "Users_name_key" UNIQUE (name);


--
-- TOC entry 2839 (class 2606 OID 16516)
-- Name: Users Users_pkey; Type: CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users"
    ADD CONSTRAINT "Users_pkey" PRIMARY KEY (id);


--
-- TOC entry 2862 (class 2606 OID 16708)
-- Name: Log Log_pkey; Type: CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Log"
    ADD CONSTRAINT "Log_pkey" PRIMARY KEY (id);


--
-- TOC entry 2864 (class 2606 OID 16730)
-- Name: Tokens Tokens_pkey; Type: CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Tokens"
    ADD CONSTRAINT "Tokens_pkey" PRIMARY KEY (token);


--
-- TOC entry 2845 (class 1259 OID 16611)
-- Name: Downloads_ModIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Downloads_ModIndex" ON "mod-repo"."Downloads" USING btree (mod) INCLUDE (type, "cdnFile", hashes);


--
-- TOC entry 2848 (class 1259 OID 16628)
-- Name: GameVersion_VisibleGroups_Joiner_index; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE UNIQUE INDEX "GameVersion_VisibleGroups_Joiner_index" ON "mod-repo"."GameVersion_VisibleGroups_Joiner" USING btree ("gameVersion", "group");


--
-- TOC entry 2842 (class 1259 OID 16587)
-- Name: GroupIDindex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "GroupIDindex" ON "mod-repo"."Groups" USING btree (id);


--
-- TOC entry 2832 (class 1259 OID 16665)
-- Name: Mods_SearchIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Mods_SearchIndex" ON "mod-repo"."Mods" USING btree (name text_pattern_ops, description text_pattern_ops, "approvalState", system) INCLUDE (version, uuid, author, id, "gameVersion");


--
-- TOC entry 2854 (class 1259 OID 16658)
-- Name: News_OrderIndex; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "News_OrderIndex" ON "mod-repo"."News" USING btree (posted DESC NULLS LAST, edited DESC NULLS LAST) INCLUDE (id, title, author);


--
-- TOC entry 2835 (class 1259 OID 16517)
-- Name: UserSearch; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE UNIQUE INDEX "UserSearch" ON "mod-repo"."Users" USING btree (name text_pattern_ops) INCLUDE (id);


--
-- TOC entry 2851 (class 1259 OID 16644)
-- Name: Users_Groups_Joiner_index; Type: INDEX; Schema: mod-repo; Owner: -
--

CREATE INDEX "Users_Groups_Joiner_index" ON "mod-repo"."Users_Groups_Joiner" USING btree ("user", "group");


--
-- TOC entry 2867 (class 2606 OID 16606)
-- Name: Downloads Downloads_mod_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Downloads"
    ADD CONSTRAINT "Downloads_mod_fkey" FOREIGN KEY (mod) REFERENCES "mod-repo"."Mods"(uuid) MATCH FULL;


--
-- TOC entry 2868 (class 2606 OID 16617)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_gameVersion_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_gameVersion_fkey" FOREIGN KEY ("gameVersion") REFERENCES "mod-repo"."GameVersions"(id) MATCH FULL;


--
-- TOC entry 2869 (class 2606 OID 16622)
-- Name: GameVersion_VisibleGroups_Joiner GameVersion_VisibleGroups_Joiner_group_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."GameVersion_VisibleGroups_Joiner"
    ADD CONSTRAINT "GameVersion_VisibleGroups_Joiner_group_fkey" FOREIGN KEY ("group") REFERENCES "mod-repo"."Groups"(id) MATCH FULL;


--
-- TOC entry 2873 (class 2606 OID 16687)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_mod_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_mod_fkey" FOREIGN KEY (mod) REFERENCES "mod-repo"."Mods"(uuid) MATCH FULL;


--
-- TOC entry 2874 (class 2606 OID 16692)
-- Name: Mods_Tags_Joiner Mods_Tags_Joiner_tag_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods_Tags_Joiner"
    ADD CONSTRAINT "Mods_Tags_Joiner_tag_fkey" FOREIGN KEY (tag) REFERENCES "mod-repo"."Tags"(id) MATCH FULL;


--
-- TOC entry 2865 (class 2606 OID 16523)
-- Name: Mods Mods_author_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_author_fkey" FOREIGN KEY (author) REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2866 (class 2606 OID 16539)
-- Name: Mods Mods_gameVersion_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Mods"
    ADD CONSTRAINT "Mods_gameVersion_fkey" FOREIGN KEY ("gameVersion") REFERENCES "mod-repo"."GameVersions"(id) MATCH FULL;


--
-- TOC entry 2872 (class 2606 OID 16653)
-- Name: News News_author_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."News"
    ADD CONSTRAINT "News_author_fkey" FOREIGN KEY (author) REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2871 (class 2606 OID 16639)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_group_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_group_fkey" FOREIGN KEY ("group") REFERENCES "mod-repo"."Groups"(id) MATCH FULL;


--
-- TOC entry 2870 (class 2606 OID 16634)
-- Name: Users_Groups_Joiner Users_Groups_Joiner_user_fkey; Type: FK CONSTRAINT; Schema: mod-repo; Owner: -
--

ALTER TABLE ONLY "mod-repo"."Users_Groups_Joiner"
    ADD CONSTRAINT "Users_Groups_Joiner_user_fkey" FOREIGN KEY ("user") REFERENCES "mod-repo"."Users"(id) MATCH FULL;


--
-- TOC entry 2875 (class 2606 OID 16731)
-- Name: Tokens Tokens_user_fkey; Type: FK CONSTRAINT; Schema: server-state; Owner: -
--

ALTER TABLE ONLY "server-state"."Tokens"
    ADD CONSTRAINT "Tokens_user_fkey" FOREIGN KEY ("user") REFERENCES "mod-repo"."Users"(id) MATCH FULL;


-- Completed on 2019-06-19 16:17:17

--
-- PostgreSQL database dump complete
--

