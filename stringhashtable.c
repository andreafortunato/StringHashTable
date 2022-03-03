#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


/* Structure that holds information of an hash table entry. */
typedef struct hashtable_entry_t {
    char* key;                      /* Entry key */
    unsigned int val;               /* Entry value */

    struct hashtable_entry_t* next; /* Pointer to next entry */

} hashtable_entry;

/* Structure that holds information of an hash table. */
typedef struct hashtable_t {
    unsigned int size;              /* Hash table size */
    unsigned int different_entries; /* Number of different entries */
    unsigned int collisions;        /* Number of collisions */

    struct hashtable_entry_t** table; /* Hash table array */
} hashtable;


/* Releases the memory occupied by a pointer and, for security
   reasons, clears its entire contents. */
void erease(void* pointer, unsigned int size) {
    if(pointer == NULL || size == 0)
        return;
    
    memset(pointer, '\0', size);
    free(pointer);
}

/* Calculate hash value (integer) of a string (key) and return it.
   The algorithm used consists in adding all the integer values
   corresponding to each character of the string, adding the
   previous result multiplied by 33 each time.
   To optimize the speed, the multiplication by 33 is done by making
   a 5 bit shift to the left, which corresponds to a multiplication
   by 2^5 = 32, and then the last value is added.
   At each step, to prevent the value from overflowing and above all
   to prevent it from exceeding the maximum size of the hash table,
   the modulo operation is used. */
unsigned int hashtable_gethash(unsigned int hashtable_size, char* key) {
    unsigned int hash = 0;

    for (char* ch = key; *ch != '\0'; ch++) {
        hash = ((int)(*ch) + (hash << 5) + hash) % hashtable_size;
    }

    return hash;
}

/* Create a new hash table with a specific size and return it. */
hashtable* hashtable_newhashtable(unsigned int size) {
    if(size < 2)
        return NULL;
    
    hashtable* htable = NULL;

    if((htable = (hashtable*)malloc(sizeof(hashtable))) == NULL) {
        printf("[ERROR] There was an error while trying to call 'malloc' on 'htable'. Closing...\n");
		exit(EXIT_FAILURE);
    }

    /* Initialize hash table */    
    htable->size = size;
    htable->different_entries = 0;
    htable->collisions = 0;

    if((htable->table = (hashtable_entry**)malloc(sizeof(hashtable_entry*)*size)) == NULL) {
        printf("[ERROR] There was an error while trying to call 'malloc' on 'htable->table'. Closing...\n");
		exit(EXIT_FAILURE);
    }

    /* Initialize all entries to NULL */
    for(unsigned int i = 0; i < size; i++)
        htable->table[i] = NULL;
    
    return htable;
}

/* Create a new hash table entry (key, val) and return it. */
hashtable_entry* hashtable_newentry(char* key, unsigned int val) {
    if(key == NULL)
        return NULL;

    hashtable_entry* new_entry;

	if((new_entry = (hashtable_entry*)malloc(sizeof(hashtable_entry))) == NULL ) {
        printf("[ERROR] There was an error while trying to call 'malloc' on 'new_entry'. Closing...\n");
		exit(EXIT_FAILURE);
	}

	if((new_entry->key = (char*)malloc(65)) == NULL ) {
		printf("[ERROR] There was an error while trying to call 'malloc' on 'new_entry->key'. Closing...\n");
		exit(EXIT_FAILURE);
	}

    /* Initialize entry with key,val and "next" pointer set to NULL */
    strcpy(new_entry->key, key);
    new_entry->val = val;
    new_entry->next = NULL;

	return new_entry;
}

/* Insert a new entry (or, if already present, update it) in
   the hash table and return the entry just inserted/updated. */
hashtable_entry* hashtable_insert(hashtable* htable, char* key, unsigned int val) {
    if(htable == NULL || key == NULL)
        return NULL;

    unsigned int hash = hashtable_gethash(htable->size, key);

    // printf("Insert: %s -> %u\n", key, hash);

    hashtable_entry* current_entry = htable->table[hash];
    
    /* Check if the new entry is the first one with that hash value, then
       add it as the head of the chaining list. */
    if(current_entry == NULL) {
        hashtable_entry* new_entry = hashtable_newentry(key, val);
        htable->table[hash] = new_entry;
        (htable->different_entries)++;

        return new_entry;
    }

    /* There is already at least one entry with the same hash value. Search
       if the key is already present in the chaining list. */
    int found = false;
    while(true) {
        if(strcmp(current_entry->key, key) == 0) {
            found = true;
            break;
        }
        if(current_entry->next == NULL)
            break;
        
        current_entry = current_entry->next;
    }
    
    /* The key is already present, so update its value. */
    if(found) {
        current_entry->val = val;

        return current_entry;
    }

    /* The key is not present, so insert it at the end of the chaining list. */
    current_entry->next = hashtable_newentry(key, val);
    (htable->collisions)++;

    return current_entry->next;
}

/* Search an entry by 'key' and delete it if found, returning
   its value. Return 0 if 'key' was not found. */
unsigned int hashtable_delete(hashtable* htable, char* key) {
    if(htable == NULL || key == NULL)
        return 0;

    unsigned int hash = hashtable_gethash(htable->size, key);

    // printf("Delete: %s -> %u\n", key, hash);

    hashtable_entry* current_entry = htable->table[hash];

    if(current_entry == NULL) {
        return 0;
    }

    /* Search the entry in the chaining list */
    hashtable_entry* previous_entry = current_entry;
    while(current_entry != NULL && strcmp(current_entry->key, key) != 0) {
        previous_entry = current_entry;
        current_entry = current_entry->next;
    }
    
    if(current_entry != NULL) {
        int val = current_entry->val;

        /* Check if the entry is the head of the chaining list (htable->table[i]). */
        if(previous_entry == current_entry) {
            if(current_entry->next == NULL) {   /* Check if it is the only entry. */
                htable->table[hash] = NULL;
                (htable->different_entries)--;
            }
            else {
                htable->table[hash] = current_entry->next;
                (htable->collisions)--;
            }
        } else {
            previous_entry->next = current_entry->next;
            (htable->collisions)--;
        }

        /* Releases the memory of both the string in the entry and the entry itself.*/
        erease(current_entry->key, 65);
        erease(current_entry, sizeof(hashtable_entry));

        return val;
    }

    return 0;
}

/* Search an entry by 'key' and return the entry (key, val) if
   found, NULL otherwise. */
hashtable_entry* hashtable_get(hashtable* htable, char* key) {
    if(htable == NULL || key == NULL)
        return NULL;

    unsigned int hash = hashtable_gethash(htable->size, key);

    hashtable_entry* current_entry = htable->table[hash];

    while(true) {
        /* The end of the chaining list has been reached and the
           entry was not found. */
        if(current_entry == NULL)
            return NULL;
        if(strcmp(current_entry->key, key) == 0) {
            return current_entry;
        }
        current_entry = current_entry->next;
    }
}

/* Print an hash table with nice formatting of the individual entries. */
void hashtable_prettyprint(hashtable* htable) {
    if(htable == NULL) {
        printf("This hash table does not exist.\n");
        return;
    }
    
    /* Calculate the number of digits of the size of the
       hashtable (es. (1...9)->1, (10...99)->2, (100...999)->3).
       It will be used to add padding to the print. */
    char size_str[11];
    sprintf(size_str, "%u", htable->size);
    unsigned int padding_size = (unsigned int) strlen(size_str) + 1;

    hashtable_entry* current_entry = NULL;
    int dots = false;                       /* True if '[...]' has already been printed, false otherwise.  */
    int consecutive_null = 0;               /* Number of consecutive empty (NULL) hashtable entries. */

    for(unsigned int i = 0; i < htable->size; i++) {       
        if(htable->table[i] == NULL) {
            consecutive_null++;
            
            /* If first or last entry is NULL, then print it. */
            if(i == 0 || i == htable->size-1) {
                printf("%*d --> NULL\n", padding_size, i);
            } else {
                /* If this NULL entry is the first or the last (next one is not NULL), then print it. */
                if(consecutive_null == 1 || htable->table[i+1] != NULL) {
                    printf("%*d --> NULL\n", padding_size, i);
                } else {
                    /* If there is more than one NULL entry, print "truncation points" -> [...]. */
                    if(!dots) {
                        /* Print as many dots as it needs to fill the
                           padding size (minimum 3 dots). */
                        if(padding_size < 7) {
                            printf(" [...]\n");
                        }
                        else {
                            printf(" [");
                            for(unsigned int j = 0; j < padding_size-3; j++)
                                printf(".");
                            printf("]\n");
                        }
                        dots = true;
                    }
                }
            }
        } else {
            consecutive_null = 0;
            dots = false;

            /* Print first entry for a specific hash value. */
            printf("%*d --> {(%s, %u)", padding_size, i, htable->table[i]->key, htable->table[i]->val);

            /* If there are, print all collisions for a specific hash value. */
            current_entry = htable->table[i]->next;
            while(current_entry != NULL) {
                printf(", (%s, %u)", current_entry->key, current_entry->val);
                
                current_entry = current_entry->next;
            }
            printf("}\n");
        }
    }
    printf("\n\n");
}

/* Test function: 
    • add 12 unique string (10 characters) to an hash table;
    • delete 4 strings;
    • change value of 3 strings;
    At each single step, it pretty prints the entire hash table. */
void test_12_strings() {
    hashtable* htable = hashtable_newhashtable(16);

    printf("Empty hashtable\n");
    hashtable_prettyprint(htable);

    printf("\nInsert strings (8ct4xaucod, 7i2pefipwc, mmnoy7c6yq, ouam4phm2c, e2xztziqtj, wrrw5arl6d, 7lc5pgl8kd, 93i5i8sx17, 6kkd8e0zq1, yeqmy6bjmk, hn1gybiuy6, 5wr2vyui8t), with value '0', in the hash table:\n");
    hashtable_insert(htable, "8ct4xaucod", 0); /* hash = 7 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "7i2pefipwc", 0); /* hash = 0 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "mmnoy7c6yq", 0); /* hash = 10 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "ouam4phm2c", 0); /* hash = 0 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "e2xztziqtj", 0); /* hash = 15 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "wrrw5arl6d", 0); /* hash = 0 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "7lc5pgl8kd", 0); /* hash = 5 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "93i5i8sx17", 0); /* hash = 14 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "6kkd8e0zq1", 0); /* hash = 9 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "yeqmy6bjmk", 0); /* hash = 15 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "hn1gybiuy6", 0); /* hash = 6 */
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "5wr2vyui8t", 0); /* hash = 9 */
    hashtable_prettyprint(htable);

    printf("\nDelete strings (7lc5pgl8kd, 6kkd8e0zq1, e2xztziqtj, yeqmy6bjmk) from the hash table:\n");
    hashtable_delete(htable, "7lc5pgl8kd");
    hashtable_prettyprint(htable);
    hashtable_delete(htable, "6kkd8e0zq1");
    hashtable_prettyprint(htable);
    hashtable_delete(htable, "e2xztziqtj");
    hashtable_prettyprint(htable);
    hashtable_delete(htable, "yeqmy6bjmk");
    hashtable_prettyprint(htable);

    printf("\nChange value of strings (ouam4phm2c -> 37, 93i5i8sx17 -> 55, 5wr2vyui8t -> 79) in the hash table:\n");
    hashtable_insert(htable, "ouam4phm2c", 37);
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "93i5i8sx17", 55);
    hashtable_prettyprint(htable);
    hashtable_insert(htable, "5wr2vyui8t", 79);
    hashtable_prettyprint(htable);
}

/* Test function: reads from a file (rnd_str.txt) which contains 100.000
   unique strings and adds them all into an hash table. */
void test_100000_strings() {
    hashtable* htable = hashtable_newhashtable(262144); /* 2^18 = 262144 */

    FILE* file;
    if((file = fopen("rnd_str.txt", "r")) == NULL) {
        printf("[ERROR] There was an error while trying to call 'fopen' on 'rnd_str_2.txt'. Closing...\n");
		exit(EXIT_FAILURE);
    }
    
    /* Read line by line, up to 64 characters (+1 for string terminator '\0'), 
       and add this string to the hash table. */
    char line[65];
    while (fgets(line, sizeof(line), file)) {
        line[64] = '\0';

        hashtable_insert(htable, line, 0);
    }
    fclose(file);

    hashtable_prettyprint(htable);
}

int main() {
    /* Print a simple choice menu */
    printf("Welcome to the String Hash Table implementation in C!\n\n");
    printf("There are two test functions available:\n");
    printf("  1) Test with 12 different strings, each 10 characters long\n");
    printf("  2) Test with 100.000 different strings, each 64 characters long, written in a file called \"rnd_str.txt\"\n");
    printf("  3) Exit\n");
    
    /* Reads from input (stdin) a choice between 1 and 3 */
    char tmp_buff[16];
    int option, result;
    do {
        printf("Please, choose an option [1,2,3]: ");
        if (fgets(tmp_buff, sizeof(tmp_buff), stdin) == NULL) {
            option = -1;
            break;
        }
        result = sscanf(tmp_buff, "%d", &option);
    } while(result != 1 || option < 1 || option > 3);

    switch (option) {
        case 1:
            test_12_strings();
            break;
        case 2:
            test_100000_strings();
            break;
        case 3:
            printf("\nGoodbye! :)\n");
            break;
        
        default:
            printf("[ERROR] There was an error while trying to read the value. Closing...\n");
            exit(EXIT_FAILURE);
            break;
    }
    
    return EXIT_SUCCESS;
}