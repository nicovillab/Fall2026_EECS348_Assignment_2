// EECS 348 Assignment 2
// REFERENCE ONLY, NOT THE SUBMITTED PROGRAM.
// Unmodified output from Claude (Anthropic, Claude Opus 5), claude.ai,
// September 16, 2026. Included as documentation for section 2d of the
// GenAI analysis. The submitted program is "Source Code.c".

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EMAILS 1000
#define SENDER_LEN 32
#define SUBJECT_LEN 256
#define DATE_LEN 11
#define LINE_LEN 512

typedef struct {
    char sender[SENDER_LEN];
    char subject[SUBJECT_LEN];
    char date[DATE_LEN];
    int priority;
    long dateKey;
} Email;

/* List-based max heap: the list is the array, index 0 is the root. */
typedef struct {
    Email items[MAX_EMAILS];
    int size;
} MaxHeap;

void heapInit(MaxHeap *h) {
    h->size = 0;
}

int senderPriority(const char *sender) {
    if (strcmp(sender, "Boss") == 0) return 5;
    if (strcmp(sender, "Subordinate") == 0) return 4;
    if (strcmp(sender, "Peer") == 0) return 3;
    if (strcmp(sender, "ImportantPerson") == 0) return 2;
    return 1; /* OtherPerson */
}

long makeDateKey(const char *date) {
    int month = 0, day = 0, year = 0;
    sscanf(date, "%d-%d-%d", &month, &day, &year);
    return year * 10000L + month * 100L + day;
}

int higherPriority(const Email *a, const Email *b) {
    if (a->priority != b->priority) {
        return a->priority > b->priority;
    }
    return a->dateKey > b->dateKey;
}

void swapEmails(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

void siftUp(MaxHeap *h, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (higherPriority(&h->items[index], &h->items[parent])) {
            swapEmails(&h->items[index], &h->items[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

void siftDown(MaxHeap *h, int index) {
    while (1) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        if (left < h->size && higherPriority(&h->items[left], &h->items[largest])) {
            largest = left;
        }
        if (right < h->size && higherPriority(&h->items[right], &h->items[largest])) {
            largest = right;
        }
        if (largest == index) {
            break;
        }
        swapEmails(&h->items[index], &h->items[largest]);
        index = largest;
    }
}

void heapInsert(MaxHeap *h, Email email) {
    if (h->size >= MAX_EMAILS) {
        return;
    }
    h->items[h->size] = email;
    h->size++;
    siftUp(h, h->size - 1);
}

Email *heapPeek(MaxHeap *h) {
    if (h->size == 0) {
        return NULL;
    }
    return &h->items[0];
}

void heapRemoveMax(MaxHeap *h) {
    if (h->size == 0) {
        return;
    }
    h->items[0] = h->items[h->size - 1];
    h->size--;
    siftDown(h, 0);
}

void addEmail(MaxHeap *h, char *fields) {
    Email email;
    char *sender = strtok(fields, ",");
    char *subject = strtok(NULL, ",");
    char *date = strtok(NULL, ",");

    if (sender == NULL || subject == NULL || date == NULL) {
        return;
    }

    strncpy(email.sender, sender, SENDER_LEN - 1);
    email.sender[SENDER_LEN - 1] = '\0';
    strncpy(email.subject, subject, SUBJECT_LEN - 1);
    email.subject[SUBJECT_LEN - 1] = '\0';
    strncpy(email.date, date, DATE_LEN - 1);
    email.date[DATE_LEN - 1] = '\0';

    email.priority = senderPriority(email.sender);
    email.dateKey = makeDateKey(email.date);

    heapInsert(h, email);
}

int main(int argc, char *argv[]) {
    static MaxHeap inbox;
    FILE *input = stdin;
    char line[LINE_LEN];

    heapInit(&inbox);

    if (argc > 1) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            fprintf(stderr, "Could not open %s\n", argv[1]);
            return 1;
        }
    }

    while (fgets(line, sizeof(line), input) != NULL) {
        size_t length = strlen(line);
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[length - 1] = '\0';
            length--;
        }
        if (length == 0) {
            continue;
        }

        if (strncmp(line, "EMAIL ", 6) == 0) {
            addEmail(&inbox, line + 6);
        } else if (strcmp(line, "NEXT") == 0) {
            Email *top = heapPeek(&inbox);
            if (top == NULL) {
                printf("No emails to read.\n");
            } else {
                printf("Next email:\n");
                printf("Sender: %s\n", top->sender);
                printf("Subject: %s\n", top->subject);
                printf("Date: %s\n", top->date);
            }
        } else if (strcmp(line, "READ") == 0) {
            heapRemoveMax(&inbox);
        } else if (strcmp(line, "COUNT") == 0) {
            printf("There are %d emails to read.\n", inbox.size);
        }
    }

    if (input != stdin) {
        fclose(input);
    }

    return 0;
}
