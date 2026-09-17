// EECS 348 Assignment 2
// REFERENCE ONLY, NOT THE SUBMITTED PROGRAM.
// Unmodified output from Mistral (Mistral AI, Mistral Medium 3.5),
// chat.mistral.ai, September 16, 2026. Included as documentation for section
// 2d of the GenAI analysis. The submitted program is "Source Code.c".

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_EMAILS 1000
#define MAX_LINE 1024

typedef struct {
    char sender[50];
    char subject[256];
    char date[11]; // MM-DD-YYYY
} Email;

typedef struct {
    Email emails[MAX_EMAILS];
    int size;
} MaxHeap;

// Helper function to get priority of sender
int get_priority(char *sender) {
    if (strcmp(sender, "Boss") == 0) return 4;
    if (strcmp(sender, "Subordinate") == 0) return 3;
    if (strcmp(sender, "Peer") == 0) return 2;
    if (strcmp(sender, "ImportantPerson") == 0) return 1;
    if (strcmp(sender, "OtherPerson") == 0) return 0;
    return -1; // Unknown sender
}

// Helper function to parse date and return as integer (YYYYMMDD)
int parse_date(char *date) {
    int month, day, year;
    sscanf(date, "%d-%d-%d", &month, &day, &year);
    return year * 10000 + month * 100 + day;
}

// Compare two emails: returns true if 'a' has higher priority than 'b'
bool is_higher_priority(Email a, Email b) {
    int priority_a = get_priority(a.sender);
    int priority_b = get_priority(b.sender);
    if (priority_a != priority_b) {
        return priority_a > priority_b;
    }
    // Same priority, compare dates (newer is higher)
    int date_a = parse_date(a.date);
    int date_b = parse_date(b.date);
    return date_a > date_b;
}

// Move the element at 'index' up the heap
void heapify_up(MaxHeap *heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (is_higher_priority(heap->emails[index], heap->emails[parent])) {
            // Swap
            Email temp = heap->emails[index];
            heap->emails[index] = heap->emails[parent];
            heap->emails[parent] = temp;
            index = parent;
        } else {
            break;
        }
    }
}

// Move the element at 'index' down the heap
void heapify_down(MaxHeap *heap, int index) {
    int left, right, largest;
    while (1) {
        left = 2 * index + 1;
        right = 2 * index + 2;
        largest = index;

        if (left < heap->size && is_higher_priority(heap->emails[left], heap->emails[largest])) {
            largest = left;
        }
        if (right < heap->size && is_higher_priority(heap->emails[right], heap->emails[largest])) {
            largest = right;
        }

        if (largest != index) {
            Email temp = heap->emails[index];
            heap->emails[index] = heap->emails[largest];
            heap->emails[largest] = temp;
            index = largest;
        } else {
            break;
        }
    }
}

// Insert an email into the heap
void insert_email(MaxHeap *heap, Email email) {
    if (heap->size >= MAX_EMAILS) {
        return; // Heap full
    }
    heap->emails[heap->size] = email;
    heap->size++;
    heapify_up(heap, heap->size - 1);
}

// Remove and return the highest priority email
Email extract_max(MaxHeap *heap) {
    Email empty = {"", "", ""};
    if (heap->size == 0) {
        return empty;
    }
    Email max = heap->emails[0];
    heap->emails[0] = heap->emails[heap->size - 1];
    heap->size--;
    heapify_down(heap, 0);
    return max;
}

// Return the highest priority email without removing it
Email peek_max(MaxHeap *heap) {
    Email empty = {"", "", ""};
    if (heap->size == 0) {
        return empty;
    }
    return heap->emails[0];
}

int main() {
    MaxHeap heap = { .size = 0 };
    char line[MAX_LINE];

    while (fgets(line, sizeof(line), stdin)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "EMAIL ", 6) == 0) {
            // Parse the EMAIL command
            char *token = line + 6; // Skip "EMAIL "
            char *sender = strtok(token, ",");
            char *subject = strtok(NULL, ",");
            char *date = strtok(NULL, ",");

            if (sender && subject && date) {
                Email email;
                strncpy(email.sender, sender, sizeof(email.sender) - 1);
                strncpy(email.subject, subject, sizeof(email.subject) - 1);
                strncpy(email.date, date, sizeof(email.date) - 1);
                insert_email(&heap, email);
            }
        } else if (strcmp(line, "NEXT") == 0) {
            // Display the next email
            Email next = peek_max(&heap);
            if (heap.size > 0) {
                printf("Next email:\n");
                printf("Sender: %s\n", next.sender);
                printf("Subject: %s\n", next.subject);
                printf("Date: %s\n", next.date);
            } else {
                printf("No emails to display.\n");
            }
        } else if (strcmp(line, "READ") == 0) {
            // Remove the highest priority email
            if (heap.size > 0) {
                extract_max(&heap);
            }
        } else if (strcmp(line, "COUNT") == 0) {
            // Display the current count of unread emails
            printf("There are %d emails to read.\n", heap.size);
        }
    }

    return 0;
}
