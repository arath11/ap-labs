#define _GNU_SOURCE

#include <sys/select.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/select.h>
#include <sys/inotify.h>
#include <fcntl.h>
#include <ftw.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "logger.h"

#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

/* logMessage() flags */

#define VB_BASIC 1 /* Basic messages */
#define VB_NOISY 2 /* Verbose messages */

static int verboseMask;
static int checkCache;
static int dumpCache;
static int readBufferSize = 0;
static char *stopFile;
static int abortOnCacheProblem;

static FILE *logfp = NULL;

static int inotifyReadCnt = 0; /* Counts number of read()s from
                                           inotify file descriptor */

static const int INOTIFY_READ_BUF_LEN =
    (100 * (sizeof(struct inotify_event) + NAME_MAX + 1));

static void dumpCacheToLog(void);

//stop file

__attribute__((__noreturn__)) static void
createStopFileAndAbort(void)
{
    open(stopFile, O_CREAT | O_RDWR, 0600);
    dumpCacheToLog();
    abort();
}

//log message
static void
logMessage(int vb_mask, const char *format, ...)
{
    va_list argList;
    if ((vb_mask == 0) || (vb_mask & verboseMask))
    {
        va_start(argList, format);
        vfprintf(stderr, format, argList);
        va_end(argList);
    }

    if (logfp != NULL)
    {
        va_start(argList, format);
        vfprintf(logfp, format, argList);
        va_end(argList);
    }
}

//display

static void
displayInotifyEvent(struct inotify_event *ev)
{
    if (ev->mask & IN_CREATE){
        if (ev->mask & IN_ISDIR)
        {
            infof("- [Directory Creation]: %s\n", ev->name);
        }
        else
        {
            infof("- [File Creation]: %s\n", ev->name);
        }
    }
    else if (ev->mask & IN_DELETE)
    {
        if (ev->mask & IN_ISDIR)
        {
            infof("- [Directory removal]: %s\n", ev->name);
        }
        else
        {
            infof("- [File removal]: %s\n", ev->name);
        }
    }
    else if (ev->mask & IN_MOVED_FROM)
    {
        if (ev->mask & IN_ISDIR)
        {
            infof("- [Directory rename]: %s", ev->name);
        }
        else
        {
            infof("- [File rename]: %s", ev->name);
        }
    }
    else if (ev->mask & IN_MOVED_TO)
    {
        if (!(ev->mask & IN_ISDIR))
        {
            printf(" to -> %s\n", ev->name);
        }
    }
}

//struct to watch 

struct watch
{
    int wd;              /* Watch descriptor (-1 if slot unused) */
    char path[PATH_MAX]; /* Cached pathname */
};

struct watch *wlCache = NULL; /* Array of cached items */

static int cacheSize = 0; /* Current size of the array */


static void
freeCache(void)
{
    free(wlCache);
    cacheSize = 0;
    wlCache = NULL;
}

static void
checkCacheConsistency(void)
{
    int failures;
    struct stat sb;

    failures = 0;
    for (int j = 0; j < cacheSize; j++)
    {
        if (wlCache[j].wd >= 0)
        {
            if (lstat(wlCache[j].path, &sb) == -1)
            {
                logMessage(0,
                           "checkCacheConsistency: stat: "
                           "[slot = %d; wd = %d] %s: %s\n",
                           j, wlCache[j].wd, wlCache[j].path, strerror(errno));
                failures++;
            }
            else if (!S_ISDIR(sb.st_mode))
            {
                logMessage(0, "checkCacheConsistency: %s is not a directory\n",
                           wlCache[j].path);
                exit(EXIT_FAILURE);
            }
        }
    }

    if (failures > 0)
        logMessage(VB_NOISY, "checkCacheConsistency: %d failures\n",
                   failures);
}

static int
findWatch(int wd)
{
    for (int j = 0; j < cacheSize; j++)
        if (wlCache[j].wd == wd)
            return j;

    return -1;
}

static int
findWatchChecked(int wd)
{
    int slot;

    slot = findWatch(wd);

    if (slot >= 0)
        return slot;

    logMessage(0, "Could not find watch %d\n", wd);


    if (abortOnCacheProblem)
    {
        createStopFileAndAbort();
    }
    else
    {
        return -1;
    }
}

/* Mark a cache entry as unused */

static void
markCacheSlotEmpty(int slot)
{
    // logMessage(VB_NOISY,
    //            "        markCacheSlotEmpty: slot = %d;  wd = %d; path = %s\n",
    //            slot, wlCache[slot].wd, wlCache[slot].path);

    wlCache[slot].wd = -1;
    wlCache[slot].path[0] = '\0';
}

/* Find a free slot in the cache */

static int
findEmptyCacheSlot(void)
{
    const int ALLOC_INCR = 200;

    for (int j = 0; j < cacheSize; j++)
        if (wlCache[j].wd == -1)
            return j;

    /* No free slot found; resize cache */

    cacheSize += ALLOC_INCR;

    wlCache = realloc(wlCache, cacheSize * sizeof(struct watch));
    if (wlCache == NULL)
        errExit("realloc");

    for (int j = cacheSize - ALLOC_INCR; j < cacheSize; j++)
        markCacheSlotEmpty(j);

    return cacheSize - ALLOC_INCR; /* Return first slot in
                                           newly allocated space */
}

/* Add an item to the cache */

static int
addWatchToCache(int wd, const char *pathname)
{
    int slot;

    slot = findEmptyCacheSlot();

    wlCache[slot].wd = wd;
    strncpy(wlCache[slot].path, pathname, PATH_MAX);

    return slot;
}

/* Return the cache slot that corresponds to a particular pathname,
   or -1 if the pathname is not in the cache */

static int
pathnameToCacheSlot(const char *pathname)
{
    for (int j = 0; j < cacheSize; j++)
        if (wlCache[j].wd >= 0 && strcmp(wlCache[j].path, pathname) == 0)
            return j;

    return -1;
}

/* Is 'pathname' in the watch cache? */

static int
pathnameInCache(const char *pathname)
{
    return pathnameToCacheSlot(pathname) >= 0;
}

/* Dump contents of watch cache to the log file */

static void
dumpCacheToLog(void)
{
    int cnt;

    cnt = 0;

    for (int j = 0; j < cacheSize; j++)
    {
        if (wlCache[j].wd >= 0)
        {
            fprintf(logfp, "%d: wd = %d; %s\n", j,
                    wlCache[j].wd, wlCache[j].path);
            cnt++;
        }
    }

    fprintf(logfp, "Total entries: %d\n", cnt);
}

/***********************************************************************/

/* Data structures and functions for dealing with the directory pathnames
   provided as command-line arguments. These directories form the roots of
   the trees that we will monitor */

static char **rootDirPaths; /* List of pathnames supplied on command line */
static int numRootDirs;     /* Number of pathnames supplied on command line */
static int ignoreRootDirs;  /* Number of command-line pathnames that
                               we've ceased to monitor */
static struct stat *rootDirStat;
/* stat(2) structures for root directories */

/* Duplicate the pathnames supplied on the command line, perform
   some sanity checking along the way */

static void
copyRootDirPaths(char *argv[])
{
    struct stat sb;

    numRootDirs = 0;

    /* Count the number of root paths, and check that the paths are valid */

    for (char **p = argv; *p != NULL; p++)
    {

        /* Check that command-line arguments are directories */

        if (lstat(*p, &sb) == -1)
        {
            fprintf(stderr, "lstat() failed on '%s'\n", *p);
            exit(EXIT_FAILURE);
        }

        if (!S_ISDIR(sb.st_mode))
        {
            fprintf(stderr, "'%s' is not a directory\n", *p);
            exit(EXIT_FAILURE);
        }

        numRootDirs++;
    }

    /* Create a copy of the root directory pathnames */

    rootDirPaths = calloc(numRootDirs, sizeof(char *));
    if (rootDirPaths == NULL)
        errExit("calloc");

    rootDirStat = calloc(numRootDirs, sizeof(struct stat));
    if (rootDirPaths == NULL)
        errExit("calloc");

    for (int j = 0; j < numRootDirs; j++)
    {
        rootDirPaths[j] = strdup(argv[j]);
        if (rootDirPaths[j] == NULL)
            errExit("strdup");

        /* If the same filesystem object appears more than once in the
           command line, this will cause confusion if we later try to zap an
           object from the set of root paths. So, reject such duplicates now.
           Note that we can't just do simple string comparisons of the
           arguments, since different pathname strings may refer to the same
           filesystem object (e.g., "mydir" and "./mydir"). So, we use stat()
           to compare i-node numbers and containing device IDs. */

        if (lstat(argv[j], &rootDirStat[j]) == -1)
            errExit("lstat");

        for (int k = 0; k < j; k++)
        {
            if ((rootDirStat[j].st_ino == rootDirStat[k].st_ino) &&
                (rootDirStat[j].st_dev == rootDirStat[k].st_dev))
            {

                fprintf(stderr, "Duplicate filesystem objects: %s, %s\n",
                        argv[j], argv[k]);
                exit(EXIT_FAILURE);
            }
        }
    }

    ignoreRootDirs = 0;
}

/* Return the address of the element in 'rootDirPaths' that points
   to a string matching 'path', or NULL if there is no match */

static char **
findRootDirPath(const char *path)
{
    for (int j = 0; j < numRootDirs; j++)
        if (rootDirPaths[j] != NULL && strcmp(path, rootDirPaths[j]) == 0)
            return &rootDirPaths[j];

    return NULL;
}

/* Is 'path' one of the pathnames that was listed on the command line? */

static int
isRootDirPath(const char *path)
{
    return findRootDirPath(path) != NULL;
}

/* We've ceased to monitor a root directory pathname (probably because it
   was renamed), so zap this pathname from the root path list */

static void
zapRootDirPath(const char *path)
{
    char **p;

    printf("zapRootDirPath: %s\n", path);

    p = findRootDirPath(path);
    if (p == NULL)
    {
        fprintf(stderr, "zapRootDirPath(): path not found!\n");
        exit(EXIT_FAILURE);
    }

    *p = NULL;
    ignoreRootDirs++;
    if (ignoreRootDirs == numRootDirs)
    {
        fprintf(stderr, "No more root paths left to monitor; bye!\n");
        exit(EXIT_SUCCESS);
    }
}

/***********************************************************************/

/* Below is a function called by nftw() to traverse a directory tree.
   The function adds a watch for each directory in the tree. Each
   successful call to this function should return 0 to indicate to
   nftw() that the tree traversal should continue. */

/* The usual hack for nftw()...  We can't pass arguments to the
   function invoked by nftw(), so we use these global variables to
   exchange information with the function. */

static int dirCnt; /* Count of directories added to watch list */
static int ifd;    /* Inotify file descriptor */

static int
traverseTree(const char *pathname, const struct stat *sb, int tflag,
             struct FTW *ftwbuf)
{
    int wd, slot, flags;

    if (!S_ISDIR(sb->st_mode))
        return 0; /* Ignore nondirectory files */

    /* Create a watch for this directory */

    flags = IN_CREATE | IN_MOVED_FROM | IN_MOVED_TO | IN_DELETE;

    if (isRootDirPath(pathname))
        flags |= IN_MOVE_SELF;

    wd = inotify_add_watch(ifd, pathname, flags | IN_ONLYDIR);
    if (wd == -1)
    {

        /* By the time we come to create a watch, the directory might already
           have been deleted or renamed, in which case we'll get an ENOENT
           error. In that case, we log the error, but carry on execution.
           Other errors are unexpected, and if we hit them, we give up. */

        logMessage(VB_BASIC, "inotify_add_watch: %s: %s\n",
                   pathname, strerror(errno));
        if (errno == ENOENT)
            return 0;
        else
            exit(EXIT_FAILURE);
    }

    if (findWatch(wd) >= 0)
    {

        /* This watch descriptor is already in the cache;
           nothing more to do. */

        logMessage(VB_BASIC, "WD %d already in cache (%s)\n", wd, pathname);
        return 0;
    }

    dirCnt++;

    /* Cache information about the watch */

    slot = addWatchToCache(wd, pathname);

    /* Print the name of the current directory */

    // logMessage(VB_NOISY, "    watchDir: wd = %d [cache slot: %d]; %s\n",
    //            wd, slot, pathname);

    return 0;
}

/* Add the directory in 'pathname' to the watch list of the inotify
   file descriptor 'inotifyFd'. The process is recursive: watch items
   are also created for all of the subdirectories of 'pathname'.
   Returns number of watches/cache entries added for this subtree. */

static int
watchDir(int inotifyFd, const char *pathname)
{
    dirCnt = 0;
    ifd = inotifyFd;

    /* Use FTW_PHYS to avoid following soft links to directories (which
       could lead us in circles) */

    /* By the time we come to process 'pathname', it may already have
       been deleted, so we log errors from nftw(), but keep on going */

    if (nftw(pathname, traverseTree, 20, FTW_PHYS) == -1)
        logMessage(VB_BASIC,
                   "nftw: %s: %s (directory probably deleted before we "
                   "could watch)\n",
                   pathname, strerror(errno));

    return dirCnt;
}

/* Add watches and cache entries for a subtree, logging a message
   noting the number entries added. */

static void
watchSubtree(int inotifyFd, char *path)
{
    int cnt;

    cnt = watchDir(inotifyFd, path);

    logMessage(VB_BASIC, "    watchSubtree: %s: %d entries added\n",
               path, cnt);
}

/***********************************************************************/

/* The directory oldPathPrefix/oldName was renamed to newPathPrefix/newName.
   Fix up cache entries for oldPathPrefix/oldName and all of its subdirectories
   to reflect the change. */

static void
rewriteCachedPaths(const char *oldPathPrefix, const char *oldName,
                   const char *newPathPrefix, const char *newName)
{
    char fullPath[PATH_MAX], newPrefix[PATH_MAX];
    char newPath[PATH_MAX];
    size_t len;
    int s;

    snprintf(fullPath, sizeof(fullPath), "%s/%s", oldPathPrefix, oldName);
    snprintf(newPrefix, sizeof(newPrefix), "%s/%s", newPathPrefix, newName);
    len = strlen(fullPath);

    logMessage(VB_BASIC, "Rename: %s ==> %s\n", fullPath, newPrefix);

    for (int j = 0; j < cacheSize; j++)
    {
        if (strncmp(fullPath, wlCache[j].path, len) == 0 &&
            (wlCache[j].path[len] == '/' ||
             wlCache[j].path[len] == '\0'))
        {
            s = snprintf(newPath, sizeof(newPath), "%s%s", newPrefix,
                         &wlCache[j].path[len]);
            if (s > sizeof(newPath))
                logMessage(VB_BASIC, "Truncated pathname: %s\n", newPath);

            strncpy(wlCache[j].path, newPath, PATH_MAX);

            printf(" -> %s\n", newName);
        }
    }
}

/* Zap watches and cache entries for directory 'path' and all of its
   subdirectories. Returns number of entries that we (tried to) zap,
   or -1 if an inotify_rm_watch() call failed. */

static int
zapSubtree(int inotifyFd, char *path)
{
    size_t len;
    int cnt;
    char *pn;

    logMessage(VB_NOISY, "Zapping subtree: %s\n", path);

    len = strlen(path);

    /* The argument we receive might be a pointer to a pathname string
       that is actually stored in the cache.  If we zap that pathname part way
       through scanning the whole cache, then chaos results.  So, create a
       temporary copy. */

    pn = strdup(path);

    cnt = 0;

    for (int j = 0; j < cacheSize; j++)
    {
        if (wlCache[j].wd >= 0)
        {
            if (strncmp(pn, wlCache[j].path, len) == 0 &&
                (wlCache[j].path[len] == '/' ||
                 wlCache[j].path[len] == '\0'))
            {

                logMessage(VB_NOISY,
                           "    removing watch: wd = %d (%s)\n",
                           wlCache[j].wd, wlCache[j].path);

                if (inotify_rm_watch(inotifyFd, wlCache[j].wd) == -1)
                {
                    logMessage(0, "inotify_rm_watch wd = %d (%s): %s\n",
                               wlCache[j].wd, wlCache[j].path, strerror(errno));

                    /* When we have multiple renamers, sometimes
                       inotify_rm_watch() fails. In this case, we force a
                       cache rebuild by returning -1.
                       (TODO: Is there a better solution?) */

                    cnt = -1;
                    break;
                }

                markCacheSlotEmpty(j);
                cnt++;
            }
        }
    }

    free(pn);
    return cnt;
}


static int
reinitialize(int oldInotifyFd)
{
    int inotifyFd;
    static int reinitCnt;
    int cnt;

    if (oldInotifyFd >= 0)
    {
        close(oldInotifyFd);

        reinitCnt++;
        logMessage(0, "Reinitializing cache and inotify FD (reinitCnt = %d)\n",
                   reinitCnt);
    }
    else
    {
        // logMessage(0, "Initializing cache\n");
        reinitCnt = 0;
    }

    inotifyFd = inotify_init();
    if (inotifyFd == -1)
        errExit("inotify_init");

    logMessage(VB_BASIC, "    new inotifyFd = %d\n", inotifyFd);

    freeCache();

    for (int j = 0; j < numRootDirs; j++)
        if (rootDirPaths[j] != NULL)
            watchSubtree(inotifyFd, rootDirPaths[j]);

    cnt = 0;
    for (int j = 0; j < cacheSize; j++)
        if (wlCache[j].wd >= 0)
            cnt++;

    if (oldInotifyFd >= 0)
        logMessage(0, "Rebuilt cache with %d entries\n", cnt);

    return inotifyFd;
}

/* Process the next inotify event in the buffer specified by 'buf'
   and 'bufSize'. In most cases, a single event is consumed, but
   if there is an IN_MOVED_FROM+IN_MOVED_TO pair that share a cookie
   value, both events are consumed.
   Returns the number of bytes in the event(s) consumed from 'buf'.  */

static size_t
processNextInotifyEvent(int *inotifyFd, char *buf, int bufSize, int firstTry)
{
    char fullPath[PATH_MAX + NAME_MAX];
    struct inotify_event *ev;
    size_t evLen;
    int evCacheSlot;

    ev = (struct inotify_event *)buf;

    displayInotifyEvent(ev);

    if (ev->wd != -1 && !(ev->mask & IN_IGNORED))
    {
        evCacheSlot = findWatchChecked(ev->wd);
        if (evCacheSlot == -1)
        {

            *inotifyFd = reinitialize(*inotifyFd);


            return INOTIFY_READ_BUF_LEN;
        }
    }

    evLen = sizeof(struct inotify_event) + ev->len;

    if ((ev->mask & IN_ISDIR) &&
        (ev->mask & (IN_CREATE | IN_MOVED_TO)))
    {

        /* A new subdirectory was created, or a subdirectory was
           renamed into the tree; create watches for it, and all
           of its subdirectories. */

        snprintf(fullPath, sizeof(fullPath), "%s/%s",
                 wlCache[evCacheSlot].path, ev->name);

        logMessage(VB_BASIC, "Directory creation on wd %d: %s\n",
                   ev->wd, fullPath);

  
        if (!pathnameInCache(fullPath))
            watchSubtree(*inotifyFd, fullPath);
    }
    else if (ev->mask & IN_DELETE_SELF)
    {

        /* A directory was deleted. Remove the corresponding item from
           the cache. */

        logMessage(VB_BASIC, "Clearing watchlist item %d (%s)\n",
                   ev->wd, wlCache[evCacheSlot].path);

        if (isRootDirPath(wlCache[evCacheSlot].path))
            zapRootDirPath(wlCache[evCacheSlot].path);

        markCacheSlotEmpty(evCacheSlot);
        /* No need to remove the watch; that happens automatically */
    }
    else if ((ev->mask & (IN_MOVED_FROM | IN_ISDIR)) ==
             (IN_MOVED_FROM | IN_ISDIR))
    {

        struct inotify_event *nextEv;

        nextEv = (struct inotify_event *)(buf + evLen);

        if (((char *)nextEv < buf + bufSize) &&
            (nextEv->mask & IN_MOVED_TO) &&
            (nextEv->cookie == ev->cookie))
        {

            int nextEvCacheSlot;

            /* We have a rename() event. We need to fix up the cached pathnames
             * for the corresponding directory and all of its subdirectories. */

            nextEvCacheSlot = findWatchChecked(nextEv->wd);

            if (nextEvCacheSlot == -1)
            {

                /* Cache reached an inconsistent state */

                *inotifyFd = reinitialize(*inotifyFd);

                /* Discard all remaining events in current read() buffer */

                return INOTIFY_READ_BUF_LEN;
            }

            rewriteCachedPaths(wlCache[evCacheSlot].path, ev->name,
                               wlCache[nextEvCacheSlot].path, nextEv->name);

            /* We have also processed the next (IN_MOVED_TO) event,
               so skip over it */

            evLen += sizeof(struct inotify_event) + nextEv->len;
        }
        else if (((char *)nextEv < buf + bufSize) || !firstTry)
        {

            /* We got a "moved from" event without an accompanying "moved to"
               event. The directory has been moved outside the tree we are
               monitoring. We need to remove the watches and zap the cache
               entries for the moved directory and all of its subdirectories. */

            logMessage(VB_NOISY, "MOVED_OUT: %p %p\n",
                       wlCache[evCacheSlot].path, ev->name);
            logMessage(VB_NOISY, "firstTry = %d; remaining bytes = %d\n",
                       firstTry, buf + bufSize - (char *)nextEv);
            snprintf(fullPath, sizeof(fullPath), "%s/%s",
                     wlCache[evCacheSlot].path, ev->name);

            if (zapSubtree(*inotifyFd, fullPath) == -1)
            {

                /* Cache reached an inconsistent state */

                *inotifyFd = reinitialize(*inotifyFd);

                /* Discard all remaining events in current read() buffer */

                return INOTIFY_READ_BUF_LEN;
            }
        }
        else
        {
            logMessage(VB_NOISY, "HANGING IN_MOVED_FROM\n");

            return -1; /* Tell our caller to do another read() */
        }
    }
    else if (ev->mask & IN_Q_OVERFLOW)
    {

        static int overflowCnt = 0;

        overflowCnt++;

        logMessage(0, "Queue overflow (%d) (inotifyReadCnt = %d)\n",
                   overflowCnt, inotifyReadCnt);

        /* When the queue overflows, some events are lost, at which point
           we've lost any chance of keeping our cache consistent with the
           state of the filesystem. So, discard this inotify file descriptor
           and create a new one, and zap and rebuild the cache. */

        *inotifyFd = reinitialize(*inotifyFd);

        /* Discard all remaining events in current read() buffer */

        evLen = INOTIFY_READ_BUF_LEN;
    }
    else if (ev->mask & IN_UNMOUNT)
    {

    

        logMessage(0, "Filesystem unmounted: %s\n",
                   wlCache[evCacheSlot].path);

        markCacheSlotEmpty(evCacheSlot);
        /* No need to remove the watch; that happens automatically */
    }
    else if (ev->mask & IN_MOVE_SELF &&
             isRootDirPath(wlCache[evCacheSlot].path))
    {

     
        logMessage(0, "Root path moved: %s\n",
                   wlCache[evCacheSlot].path);

        zapRootDirPath(wlCache[evCacheSlot].path);

        if (zapSubtree(*inotifyFd, wlCache[evCacheSlot].path) == -1)
        {

            /* Cache reached an inconsistent state */

            *inotifyFd = reinitialize(*inotifyFd);

            /* Discard all remaining events in current read() buffer */

            return INOTIFY_READ_BUF_LEN;
        }
    }

    if (checkCache)
        checkCacheConsistency();

    if (dumpCache)
        dumpCacheToLog();

    return evLen;
}

static void
alarmHandler(int sig)
{
    return; /* Just interrupt read() */
}


static void
processInotifyEvents(int *inotifyFd)
{
    char buf[INOTIFY_READ_BUF_LEN]
        __attribute__((aligned(__alignof__(struct inotify_event))));
    ssize_t numRead, nr;
    size_t cnt;
    int evLen;
    int firstTry;
    struct sigaction sa;

    /* SIGALRM handler is designed simply to interrupt read() */

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = alarmHandler;
    sa.sa_flags = 0;
    if (sigaction(SIGALRM, &sa, NULL) == -1)
        errExit("sigaction");

    firstTry = 1;

    /* Read some events from inotify file descriptor */

    cnt = (readBufferSize > 0) ? readBufferSize : INOTIFY_READ_BUF_LEN;
    numRead = read(*inotifyFd, buf, cnt);
    if (numRead == -1)
        errExit("read");
    if (numRead == 0)
    {
        fprintf(stderr, "read() from inotify fd returned 0!");
        exit(EXIT_FAILURE);
    }

    inotifyReadCnt++;


    for (char *evp = buf; evp < buf + numRead;){
        evLen = processNextInotifyEvent(inotifyFd, evp,
                                        buf + numRead - evp, firstTry);

        if (evLen > 0)
        {
            evp += evLen;
            firstTry = 1;
        }
        else
        {
            int savedErrno;

            firstTry = 0;

            numRead = buf + numRead - evp;

            /* Shuffle remaining bytes to start of buffer */

            for (int j = 0; j < numRead; j++)
                buf[j] = evp[j];

            ualarm(2000, 0);

            nr = read(*inotifyFd, buf + numRead,
                      INOTIFY_READ_BUF_LEN - numRead);

            savedErrno = errno; /* In case ualarm() should change errno */
            ualarm(0, 0);       /* Cancel alarm */
            errno = savedErrno;

            if (nr == -1 && errno != EINTR)
                errExit("read");
            if (nr == 0)
            {
                fprintf(stderr, "read() from inotify fd returned 0!");
                exit(EXIT_FAILURE);
            }

            if (errno != -1)
            {
                numRead += nr;
                inotifyReadCnt++;

                logMessage(VB_NOISY,
                           "\n==========> SECONDARY Read %d: got %zd bytes\n",
                           inotifyReadCnt, nr);
            }
            else
            { /* EINTR */
                logMessage(VB_NOISY,
                           "\n==========> SECONDARY Read got nothing\n");
            }

            evp = buf; /* Start again at beginning of buffer */
        }
    }
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        errorf("Error en directorio\n");
        return 0;
    }else{
    fd_set rfds;
    int opt;
    int inotifyFd;

    /* Parse command-line options */
    //donde se encuentra
    verboseMask = 2;

    checkCache = 0;
    dumpCache = 0;
    stopFile = NULL;
    abortOnCacheProblem = 0;

    /* Save a copy of the directories on the command line */

    copyRootDirPaths(&argv[optind]);

    /* Create an inotify instance and populate it with entries for
       directory named on command line */

    inotifyFd = reinitialize(-1);

    /* Loop to handle inotify events */

    infof("Starting File/Directory Monitor on %s", argv[1]);
    infof("\n-----------------------------------------------------\n");
    for (;;){
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(inotifyFd, &rfds);
        if (select(inotifyFd + 1, &rfds, NULL, NULL, NULL) == -1){
            errExit("select");
        }
        if (FD_ISSET(inotifyFd, &rfds))
            processInotifyEvents(&inotifyFd);
    }
    return 0;
    }
}