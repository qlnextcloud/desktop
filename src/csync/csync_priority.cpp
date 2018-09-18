//
// Created by isshe on 2018/9/4.
//
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "c_lib.h"
#include "c_private.h"
#include "c_utf8.h"

#include "csync_private.h"
#include "csync_priority.h"
#include "csync_misc.h"

#include "common/utility.h"

#include <QString>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define CSYNC_LOG_CATEGORY_NAME "csync.priority"
#include "csync_log.h"

#ifndef WITH_TESTING
static
#endif
int _csync_priority_add(c_strlist_t **inList, const char *string) {
    size_t i = 0;

    // We never want duplicates, so check whether the string is already
    // in the list first.
    if (*inList) {
        for (i = 0; i < (*inList)->count; ++i) {
            char *pattern = (*inList)->vector[i];
            if (c_streq(pattern, string)) {
                return 1;
            }
        }
    }
    return c_strlist_add_grow(inList, string);
}


/* Only for bnames (not paths) */
/*
static QString convertToBnameRegexpSyntax(QString prio)
{
    QString s = QRegularExpression::escape(prio).replace("\\*", ".*").replace("\\?", ".");
    return s;
}
*/
/*
void csync_s::TraversalPrioritys::prepare(c_strlist_t *prioritys)
{
    c_strlist_destroy(list_patterns_fnmatch);
    list_patterns_fnmatch = nullptr;

    // Start out with regexes that would match nothing
    QString prio_only = "a^";

    size_t prio_count = prioritys ? prioritys->count : 0;
    for (unsigned int i = 0; i < prio_count; i++) {
        char *prio = prioritys->vector[i];
        QString *builderToUse = & prio_only;
        if (prio[0] == '\n') continue; // empty line
        if (prio[0] == '\r') continue; // empty line

        // If an exclude entry contains some fnmatch-ish characters, we use the C-style codepath without QRegularEpression
        if (strchr(prio, '/') || strchr(prio, '[') || strchr(prio, '{') || strchr(prio, '\\')) {
            _csync_priority_add(&list_patterns_fnmatch, prio);
            continue;
        }

        // not enabled
        if (prio[0] != ']'){
            continue;
        }
        if (builderToUse->size() > 0) {
            builderToUse->append("|");
        }
        builderToUse->append(convertToBnameRegexpSyntax(prio));
    }

    //QString pattern = "^(" + prio_only + ")$|^(" + prio_only + ")$";
    QString pattern = "^(" + prio_only + ")$";
    regexp_prioritys.setPattern(pattern);
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::OptimizeOnFirstUsageOption;
    if (OCC::Utility::fsCasePreserving())
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    regexp_prioritys.setPatternOptions(patternOptions);
    regexp_prioritys.optimize();
}
*/

/** Expands C-like escape sequences.
 *
 * The returned string is heap-allocated and owned by the caller.
 */
static const char *csync_priority_expand_escapes(const char * input)
{
    size_t i_len = strlen(input) + 1;
    char *out = (char*)c_malloc(i_len); // out can only be shorter

    size_t i = 0;
    size_t o = 0;
    for (; i < i_len; ++i) {
        if (input[i] == '\\') {
            // at worst input[i+1] is \0
            switch (input[i+1]) {
                case '\'': out[o++] = '\''; break;
                case '"': out[o++] = '"'; break;
                case '?': out[o++] = '?'; break;
                case '\\': out[o++] = '\\'; break;
                case '#': out[o++] = '#'; break;
                case 'a': out[o++] = '\a'; break;
                case 'b': out[o++] = '\b'; break;
                case 'f': out[o++] = '\f'; break;
                case 'n': out[o++] = '\n'; break;
                case 'r': out[o++] = '\r'; break;
                case 't': out[o++] = '\t'; break;
                case 'v': out[o++] = '\v'; break;
                default:
                    out[o++] = input[i];
                    out[o++] = input[i+1];
                    break;
            }
            ++i;
        } else {
            out[o++] = input[i];
        }
    }
    return out;
}


int csync_priority_load(const char *fname, c_strlist_t **list) {
    int fd = -1;
    int i = 0;
    int rc = -1;
    int64_t size;
    char *buf = nullptr;
    char *entry = nullptr;
    mbchar_t *w_fname;

    if (fname == nullptr) {
        return -1;
    }

#ifdef _WIN32
    _fmode = _O_BINARY;
#endif

    w_fname = c_utf8_path_to_locale(fname);
    if (w_fname == nullptr) {
        return -1;
    }

    fd = _topen(w_fname, O_RDONLY);
    c_free_locale_string(w_fname);
    if (fd < 0) {
        return -1;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        rc = -1;
        goto out;
    }
    lseek(fd, 0, SEEK_SET);
    if (size == 0) {
        rc = 0;
        goto out;
    }
    buf = (char*)c_malloc((size_t)size + 1);
    if (read(fd, buf, (size_t)size) != size) {
        rc = -1;
        goto out;
    }
    buf[size] = '\0';

    /* FIXME: Use fgets and don't add duplicates */
    entry = buf;
    for (i = 0; i < size; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            if (entry != buf + i) {
                buf[i] = '\0';
                if (*entry != '#') {
                    const char *unescaped = csync_priority_expand_escapes(entry);
                    rc = _csync_priority_add(list, unescaped);
                    if( rc == 0 ) {
                        CSYNC_LOG(CSYNC_LOG_PRIORITY_TRACE, "Adding entry: %s", unescaped);
                    }
                    SAFE_FREE(unescaped);
                    if (rc < 0) {
                        goto out;
                    }
                }
            }
            entry = buf + i + 1;
        }
    }

    rc = 0;
    out:
    SAFE_FREE(buf);
    close(fd);
    return rc;
}

static CSYNC_PRIORITY_TYPE _csync_priority_common(c_strlist_t *prioritizes,
                                                  const char *path,
                                                  int filetype,
                                                  bool check_leading_dirs) {
    size_t i = 0;
    const char *bname = nullptr;
    size_t blen = 0;
    int rc = -1;
    CSYNC_PRIORITY_TYPE match = CSYNC_NOT_PRIORITY;
    c_strlist_t *path_components = nullptr;

    /* split up the path */
    bname = strrchr(path, '/');
    if (bname) {
        bname += 1; // don't include the /
    } else {
        bname = path;
    }
    blen = strlen(bname);

    // check the strlen and ignore the file if its name is longer than 254 chars.
    // whenever changing this also check createDownloadTmpFileName
    if (blen > 254) {
        //match = CSYNC_FILE_EXCLUDE_LONG_FILENAME;
        goto out;
    }

    if( ! prioritizes ) {
        goto out;
    }

    if (check_leading_dirs) {
        /* Build a list of path components to check. */
        path_components = c_strlist_new(32);
        char *path_split = strdup(path);
        size_t len = strlen(path_split);
        for (i = len; ; --i) {
            // read backwards until a path separator is found
            if (i != 0 && path_split[i-1] != '/') {
                continue;
            }

            // check 'basename', i.e. for "/foo/bar/fi" we'd check 'fi', 'bar', 'foo'
            if (path_split[i] != 0) {
                c_strlist_add_grow(&path_components, path_split + i);
            }

            if (i == 0) {
                break;
            }

            // check 'dirname', i.e. for "/foo/bar/fi" we'd check '/foo/bar', '/foo'
            path_split[i-1] = '\0';
            c_strlist_add_grow(&path_components, path_split);
        }
        SAFE_FREE(path_split);
    }

    /* Loop over all priority patterns and evaluate the given path */
    for (i = 0; match == CSYNC_NOT_PRIORITY && i < prioritizes->count; i++) {
        bool match_dirs_only = false;
        char *pattern = prioritizes->vector[i];

        /* empty pattern */
        /* priority not starting with ']' means it not enabled */
        if (pattern[0] != ']') {
            continue;
        }

        pattern++;      // 跳过']'

        /* Check if the pattern applies to pathes only. */
        if (pattern[strlen(pattern)-1] == '/') {
            if (!check_leading_dirs && filetype == CSYNC_FTW_TYPE_FILE) {
                continue;
            }
            match_dirs_only = true;
            pattern[strlen(pattern)-1] = '\0'; /* Cut off the slash */
        }

        /* check if the pattern contains a / and if, compare to the whole path */
        if (strchr(pattern, '/')) {
            rc = csync_fnmatch(pattern, path, FNM_PATHNAME);
            if( rc == 0 ) {
                match = CSYNC_FILE_PRIORITY;
            }
            /* if the pattern requires a dir, but path is not, its still not excluded. */
            if (match_dirs_only && filetype != CSYNC_FTW_TYPE_DIR) {
                match = CSYNC_NOT_PRIORITY;
            }
        }

        /* if still not priority, check each component and leading directory of the path */
        if (match == CSYNC_NOT_PRIORITY && check_leading_dirs) {
            size_t j = 0;
            if (match_dirs_only && filetype == CSYNC_FTW_TYPE_FILE) {
                j = 1; // skip the first entry, which is bname
            }
            for (; j < path_components->count; ++j) {
                rc = csync_fnmatch(pattern, path_components->vector[j], 0);
                if (rc == 0) {
                    match = CSYNC_FILE_PRIORITY;
                    break;
                }
            }
        } else if (match == CSYNC_NOT_PRIORITY && !check_leading_dirs) {
            rc = csync_fnmatch(pattern, bname, 0);
            if (rc == 0) {
                match = CSYNC_FILE_PRIORITY;
            }
        }
        if (match_dirs_only) {
            /* restore the '/' */
            pattern[strlen(pattern)] = '/';
        }
    }
    c_strlist_destroy(path_components);

    out:
    return match;
}
/*
void csync_priority_traversal_prepare(CSYNC *ctx)
{
    //ctx->parsed_traversal_prioritys.prepare(ctx->prioritys);
}

CSYNC_PRIORITY_TYPE csync_priority_traversal(CSYNC *ctx, const char *path, int filetype) {
    //return _csync_priority_common(ctx->prioritys, path, filetype, false);
}
 */

/*
CSYNC_PRIORITY_TYPE csync_priority_traversal(CSYNC *ctx, const char *path, int filetype) {
    CSYNC_PRIORITY_TYPE match = CSYNC_NOT_PRIORITY;

    // Check only static patterns and only with the reduced list which is empty usually
    match = _csync_priority_common(ctx->parsed_traversal_prioritys.list_patterns_fnmatch, path, filetype, false);
    if (match != CSYNC_NOT_PRIORITY) {
        return match;
    }

    if (ctx->prioritys) {
        // Now check with our optimized regexps
        const char *bname = nullptr;
        // split up the path
        bname = strrchr(path, '/');
        if (bname) {
            bname += 1; // don't include the /
        } else {
            bname = path;
        }
        QString p = QString::fromUtf8(bname);
        auto m = ctx->parsed_traversal_prioritys.regexp_prioritys.match(p);
        if (m.hasMatch()) {
            if (!m.captured(1).isEmpty()) {
                match = CSYNC_FILE_PRIORITY;
            } else if (!m.captured(2).isEmpty()) {
                match = CSYNC_FILE_PRIORITY;
            }
        }
    }
    return match;
}
*/
CSYNC_PRIORITY_TYPE csync_priority_no_ctx(c_strlist_t *prioritys, const char *path, int filetype) {
    return _csync_priority_common(prioritys, path, filetype, true);
}

