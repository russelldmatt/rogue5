/*
 * Copyright (c) 1988 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Timothy C. Stoehr.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
static char sccsid[] = "@(#)score.c	5.5 (Berkeley) 6/1/90";
#endif /* not lint */

/*
 * score.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include "rogue.h"
#include "pathnames.h"

extern char login_name[];
extern char *m_names[];
extern short max_level;
extern boolean score_only, no_skull, msg_cleared;
extern char *byebye_string, *nick_name, *score_file, *score_dir;

killed_by(monster, other)
object *monster;
short other;
{
	char buf[128];

	md_ignore_signals();

	if (other != QUIT) {
		rogue.gold = ((rogue.gold * 9) / 10);
	}

	if (other) {
		switch(other) {
		case HYPOTHERMIA:
			(void) strcpy(buf, "died of hypothermia");
			break;
		case STARVATION:
			(void) strcpy(buf, "died of starvation");
			break;
		case POISON_DART:
			(void) strcpy(buf, "killed by a dart");
			break;
		case QUIT:
			(void) strcpy(buf, "quit");
			break;
		case KFIRE:
			(void) strcpy(buf, "killed by fire");
			break;
		}
	} else {
		(void) strcpy(buf, "Killed by ");
		if (is_vowel(m_names[monster->m_char - 'A'][0])) {
			(void) strcat(buf, "an ");
		} else {
			(void) strcat(buf, "a ");
		}
		(void) strcat(buf, m_names[monster->m_char - 'A']);
	}
	(void) strcat(buf, " with ");
	sprintf(buf+strlen(buf), "%ld gold", rogue.gold);
	if ((!other) && (!no_skull)) {
		clear();
		mvaddstr(4, 32, "__---------__");
		mvaddstr(5, 30, "_~             ~_");
		mvaddstr(6, 29, "/                 \\");
		mvaddstr(7, 28, "~                   ~");
		mvaddstr(8, 27, "/                     \\");
		mvaddstr(9, 27, "|    XXXX     XXXX    |");
		mvaddstr(10, 27, "|    XXXX     XXXX    |");
		mvaddstr(11, 27, "|    XXX       XXX    |");
		mvaddstr(12, 28, "\\         @         /");
		mvaddstr(13, 29, "--\\     @@@     /--");
		mvaddstr(14, 30, "| |    @@@    | |");
		mvaddstr(15, 30, "| |           | |");
		mvaddstr(16, 30, "| vvVvvvvvvvVvv |");
		mvaddstr(17, 30, "|  ^^^^^^^^^^^  |");
		mvaddstr(18, 31, "\\_           _/");
		mvaddstr(19, 33, "~---------~");
		center(21, nick_name);
		center(22, buf);
	} else {
		message(buf, 0);
	}
	message("", 0);
	put_scores(monster, other);
}

win()
{
	unwield(rogue.weapon);		/* disarm and relax */
	unwear(rogue.armor);
	un_put_on(rogue.left_ring);
	un_put_on(rogue.right_ring);

	clear();
	mvaddstr(10, 11, "@   @  @@@   @   @      @  @  @   @@@   @   @   @");
	mvaddstr(11, 11, " @ @  @   @  @   @      @  @  @  @   @  @@  @   @");
	mvaddstr(12, 11, "  @   @   @  @   @      @  @  @  @   @  @ @ @   @");
	mvaddstr(13, 11, "  @   @   @  @   @      @  @  @  @   @  @  @@");
	mvaddstr(14, 11, "  @    @@@    @@@        @@ @@    @@@   @   @   @");
	mvaddstr(17, 11, "Congratulations,  you have  been admitted  to  the");
	mvaddstr(18, 11, "Fighters' Guild.   You return home,  sell all your");
	mvaddstr(19, 11, "treasures at great profit and retire into comfort.");
	message("", 0);
	message("", 0);
	id_all();
	sell_pack();
	put_scores((object *) 0, WIN);
}

quit(from_intrpt)
boolean from_intrpt;
{
	char buf[128];
	short i, orow, ocol;
	boolean mc;

	md_ignore_signals();

	if (from_intrpt) {
		orow = rogue.row;
		ocol = rogue.col;

		mc = msg_cleared;

		for (i = 0; i < DCOLS; i++) {
			buf[i] = mvinch(0, i);
		}
	}
	check_message();
	message("really quit?", 1);
	if (rgetchar() != 'y') {
		md_heed_signals();
		check_message();
		if (from_intrpt) {
			for (i = 0; i < DCOLS; i++) {
				mvaddch(0, i, buf[i]);
			}
			msg_cleared = mc;
			move(orow, ocol);
			refresh();
		}
		return;
	}
	if (from_intrpt) {
		clean_up(byebye_string);
	}
	check_message();
	killed_by((object *) 0, QUIT);
}

#define SCORES_PER_PLAYER 5
#define SCORES_PER_PAGE 10

typedef struct {
        char score[82];
        char name[30];
        long value;
        boolean is_new;
        long order;
} score_entry;

static long
score_value(score)
char *score;
{
        short x = 5;

        while (score[x] == ' ') {
                x++;
        }

        return lget_number(score + x);
}

static int
compare_scores(a, b)
const void *a;
const void *b;
{
        const score_entry *sa = (const score_entry *) a;
        const score_entry *sb = (const score_entry *) b;

        if (sa->value > sb->value) {
                return -1;
        }

        if (sa->value < sb->value) {
                return 1;
        }

        /*
         * Prefer the newly-added score on an exact tie.
         */
        if (sa->is_new && !sb->is_new) {
                return -1;
        }

        if (!sa->is_new && sb->is_new) {
                return 1;
        }

        /*
         * Otherwise preserve the previous order for ties.
         */
        if (sa->order < sb->order) {
                return -1;
        }

        if (sa->order > sb->order) {
                return 1;
        }

        return 0;
}

static int
trim_scores(entries, ne)
score_entry *entries;
int ne;
{
        int i, j;
        int kept = 0;
        int player_count;

        /*
         * entries are globally sorted highest-to-lowest.
         * Therefore the first 5 encountered for each name
         * are that player's best 5.
         */
        for (i = 0; i < ne; i++) {
                player_count = 0;

                for (j = 0; j < kept; j++) {
                        if (!strcmp(entries[i].name, entries[j].name)) {
                                player_count++;
                        }
                }

                if (player_count < SCORES_PER_PLAYER) {
                        if (kept != i) {
                                entries[kept] = entries[i];
                        }
                        kept++;
                }
        }

        return kept;
}

static void
set_score_rank(score, rank)
char *score;
int rank;
{
        if (rank < 10) {
                score[0] = ' ';
                score[1] = '0' + rank;
        } else if (rank < 100) {
                score[0] = '0' + (rank / 10);
                score[1] = '0' + (rank % 10);
        } else {
                /*
                 * The historical score record only reserves
                 * two characters for rank.
                 */
                score[0] = '*';
                score[1] = '*';
        }
}

static void
show_scores(entries, ne)
score_entry *entries;
int ne;
{
        int start, end, i, row;
        char buf[128];
        char display_score[82];

        if (ne == 0) {
                clear();
                center(3, "Top Rogueists");
                mvaddstr(8, 0, "Rank   Score   Name");
                refresh();
                return;
        }

        for (start = 0; start < ne; start += SCORES_PER_PAGE) {
                clear();
                center(3, "Top Rogueists");
                mvaddstr(8, 0, "Rank   Score   Name");

                end = start + SCORES_PER_PAGE;
                if (end > ne) {
                        end = ne;
                }

                row = 10;

                for (i = start; i < end; i++) {
                        (void) strcpy(display_score, entries[i].score);
                        set_score_rank(display_score, i + 1);

                        if (entries[i].is_new) {
                                standout();
                        }

                        nickize(buf, display_score, entries[i].name);
                        mvaddstr(row++, 0, buf);

                        if (entries[i].is_new) {
                                standend();
                        }
                }

                if (end < ne) {
                        mvaddstr(22, 0,
                                "-- press any key for more scores --");
                        refresh();
                        (void) rgetchar();
                } else {
                        refresh();
                }
        }
}

static void
show_scores_stdout(entries, ne)
score_entry *entries;
int ne;
{
        int i;
        char buf[128];
        char display_score[82];

        printf("Top Rogueists\n\n");
        printf("Rank   Score   Name\n\n");

        for (i = 0; i < ne; i++) {
                (void) strcpy(display_score, entries[i].score);
                set_score_rank(display_score, i + 1);

                nickize(buf, display_score, entries[i].name);
                printf("%s\n", buf);
        }
}

static void
strip_newline(s)
char *s;
{
        int n;

        n = strlen(s);

        while ((n > 0) &&
               ((s[n-1] == '\n') || (s[n-1] == '\r'))) {
                s[--n] = 0;
        }
}

static void
append_entry(entries, ne, capacity, score, name, value, is_new)
score_entry **entries;
int *ne, *capacity;
char *score, *name;
long value;
boolean is_new;
{
        int new_capacity;
        score_entry *tmp;

        if (*ne == *capacity) {
                new_capacity = *capacity ? *capacity * 2 : 16;

                tmp = (score_entry *) realloc(
                        *entries,
                        new_capacity * sizeof(score_entry));

                if (tmp == NULL) {
                        clean_up("out of memory reading scores");
                }

                *entries = tmp;
                *capacity = new_capacity;
        }

        (void) strncpy((*entries)[*ne].score, score, 81);
        (*entries)[*ne].score[81] = 0;

        (void) strncpy((*entries)[*ne].name, name, 29);
        (*entries)[*ne].name[29] = 0;

        (*entries)[*ne].value = value;
        (*entries)[*ne].is_new = is_new;
        (*entries)[*ne].order = *ne;

        (*ne)++;
}

static boolean
same_score_exists(entries, ne, score, name)
score_entry *entries;
int ne;
char *score, *name;
{
        int i;

        for (i = 0; i < ne; i++) {
                /*
                 * Ignore the first two characters because those
                 * contain the displayed rank, which can change.
                 */
                if ((!strcmp(entries[i].name, name)) &&
                    (!strcmp(entries[i].score + 2, score + 2))) {
                        return 1;
                }
        }

        return 0;
}

static void
load_score_events(entries, ne, capacity)
score_entry **entries;
int *ne, *capacity;
{
        DIR *dir;
        struct dirent *de;
        FILE *fp;

        char path[MAX_OPT_LEN + 256];
        char line[256];

        char name[30];
        char text[82];
        char score[82];

        long value;
        int version;
        int len;
        int text_len;

        dir = opendir(score_dir);

        if (dir == NULL) {
                if (errno == ENOENT) {
                        return;
                }

                clean_up("cannot read score directory");
        }

        while ((de = readdir(dir)) != NULL) {
                len = strlen(de->d_name);

                /*
                 * Ignore temporary files and anything that isn't
                 * one of our .score files.
                 */
                if ((len < 7) ||
                    strcmp(de->d_name + len - 6, ".score")) {
                        continue;
                }

                sprintf(path, "%s/%s", score_dir, de->d_name);

                fp = fopen(path, "r");

                if (fp == NULL) {
                        continue;
                }

                version = 0;
                value = -1;
                name[0] = 0;
                text[0] = 0;

                while (fgets(line, sizeof(line), fp) != NULL) {
                        strip_newline(line);

                        if (!strncmp(line, "version=", 8)) {
                                version = atoi(line + 8);

                        } else if (!strncmp(line, "score=", 6)) {
                                value = atol(line + 6);

                        } else if (!strncmp(line, "name=", 5)) {
                                (void) strncpy(name, line + 5, 29);
                                name[29] = 0;

                        } else if (!strncmp(line, "text=", 5)) {
                                (void) strncpy(text, line + 5, 81);
                                text[81] = 0;
                        }
                }

                fclose(fp);

                /*
                 * Ignore malformed or future-version files rather
                 * than making one bad iCloud file break -s.
                 */
                if ((version != 1) ||
                    (value < 0) ||
                    (!name[0]) ||
                    (!text[0])) {
                        continue;
                }

                /*
                 * Convert the human-readable text back into Rogue's
                 * traditional padded 80-byte score string.
                 */
                (void) memset(score, ' ', 79);
                score[79] = 0;
                score[80] = 0;
                score[81] = 0;

                text_len = strlen(text);
                if (text_len > 79) {
                        text_len = 79;
                }

                (void) memcpy(score, text, text_len);

                append_entry(
                        entries,
                        ne,
                        capacity,
                        score,
                        name,
                        value,
                        0);
        }

        closedir(dir);
}

static unsigned long long
legacy_score_hash(score, name)
char *score, *name;
{
        unsigned long long h;
        int i;

        /*
         * FNV-1a. Ignore the first two score characters because
         * they contain the displayed rank, which can change.
         */
        h = 14695981039346656037ULL;

        for (i = 2; i < 80; i++) {
                h ^= (unsigned char) score[i];
                h *= 1099511628211ULL;
        }

        /*
         * Separator between score text and nickname.
         */
        h ^= 0xff;
        h *= 1099511628211ULL;

        for (i = 0; (i < 29) && name[i]; i++) {
                h ^= (unsigned char) name[i];
                h *= 1099511628211ULL;
        }

        return h;
}

static void
write_plain_score_file(tmp_path, final_path, score, name)
char *tmp_path, *final_path, *score, *name;
{
        char stored_name[30];
        char text[82];
        FILE *fp;
        int i;

        (void) strncpy(stored_name, name, 29);
        stored_name[29] = 0;

        for (i = 0; stored_name[i]; i++) {
                if ((stored_name[i] == '\n') ||
                    (stored_name[i] == '\r')) {
                        stored_name[i] = '_';
                }
        }

        (void) strncpy(text, score, 81);
        text[81] = 0;

        i = strlen(text) - 1;
        while ((i >= 0) && (text[i] == ' ')) {
                text[i--] = 0;
        }

        fp = fopen(tmp_path, "w");
        if (fp == NULL) {
                clean_up("cannot create score event");
        }

        if (fprintf(fp,
                "version=1\n"
                "score=%ld\n"
                "name=%s\n"
                "text=%s\n",
                score_value(score),
                stored_name,
                text) < 0) {
                fclose(fp);
                unlink(tmp_path);
                clean_up("cannot write score event");
        }

        if (fclose(fp) != 0) {
                unlink(tmp_path);
                clean_up("cannot close score event");
        }

        if (rename(tmp_path, final_path) != 0) {
                unlink(tmp_path);
                clean_up("cannot publish score event");
        }
}

static void
write_score_event(score, name)
char *score, *name;
{
        char tmp_path[MAX_OPT_LEN + 128];
        char final_path[MAX_OPT_LEN + 128];
        char safe_name[30];
	time_t now;
	struct tm *tm;
	char timestamp[32];
        long random_part;
        int i;

        if ((mkdir(score_dir, 0755) < 0) && (errno != EEXIST)) {
                clean_up("cannot create score directory");
        }

        /*
         * Make a filename-safe version of the nickname.
         */
        for (i = 0; (i < 29) && name[i]; i++) {
                if ((name[i] >= 'a' && name[i] <= 'z') ||
                    (name[i] >= 'A' && name[i] <= 'Z') ||
                    (name[i] >= '0' && name[i] <= '9') ||
                    name[i] == '-' || name[i] == '_') {
                        safe_name[i] = name[i];
                } else {
                        safe_name[i] = '_';
                }
        }
        safe_name[i] = 0;

	now = time((time_t *) 0);
	tm = localtime(&now);

	if (tm == NULL ||
	    strftime(timestamp, sizeof(timestamp),
		     "%Y%m%d-%H%M%S", tm) == 0) {
	  clean_up("cannot format score timestamp");
	}

	random_part = rrandom() & 0xffff;

	sprintf(tmp_path,
		"%s/.tmp-%s-%ld-%04lx",
		score_dir,
		timestamp,
		(long) getpid(),
		random_part);

	sprintf(final_path,
		"%s/score-%s-%s-%ld-%04lx.score",
		score_dir,
		timestamp,
		safe_name,
		(long) getpid(),
		random_part);

	write_plain_score_file(tmp_path, final_path, score, name);
}

static void
write_legacy_score_event(score, name)
char *score, *name;
{
        char tmp_path[MAX_OPT_LEN + 128];
        char final_path[MAX_OPT_LEN + 128];
        unsigned long long hash;
        long random_part;

        if ((mkdir(score_dir, 0755) < 0) && (errno != EEXIST)) {
                clean_up("cannot create score directory");
        }

        hash = legacy_score_hash(score, name);

        sprintf(final_path,
                "%s/legacy-%016llx.score",
                score_dir,
                hash);

        /*
         * Already safely migrated.
         */
        if (access(final_path, F_OK) == 0) {
                return;
        }

        random_part = rrandom() & 0xffff;

        sprintf(tmp_path,
                "%s/.tmp-legacy-%016llx-%ld-%04lx",
                score_dir,
                hash,
                (long) getpid(),
                random_part);

        write_plain_score_file(
                tmp_path,
                final_path,
                score,
                name);
}

put_scores(monster, other)
object *monster;
short other;
{
        int i, n;
        int ne = 0;
        int capacity = 0;

        score_entry *entries = (score_entry *) 0;

        char raw_score[82];
        char raw_name[30];

        char new_scores[1][82];
        char new_names[1][30];

        FILE *fp;

        md_lock(1);

	/*
	 * New append-only score events are our preferred source.
	 */
	load_score_events(&entries, &ne, &capacity);

	/*
	 * Read the legacy shared score file, if it exists.
	 * New versions never create or modify it.
	 */
	fp = fopen(score_file, "r");

	if (fp != NULL) {
	  rewind(fp);
	  (void) xxx(1);

	  for (;;) {
	    n = fread(raw_score, sizeof(char), 80, fp);

	    if (n == 0) {
	      break;
	    }

	    if (n < 80) {
	      break;
	    }

	    xxxx(raw_score, 80);

	    n = fread(raw_name, sizeof(char), 30, fp);

	    if (n < 30) {
	      break;
	    }

	    xxxx(raw_name, 30);

	    if (!same_score_exists(entries, ne,
				   raw_score, raw_name)) {
	      write_legacy_score_event(raw_score, raw_name);

	      append_entry(
			   &entries,
			   &ne,
			   &capacity,
			   raw_score,
			   raw_name,
			   score_value(raw_score),
			   0);
	    }
	  }

	  fclose(fp);

	} else if (errno != ENOENT) {
	  message("cannot read legacy score file", 0);
	  sf_error();
	}

        /*
         * Add the current run.
         */
        if (!score_only) {
                (void) memset(new_scores, 0, sizeof(new_scores));
                (void) memset(new_names, 0, sizeof(new_names));

                /*
                 * Reuse Rogue's existing formatting code.
                 * With n == 0, insert_score() simply creates
                 * one score and does not shift anything.
                 */
                insert_score(new_scores, new_names, nick_name,
                        0, 0, monster, other);

		/*
		 * Save an immutable copy of this individual run.
		 */
		write_score_event(new_scores[0], new_names[0]);

		append_entry(
			     &entries,
			     &ne,
			     &capacity,
			     new_scores[0],
			     new_names[0],
			     rogue.gold,
			     1);
        }

        /*
         * Interleave everybody globally by score.
         */
        if (ne > 1) {
                qsort(entries, ne,
                        sizeof(score_entry), compare_scores);
        }

        /*
         * But allow each username to contribute at most
         * its own 5 best scores.
         */
        ne = trim_scores(entries, ne);

        /*
         * Update the stored global rank numbers.
         */
        for (i = 0; i < ne; i++) {
                set_score_rank(entries[i].score, i + 1);
        }

	if (score_only) {
	  show_scores_stdout(entries, ne);

	  md_lock(0);

	  if (entries != (score_entry *) 0) {
	    free(entries);
	  }

	  md_exit(0);
	}

	md_ignore_signals();

	/*
	 * Display scores after a completed game.
	 */
	show_scores(entries, ne);

	md_lock(0);

	if (entries != (score_entry *) 0) {
	  free(entries);
	}

	message("", 0);
	clean_up("");
}

insert_score(scores, n_names, n_name, rank, n, monster, other)
char scores[][82];
char n_names[][30];
char *n_name;
short rank, n;
object *monster;
{
	short i;
	char buf[128];

	if (n > 0) {
		for (i = n; i > rank; i--) {
			if ((i < 10) && (i > 0)) {
				(void) strcpy(scores[i], scores[i-1]);
				(void) strcpy(n_names[i], n_names[i-1]);
			}
		}
	}
	sprintf(buf, "%2d    %6d   %s: ", rank+1, rogue.gold, login_name);

	if (other) {
		switch(other) {
		case HYPOTHERMIA:
			(void) strcat(buf, "died of hypothermia");
			break;
		case STARVATION:
			(void) strcat(buf, "died of starvation");
			break;
		case POISON_DART:
			(void) strcat(buf, "killed by a dart");
			break;
		case QUIT:
			(void) strcat(buf, "quit");
			break;
		case WIN:
			(void) strcat(buf, "a total winner");
			break;
		case KFIRE:
			(void) strcat(buf, "killed by fire");
			break;
		}
	} else {
		(void) strcat(buf, "killed by ");
		if (is_vowel(m_names[monster->m_char - 'A'][0])) {
			(void) strcat(buf, "an ");
		} else {
			(void) strcat(buf, "a ");
		}
		(void) strcat(buf, m_names[monster->m_char - 'A']);
	}
	sprintf(buf+strlen(buf), " on level %d ",  max_level);
	if ((other != WIN) && has_amulet()) {
		(void) strcat(buf, "with amulet");
	}
	for (i = strlen(buf); i < 79; i++) {
		buf[i] = ' ';
	}
	buf[79] = 0;
	(void) strcpy(scores[rank], buf);
	(void) strncpy(n_names[rank], n_name, 29);
	n_names[rank][29] = 0;
}

is_vowel(ch)
short ch;
{
	return( (ch == 'a') ||
		(ch == 'e') ||
		(ch == 'i') ||
		(ch == 'o') ||
		(ch == 'u') );
}

sell_pack()
{
	object *obj;
	short row = 2, val;
	char buf[DCOLS];

	obj = rogue.pack.next_object;

	clear();
	mvaddstr(1, 0, "Value      Item");

	while (obj) {
		if (obj->what_is != FOOD) {
			obj->identified = 1;
			val = get_value(obj);
			rogue.gold += val;

			if (row < DROWS) {
				sprintf(buf, "%5d      ", val);
				get_desc(obj, buf+11);
				mvaddstr(row++, 0, buf);
			}
		}
		obj = obj->next_object;
	}
	refresh();
	if (rogue.gold > MAX_GOLD) {
		rogue.gold = MAX_GOLD;
	}
	message("", 0);
}

get_value(obj)
object *obj;
{
	short wc;
	int val;

	wc = obj->which_kind;

	switch(obj->what_is) {
	case WEAPON:
		val = id_weapons[wc].value;
		if ((wc == ARROW) || (wc == DAGGER) || (wc == SHURIKEN) ||
			(wc == DART)) {
			val *= obj->quantity;
		}
		val += (obj->d_enchant * 85);
		val += (obj->hit_enchant * 85);
		break;
	case ARMOR:
		val = id_armors[wc].value;
		val += (obj->d_enchant * 75);
		if (obj->is_protected) {
			val += 200;
		}
		break;
	case WAND:
		val = id_wands[wc].value * (obj->class + 1);
		break;
	case SCROL:
		val = id_scrolls[wc].value * obj->quantity;
		break;
	case POTION:
		val = id_potions[wc].value * obj->quantity;
		break;
	case AMULET:
		val = 5000;
		break;
	case RING:
		val = id_rings[wc].value * (obj->class + 1);
		break;
	}
	if (val <= 0) {
		val = 10;
	}
	return(val);
}

id_all()
{
	short i;

	for (i = 0; i < SCROLS; i++) {
		id_scrolls[i].id_status = IDENTIFIED;
	}
	for (i = 0; i < WEAPONS; i++) {
		id_weapons[i].id_status = IDENTIFIED;
	}
	for (i = 0; i < ARMORS; i++) {
		id_armors[i].id_status = IDENTIFIED;
	}
	for (i = 0; i < WANDS; i++) {
		id_wands[i].id_status = IDENTIFIED;
	}
	for (i = 0; i < POTIONS; i++) {
		id_potions[i].id_status = IDENTIFIED;
	}
}

name_cmp(s1, s2)
char *s1, *s2;
{
	short i = 0;
	int r;

	while(s1[i] != ':') {
		i++;
	}
	s1[i] = 0;
	r = strcmp(s1, s2);
	s1[i] = ':';
	return(r);
}

xxxx(buf, n)
char *buf;
short n;
{
	short i;
	unsigned char c;

	for (i = 0; i < n; i++) {

		/* It does not matter if accuracy is lost during this assignment */
		c = (unsigned char) xxx(0);

		buf[i] ^= c;
	}
}

long
xxx(st)
boolean st;
{
	static long f, s;
	long r;

	if (st) {
		f = 37;
		s = 7;
		return(0L);
	}
	r = ((f * s) + 9337) % 8887;
	f = s;
	s = r;
	return(r);
}

nickize(buf, score, n_name)
char *buf, *score, *n_name;
{
	short i = 15, j;

	if (!n_name[0]) {
		(void) strcpy(buf, score);
	} else {
		(void) strncpy(buf, score, 16);

		while (score[i] != ':') {
			i++;
		}

		(void) strcpy(buf+15, n_name);
		j = strlen(buf);

		while (score[i]) {
			buf[j++] = score[i++];
		}
		buf[j] = 0;
		buf[79] = 0;
	}
}

center(row, buf)
short row;
char *buf;
{
	short margin;

	margin = ((DCOLS - strlen(buf)) / 2);
	mvaddstr(row, margin, buf);
}

sf_error()
{
	md_lock(0);
	message("", 1);
	clean_up("sorry, score file is out of order");
}
