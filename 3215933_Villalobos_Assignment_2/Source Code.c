// ============================================================================
// Program:  EECS 348 Assignment 2 - Email Priority Inbox
// Author:   Nicolas Villalobos
// ID:       3215933
// Created:  September 16, 2026
// Revised:  September 16, 2026
//
// Description:
//   Sorts a CEO's inbox using a max heap as a priority queue. The heap is
//   written from scratch on a list, which is a dynamically sized array.
//   Sender category sets the priority, and inside one category the newer
//   email is read first.
//
// Input:
//   A text file passed as the first command-line argument, or the same text
//   on standard input. Commands are EMAIL <category>,<subject>,<MM-DD-YYYY>,
//   NEXT, READ, and COUNT.
//
// Output:
//   Terminal output. NEXT prints the sender, subject, and date of the next
//   email. COUNT prints how many emails are unread. A line that does not
//   parse prints a warning on stderr.
//
// Collaborators and other sources:
//   Claude (Anthropic), Claude Opus 5, claude.ai, September 16, 2026.
//     Wrote the baseline program and helped with the analysis and the fixes.
//   Mistral (Mistral AI), Mistral Medium 3.5, chat.mistral.ai,
//     September 16, 2026.
//     Wrote the second program compared in the analysis. Its idea of
//     flagging an unknown sender category instead of accepting it was
//     used here.
//   No human collaborators.
//
// Revisions:
//   Rewrote the chosen raw version with terminated string copies, CRLF
//   handling, date validation, growable storage, and cached sort keys.
//   Added trim_field so spaces around fields do not throw out an email.
// ============================================================================

#include <stdio.h>                                // printf, fgets, fopen
#include <stdlib.h>                               // realloc, free
#include <string.h>                               // strlen, strcmp, strtok
#include <limits.h>                               // INT_MAX, overflow guard

#define SENDER_LEN        32                      // category text plus NUL
#define SUBJECT_LEN      256                      // subject text plus NUL
#define DATE_LEN          11                      // 10 date chars plus NUL
#define LINE_LEN         512                      // one input line
#define INITIAL_CAPACITY  16                      // slots on first insert

// Bigger number is read sooner, so the heap compares plain integers.
#define PRIORITY_BOSS             5               // read first
#define PRIORITY_SUBORDINATE      4               // read second
#define PRIORITY_PEER             3               // read third
#define PRIORITY_IMPORTANT_PERSON 2               // read fourth
#define PRIORITY_OTHER_PERSON     1               // read last

// Source: field layout from both raw versions. The two cached values at the
// bottom were added in the revision so a comparison does no string work.
typedef struct {
    char sender[SENDER_LEN];                      // category, printed by NEXT
    char subject[SUBJECT_LEN];                    // subject, printed by NEXT
    char date[DATE_LEN];                          // date, printed by NEXT
    int  priority;                                // rank from the table above
    long date_key;                                // date as YYYYMMDD, sorts
} Email;

// Source: heap-on-a-list idea from both raw versions. Both used a fixed
// 1000-slot array; this one grows. items[0] is the next email to read, and
// the children of index i sit at 2i+1 and 2i+2.
typedef struct {
    Email *items;                                 // the list itself
    int    size;                                  // emails stored
    int    capacity;                              // slots allocated
} MaxHeap;

// ============================== String helpers ==============================

// Source: Claude. Replaces strncpy, which left the date field without a
// NUL because a date fills that buffer exactly.
static void copy_field(char *dest, size_t dest_size, const char *src) {
    size_t i = 0;                                 // next write position
    while (i + 1 < dest_size && src[i] != '\0') { // leave room for the NUL
        dest[i] = src[i];                         // copy one character
        i++;                                      // move along
    }
    dest[i] = '\0';                               // always terminate
}

// Source: Claude. Both raw versions stripped only '\n', so a file saved on
// Windows kept a '\r' and no command ever matched.
static void trim_line(char *line) {
    size_t len = strlen(line);                    // current end of string
    while (len > 0) {                             // walk backwards
        char c = line[len - 1];                   // last character left
        if (c != '\n' && c != '\r' && c != ' ' && c != '\t') {
            break;                                // real text, stop
        }
        line[--len] = '\0';                       // drop the blank
    }
}

// Source: Claude. Trims blanks off both ends of one field and returns where
// the real text starts. Without this a space after a comma made the date 11
// characters long and the whole email was rejected.
static char *trim_field(char *text) {
    size_t len;                                   // length after trimming
    while (*text == ' ' || *text == '\t') {       // skip leading blanks
        text++;                                   // advance the pointer
    }
    len = strlen(text);                           // measure what is left
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[--len] = '\0';                       // cut trailing blanks
    }
    return text;                                  // start of the real text
}

// ============================== Field parsing ===============================

// Source: Claude. Reads exactly `count` digits and returns 0 if any
// character is not one, which is how the date is checked without sscanf.
static int read_digits(const char *text, int count, int *out) {
    int value = 0;                                // number built so far
    int i;                                        // loop counter
    for (i = 0; i < count; i++) {                 // one character at a time
        if (text[i] < '0' || text[i] > '9') {     // not a digit
            return 0;                             // reject
        }
        value = value * 10 + (text[i] - '0');     // shift up, add the digit
    }
    *out = value;                                 // hand back the number
    return 1;                                     // all characters were digits
}

// Source: five comparisons from both raw versions. Returning 0 for anything
// else follows Mistral, which flagged an unknown category instead of filing
// it under OtherPerson the way the Claude version did.
static int sender_priority(const char *sender, int *out) {
    if (strcmp(sender, "Boss") == 0) {            // highest category
        *out = PRIORITY_BOSS;                     // store the rank
        return 1;                                 // recognized
    }
    if (strcmp(sender, "Subordinate") == 0) {     // second category
        *out = PRIORITY_SUBORDINATE;              // store the rank
        return 1;                                 // recognized
    }
    if (strcmp(sender, "Peer") == 0) {            // third category
        *out = PRIORITY_PEER;                     // store the rank
        return 1;                                 // recognized
    }
    if (strcmp(sender, "ImportantPerson") == 0) { // fourth category
        *out = PRIORITY_IMPORTANT_PERSON;         // store the rank
        return 1;                                 // recognized
    }
    if (strcmp(sender, "OtherPerson") == 0) {     // lowest listed category
        *out = PRIORITY_OTHER_PERSON;             // store the rank
        return 1;                                 // recognized
    }
    return 0;                                     // caller rejects the line
}

// Source: Claude. Both raw versions called sscanf and ignored what it
// returned, so "13-45-2025" and "hello" both became dates. This locates the
// two dashes, reads each part, and range-checks it. One or two digits are
// accepted for the month and day so "1-3-2025" is not thrown out.
static int parse_date(const char *text, long *out) {
    int month, day, year;                         // the three parts
    size_t len = strlen(text);                    // total length
    const char *d1 = strchr(text, '-');           // first dash
    const char *d2;                               // second dash
    if (d1 == NULL) {                             // no separator at all
        return 0;                                 // reject
    }
    d2 = strchr(d1 + 1, '-');                     // look past the first
    if (d2 == NULL) {                             // only one separator
        return 0;                                 // reject
    }
    if (strchr(d2 + 1, '-') != NULL) {            // a third separator
        return 0;                                 // reject
    }
    if (d1 - text < 1 || d1 - text > 2) {         // month is 1 or 2 digits
        return 0;                                 // reject
    }
    if (d2 - d1 - 1 < 1 || d2 - d1 - 1 > 2) {     // day is 1 or 2 digits
        return 0;                                 // reject
    }
    if (text + len - d2 - 1 != 4) {               // year must be 4 digits
        return 0;                                 // reject
    }
    if (!read_digits(text, (int)(d1 - text), &month)) {
        return 0;                                 // month is not numeric
    }
    if (!read_digits(d1 + 1, (int)(d2 - d1 - 1), &day)) {
        return 0;                                 // day is not numeric
    }
    if (!read_digits(d2 + 1, 4, &year)) {         // four year digits
        return 0;                                 // year is not numeric
    }
    if (month < 1 || month > 12) {                // month out of range
        return 0;                                 // reject
    }
    if (day < 1 || day > 31) {                    // day out of range
        return 0;                                 // reject
    }
    if (year < 1) {                               // year zero is not a date
        return 0;                                 // reject
    }
    *out = year * 10000L + month * 100L + day;    // bigger number is newer
    return 1;                                     // valid date
}

// ============================= Heap operations ==============================

// Source: Claude. Nothing is allocated until the first email arrives.
static void heap_init(MaxHeap *h) {
    h->items = NULL;                              // no list yet
    h->size = 0;                                  // no emails
    h->capacity = 0;                              // no slots
}

// Source: Claude. Hands the memory back before the program exits.
static void heap_free(MaxHeap *h) {
    free(h->items);                               // release the list
    h->items = NULL;                              // no dangling pointer
    h->size = 0;                                  // nothing stored
    h->capacity = 0;                              // nothing reserved
}

// Source: Claude. Neither raw version had this; both reserved 1000 slots up
// front and silently dropped anything past them. Doubling instead of adding
// one slot at a time keeps the number of reallocs low.
static int heap_reserve(MaxHeap *h, int needed) {
    int new_capacity;                             // size being aimed for
    Email *grown;                                 // holds the realloc result
    if (needed <= h->capacity) {                  // already big enough
        return 1;                                 // nothing to do
    }
    new_capacity = h->capacity;                   // start from what we have
    if (new_capacity == 0) {                      // first allocation
        new_capacity = INITIAL_CAPACITY;          // start at 16 slots
    }
    while (new_capacity < needed) {               // grow until it fits
        if (new_capacity > INT_MAX / 2) {         // doubling would overflow
            return 0;                             // give up safely
        }
        new_capacity *= 2;                        // double the list
    }
    grown = realloc(h->items, (size_t)new_capacity * sizeof(Email));
    if (grown == NULL) {                          // allocation failed
        return 0;                                 // old list is still valid
    }
    h->items = grown;                             // switch to the bigger list
    h->capacity = new_capacity;                   // record the new size
    return 1;                                     // room is available
}

// Source: rule from the assignment, cached fields from Claude.
// True when a belongs above b in the heap.
static int higher_priority(const Email *a, const Email *b) {
    if (a->priority != b->priority) {             // different categories
        return a->priority > b->priority;         // category decides
    }
    return a->date_key > b->date_key;             // same category, newer wins
}

// Source: both raw versions. Exchanges two records in the list.
static void swap_emails(Email *a, Email *b) {
    Email temp = *a;                              // hold the first
    *a = *b;                                      // first takes the second
    *b = temp;                                    // second takes the held copy
}

// Source: both raw versions. A new email lands at the end of the list, so it
// climbs until its parent outranks it.
static void sift_up(MaxHeap *h, int index) {
    while (index > 0) {                           // index 0 has no parent
        int parent = (index - 1) / 2;             // parent of this index
        if (!higher_priority(&h->items[index], &h->items[parent])) {
            break;                                // already in place
        }
        swap_emails(&h->items[index], &h->items[parent]);
        index = parent;                           // keep climbing
    }
}

// Source: both raw versions. After READ moves the last record to the root,
// that record sinks until both of its children rank below it.
static void sift_down(MaxHeap *h, int index) {
    for (;;) {                                    // until nothing moves
        int left  = 2 * index + 1;                // left child index
        int right = 2 * index + 2;                // right child index
        int best  = index;                        // best of the three so far
        if (left < h->size &&
            higher_priority(&h->items[left], &h->items[best])) {
            best = left;                          // left child outranks
        }
        if (right < h->size &&
            higher_priority(&h->items[right], &h->items[best])) {
            best = right;                         // right child outranks
        }
        if (best == index) {                      // parent already on top
            break;                                // heap order restored
        }
        swap_emails(&h->items[index], &h->items[best]);
        index = best;                             // keep sinking
    }
}

// Source: both raw versions, with the capacity check from Claude.
static int heap_insert(MaxHeap *h, const Email *email) {
    if (!heap_reserve(h, h->size + 1)) {          // make sure there is room
        return 0;                                 // caller reports the drop
    }
    h->items[h->size] = *email;                   // append at the end
    h->size++;                                    // count it
    sift_up(h, h->size - 1);                      // move it into place
    return 1;                                     // stored
}

// Source: both raw versions. Looks at the next email without removing it.
static const Email *heap_peek(const MaxHeap *h) {
    if (h->size == 0) {                           // empty inbox
        return NULL;                              // nothing to show
    }
    return &h->items[0];                          // the root is the answer
}

// Source: both raw versions. Drops the highest-priority email, which is READ.
static void heap_remove_max(MaxHeap *h) {
    if (h->size == 0) {                           // empty inbox
        return;                                   // nothing to remove
    }
    h->items[0] = h->items[h->size - 1];          // last record to the root
    h->size--;                                    // forget the old last slot
    sift_down(h, 0);                              // restore heap order
}

// ============================ Command handling ==============================

// Source: comma splitting from both raw versions, validation from Claude.
// The date is checked while it is still whole, because checking
// after the copy would let a long bad date be cut down to something valid.
static int handle_email(MaxHeap *h, char *fields) {
    Email email;                                  // built here, then inserted
    char *sender  = strtok(fields, ",");          // first field
    char *subject = strtok(NULL, ",");            // second field
    char *date    = strtok(NULL, ",");            // third field
    if (sender == NULL || subject == NULL || date == NULL) {
        return 0;                                 // a field is missing
    }
    if (strtok(NULL, ",") != NULL) {              // a fourth field exists
        return 0;                                 // more commas than expected
    }
    sender  = trim_field(sender);                 // ignore blanks around it
    subject = trim_field(subject);                // ignore blanks around it
    date    = trim_field(date);                   // ignore blanks around it
    if (!parse_date(date, &email.date_key)) {     // check before copying
        return 0;                                 // not a real date
    }
    if (!sender_priority(sender, &email.priority)) {
        return 0;                                 // not one of the five
    }
    copy_field(email.sender, sizeof(email.sender), sender);
    copy_field(email.subject, sizeof(email.subject), subject);
    copy_field(email.date, sizeof(email.date), date);
    return heap_insert(h, &email);                // store the finished record
}

// Source: output format from the assignment. Staying silent on an empty inbox
// is a choice made with Claude, since the assignment never says what belongs
// there and an invented line is more likely to break a strict comparison.
static void handle_next(const MaxHeap *h) {
    const Email *top = heap_peek(h);              // next email, or NULL
    if (top == NULL) {                            // empty inbox
        return;                                   // print nothing
    }
    printf("Next email:\n");                      // header line
    printf("Sender: %s\n", top->sender);          // category line
    printf("Subject: %s\n", top->subject);        // subject line
    printf("Date: %s\n", top->date);              // date line
}

// Source: wording from the assignment's sample output. The plural form is kept
// even for one email, because that is the only wording the assignment shows.
static void handle_count(const MaxHeap *h) {
    printf("There are %d emails to read.\n", h->size);
}

// ================================ Entry point ===============================

// Source: dispatch loop from both raw versions. Taking the file from either a
// command-line argument or standard input was added with Claude, so it does
// not matter which way the grader runs it.
int main(int argc, char *argv[]) {
    MaxHeap inbox;                                // the priority queue
    FILE *input = stdin;                          // default input
    char line[LINE_LEN];                          // one command at a time
    int status = 0;                               // 1 if a line was rejected
    heap_init(&inbox);                            // empty inbox to start
    if (argc > 1) {                               // a filename was given
        input = fopen(argv[1], "r");              // try to open it
        if (input == NULL) {                      // it did not open
            fprintf(stderr, "Error: could not open %s\n", argv[1]);
            return 1;                             // cannot continue
        }
    }
    while (fgets(line, sizeof(line), input) != NULL) {
        trim_line(line);                          // strip the line ending
        if (line[0] == '\0') {                    // blank line
            continue;                             // skip it
        }
        if (strncmp(line, "EMAIL ", 6) == 0) {    // an EMAIL command
            if (!handle_email(&inbox, line + 6)) {// skip past "EMAIL "
                fprintf(stderr, "Warning: ignored malformed EMAIL line\n");
                status = 1;                       // remember the rejection
            }
        } else if (strcmp(line, "NEXT") == 0) {   // a NEXT command
            handle_next(&inbox);                  // show, do not remove
        } else if (strcmp(line, "READ") == 0) {   // a READ command
            heap_remove_max(&inbox);              // remove, do not show
        } else if (strcmp(line, "COUNT") == 0) {  // a COUNT command
            handle_count(&inbox);                 // print how many are left
        } else {                                  // not a known command
            fprintf(stderr, "Warning: unrecognized command\n");
            status = 1;                           // remember the rejection
        }
    }
    if (input != stdin) {                         // only close what we opened
        fclose(input);                            // release the file
    }
    heap_free(&inbox);                            // release the list
    return status;                                // 0 unless a line was bad
}
